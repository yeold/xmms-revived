#include "config.h"
#include "libxmms/configfile.h"
#include "libxmms/titlestring.h"
#include "libxmms/util.h"
#include "xmms/i18n.h"
#include "xmms/plugin.h"

#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PCM_RATE 44100
#define PCM_CHANNELS 2
#define PCM_FMT FMT_S16_NE
#define CHUNK_SIZE 4096
#define POLL_TIMEOUT 100
#define FRAME_SIZE 4 /* 2ch * 16bit */

#define MPRIS_PATH "/org/mpris/MediaPlayer2"
#define MPRIS_IFACE "org.mpris.MediaPlayer2.Player"

#define DEFAULT_FIFO "/run/user/1000/spotifyd.pcm"

#define MPRIS_PREFIX "org.mpris.MediaPlayer2.spotifyd"

static char *mpris_bus_name = NULL;

static InputPlugin spotifyd_ip;

static pthread_t decoder_thread;
static volatile int running = 0;
static volatile int paused = 0;
static volatile int eof = 0;
static int pipe_fd = -1;
static char *fifo_path = NULL;

static GDBusProxy *proxy = NULL;
static GMutex meta_lock;
static char *cur_title = NULL;
static char *cur_trackid = NULL; /* object path, needed by SetPosition */
static gint cur_length = -1;     /* ms */

/* forward decls */
static void track_changed(void);
static void playback_status_changed(const char *status);
static gboolean stop_from_idle(gpointer data);

/* Ask the bus daemon for all names and find spotifyd's. */
static char *find_spotifyd_name(void)
{
  GDBusConnection *conn;
  GVariant *reply, *arr;
  GVariantIter iter;
  const char *name;
  char *found = NULL;

  conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
  if (!conn)
    return NULL;

  reply =
      g_dbus_connection_call_sync(conn, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                  "ListNames", NULL, G_VARIANT_TYPE("(as)"), G_DBUS_CALL_FLAGS_NONE, 1000, NULL, NULL);

  if (!reply)
  {
    g_object_unref(conn);
    return NULL;
  }

  arr = g_variant_get_child_value(reply, 0);
  g_variant_iter_init(&iter, arr);
  while (g_variant_iter_next(&iter, "&s", &name))
  {
    if (g_str_has_prefix(name, MPRIS_PREFIX))
    {
      found = g_strdup(name);
      break;
    }
  }

  g_variant_unref(arr);
  g_variant_unref(reply);
  g_object_unref(conn);
  return found;
}

/* ---------- metadata ---------- */

static void parse_metadata(GVariant *meta)
{
  GVariantIter iter;
  const char *key;
  GVariant *val;
  char *artist = NULL, *title = NULL, *trackid = NULL;
  gint64 len_us = -1;

  g_variant_iter_init(&iter, meta);
  while (g_variant_iter_next(&iter, "{&sv}", &key, &val))
  {
    if (!strcmp(key, "xesam:title"))
      title = g_variant_dup_string(val, NULL);
    else if (!strcmp(key, "xesam:artist"))
    {
      if (g_variant_n_children(val) > 0)
      {
        GVariant *a = g_variant_get_child_value(val, 0);
        artist = g_variant_dup_string(a, NULL);
        g_variant_unref(a);
      }
    }
    else if (!strcmp(key, "mpris:length"))
      len_us = g_variant_get_int64(val);
    else if (!strcmp(key, "mpris:trackid"))
    {
      if (g_variant_is_of_type(val, G_VARIANT_TYPE_OBJECT_PATH) || g_variant_is_of_type(val, G_VARIANT_TYPE_STRING))
        trackid = g_variant_dup_string(val, NULL);
    }
    g_variant_unref(val);
  }

  g_mutex_lock(&meta_lock);
  g_free(cur_title);
  cur_title = (artist && title) ? g_strdup_printf("%s - %s", artist, title) : g_strdup(title ? title : "spotify");
  if (trackid)
  {
    g_free(cur_trackid);
    cur_trackid = trackid;
  }
  cur_length = (len_us > 0) ? (gint)(len_us / 1000) : -1;
  g_mutex_unlock(&meta_lock);

  g_free(artist);
  g_free(title);
}

/* runs on the main loop */
static void on_props_changed(GDBusProxy *p, GVariant *changed, GStrv invalidated, gpointer user_data)
{
  const char *status = NULL;
  GVariant *meta;

  meta = g_variant_lookup_value(changed, "Metadata", G_VARIANT_TYPE_VARDICT);
  if (meta)
  {
    parse_metadata(meta);
    g_variant_unref(meta);
    track_changed();
  }

  if (g_variant_lookup(changed, "PlaybackStatus", "&s", &status))
    playback_status_changed(status);
}

/* ---------- mpris lifecycle ---------- */
static void call_async(const char *method, GVariant *params)
{
  if (!proxy)
  {
    if (params)
      g_variant_unref(g_variant_ref_sink(params));
    return;
  }
  g_dbus_proxy_call(proxy, method, params, G_DBUS_CALL_FLAGS_NONE, 2000, NULL, NULL, NULL);
}

static gboolean mpris_open_uri(const char *xmms_url)
{
  char *tail, *uri;

  if (!proxy || !xmms_url)
    return FALSE;
  if (g_ascii_strncasecmp(xmms_url, "spotify://", 10) != 0)
    return FALSE;

  tail = g_strdup(xmms_url + 10);
  g_strdelimit(tail, "/", ':');
  uri = g_strdup_printf("spotify:%s", tail);
  g_free(tail);

  call_async("OpenUri", g_variant_new("(s)", uri));
  g_free(uri);
  return TRUE;
}

static void mpris_play(void)
{
  call_async("Play", NULL);
}
static void mpris_pause(void)
{
  call_async("Pause", NULL);
}
static void mpris_stop(void)
{
  call_async("Stop", NULL);
}

static void mpris_seek_abs(gint64 usec)
{
  char *tid;

  g_mutex_lock(&meta_lock);
  tid = g_strdup(cur_trackid ? cur_trackid : "");
  g_mutex_unlock(&meta_lock);

  /* g_variant_new("(ox)") aborts on a malformed object path */
  if (!g_variant_is_object_path(tid))
  {
    g_free(tid);
    return;
  }

  call_async("SetPosition", g_variant_new("(ox)", tid, usec));
  g_free(tid);
}

static char *mpris_get_title(void)
{
  char *t;
  g_mutex_lock(&meta_lock);
  t = g_strdup(cur_title ? cur_title : "spotify");
  g_mutex_unlock(&meta_lock);
  return t;
}

static gint mpris_get_length(void)
{
  gint l;
  g_mutex_lock(&meta_lock);
  l = cur_length;
  g_mutex_unlock(&meta_lock);
  return l;
}

static gboolean mpris_ensure(void)
{
  GError *err = NULL;
  char *owner;

  if (proxy)
  {
    owner = g_dbus_proxy_get_name_owner(proxy);
    if (owner)
    {
      g_free(owner);
      return TRUE;
    }
    g_object_unref(proxy);
    proxy = NULL;
    g_free(mpris_bus_name);
    mpris_bus_name = NULL;
  }

  mpris_bus_name = find_spotifyd_name();
  if (!mpris_bus_name)
    return FALSE;

  proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START, NULL, mpris_bus_name,
                                        MPRIS_PATH, MPRIS_IFACE, NULL, &err);

  if (!proxy)
  {
    g_warning("spotifyd: MPRIS proxy failed: %s", err->message);
    g_error_free(err);
    g_free(mpris_bus_name);
    mpris_bus_name = NULL;
    return FALSE;
  }

  g_signal_connect(proxy, "g-properties-changed", G_CALLBACK(on_props_changed), NULL);
  return TRUE;
}

/* ---------- config ---------- */

static void read_config(void)
{
  ConfigFile *cfg;
  char *path = NULL;

  cfg = xmms_cfg_open_default_file();
  if (cfg)
  {
    xmms_cfg_read_string(cfg, "spotifyd", "fifo_path", &path);
    xmms_cfg_free(cfg);
  }
  else
  {
    g_warning("No config file loaded");
  }

  g_free(fifo_path);
  fifo_path = path ? path : g_strdup(DEFAULT_FIFO);
}

/* ---------- plugin init/cleanup ---------- */

static void spotifyd_init(void)
{
  g_mutex_init(&meta_lock);
  read_config();
}

static void spotifyd_cleanup(void)
{
  if (proxy)
  {
    g_object_unref(proxy);
    proxy = NULL;
  }
  g_free(mpris_bus_name);
  mpris_bus_name = NULL;
  g_free(cur_title);
  cur_title = NULL;
  g_free(cur_trackid);
  cur_trackid = NULL;
  g_free(fifo_path);
  fifo_path = NULL;
  g_mutex_clear(&meta_lock);
}

/* ---------- main-loop callbacks ---------- */

static void track_changed(void)
{
  if (!running)
    return;
  /* core takes ownership of the title string */
  spotifyd_ip.set_info(mpris_get_title(), mpris_get_length(), 320 * 1000, PCM_RATE, PCM_CHANNELS);
}

static gboolean stop_from_idle(gpointer data)
{
  eof = 1; /* get_time() will return -1 once the buffer drains */
  return FALSE;
}

static void playback_status_changed(const char *status)
{
  if (!status)
    return;
  if (!strcmp(status, "Stopped"))
    g_idle_add(stop_from_idle, NULL);
}

/* ---------- decoder ---------- */

static void *decoder_loop(void *arg)
{
  char buffer[CHUNK_SIZE];
  struct pollfd pfd;

  pfd.fd = pipe_fd;
  pfd.events = POLLIN;

  while (running)
  {
    ssize_t n;
    int r;

    if (paused)
    {
      g_usleep(50000);
      continue;
    }

    r = poll(&pfd, 1, POLL_TIMEOUT);

    if (r < 0)
    {
      if (errno == EINTR)
        continue;
      break;
    }

    if (r == 0)
      continue;

    if (pfd.revents & (POLLERR | POLLHUP))
    {
      /* writer gone: spotifyd stopped or restarted. Reopen, but sleep
         first — a writerless FIFO returns POLLHUP instantly and this
         would otherwise spin a core. */
      close(pipe_fd);
      g_usleep(200000);
      pipe_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
      if (pipe_fd < 0)
        break;
      pfd.fd = pipe_fd;
      continue;
    }

    n = read(pipe_fd, buffer, sizeof(buffer));

    if (n < 0)
    {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      break;
    }

    if (n == 0)
      continue;

    n -= n % FRAME_SIZE;
    if (n == 0)
      continue;

    spotifyd_ip.add_vis_pcm(spotifyd_ip.output->written_time(), PCM_FMT, PCM_CHANNELS, (int)n, buffer);

    while (running && spotifyd_ip.output->buffer_free() < (int)n)
    {
      if (paused)
        break;
      g_usleep(10000);
    }

    if (!running)
      break;
    if (paused)
      continue; /* drop this chunk rather than write into a full buffer */

    spotifyd_ip.output->write_audio(buffer, (int)n);
  }

  spotifyd_ip.output->close_audio();
  return NULL;
}

/* ---------- input plugin callbacks ---------- */

static void play_stream(char *filename)
{
  if (mpris_ensure())
    mpris_open_uri(filename);
  else
    g_warning("spotifyd: MPRIS unavailable, continuing without control");

  pipe_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
  if (pipe_fd < 0)
  {
    g_warning("spotifyd: cannot open %s: %s", fifo_path, g_strerror(errno));
    return;
  }

  {
    char junk[4096];
    int i;
    for (i = 0; i < 64; i++)
      if (read(pipe_fd, junk, sizeof(junk)) <= 0)
        break;
  }

  if (spotifyd_ip.output->open_audio(PCM_FMT, PCM_RATE, PCM_CHANNELS) == 0)
  {
    close(pipe_fd);
    pipe_fd = -1;
    return;
  }

  spotifyd_ip.set_info(mpris_get_title(), mpris_get_length(), 320 * 1000, PCM_RATE, PCM_CHANNELS);

  running = 1;
  paused = 0;
  eof = 0;
  pthread_create(&decoder_thread, NULL, decoder_loop, NULL);
}

static void stop_stream(void)
{
  if (!running)
    return;

  running = 0;
  paused = 0;
  pthread_join(decoder_thread, NULL);

  if (pipe_fd >= 0)
  {
    close(pipe_fd);
    pipe_fd = -1;
  }

  mpris_stop();
}

static void pause_stream(short p)
{
  paused = p;
  spotifyd_ip.output->pause(p);
  if (p)
    mpris_pause();
  else
    mpris_play();
}

static void seek_stream(int time)
{
  mpris_seek_abs((gint64)time * 1000000);
  spotifyd_ip.output->flush(time * 1000);
}

static int get_time(void)
{
  if (!running)
    return -1;
  if (eof && !spotifyd_ip.output->buffer_playing())
    return -1; /* tells XMMS the song ended */
  return spotifyd_ip.output->output_time();
}

static int is_our_file(char *filename)
{
  if (filename == NULL)
    return FALSE;
  return g_ascii_strncasecmp(filename, "spotify://", 10) == 0;
}

static void get_song_info(char *filename, char **title, int *length)
{
  char *tail;

  *length = -1;
  *title = NULL;

  if (!filename || g_ascii_strncasecmp(filename, "spotify://", 10) != 0)
    return;

  tail = g_strdup(filename + 10);
  g_strdelimit(tail, "/", ':');
  *title = g_strdup_printf("spotify:%s", tail);
  g_free(tail);
}

static InputPlugin spotifyd_ip = {
    .description = NULL,
    .init = spotifyd_init,
    .cleanup = spotifyd_cleanup,
    .is_our_file = is_our_file,
    .play_file = play_stream,
    .stop = stop_stream,
    .pause = pause_stream,
    .seek = seek_stream,
    .get_time = get_time,
    .get_song_info = get_song_info,
};

InputPlugin *get_iplugin_info(void)
{
  if (!spotifyd_ip.description)
    spotifyd_ip.description = g_strdup_printf("Spotifyd Player %s", VERSION);
  return &spotifyd_ip;
}