#include "config.h"
#include "libxmms/titlestring.h"
#include "libxmms/util.h"
#include "xmms/i18n.h"

#include "xmms/plugin.h"
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#define PCM_RATE 44100
#define PCM_CHANNELS 2
#define PCM_FMT FMT_S16_NE
#define CHUCK_SIZE 4096
#define POLL_TIMEOUT 100

static InputPlugin spotifyd_ip;

static pthread_t decoder_thread;
static volatile int running = 0;
static volatile int paused = 0;
static int pipe_fd = -1;

static void seek_stream(int time)
{
  // seek_to = time;
  // while (seek_to != -1 && is_playing)
  //   xmms_usleep(10000);
  (void)time;
}

static void *decoder_loop(void *arg)
{
  char buffer[CHUCK_SIZE];
  struct pollfd pfd;

  pfd.fd = pipe_fd;
  pfd.events = POLLIN;

  while (running)
  {
    if (paused)
    {
      xmms_usleep(50000);
      continue;
    }

    int r = poll(&pfd, 1, POLL_TIMEOUT);

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
      close(pipe_fd);
      pipe_fd = open("/run/user/1000/spotifyd.pcm", O_RDONLY | O_NONBLOCK);
      if (pipe_fd < 0)
        break;

      pfd.fd = pipe_fd;
      continue;
    }

    ssize_t n = read(pipe_fd, buffer, sizeof(buffer));

    if (n < 0)
    {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      break;
    }

    if (n == 0)
      continue;

    n -= n % 4;
    if (n == 0)
      continue;

    spotifyd_ip.add_vis_pcm(spotifyd_ip.output->written_time(), PCM_FMT, PCM_CHANNELS, n, buffer);

    while (running && !paused && spotifyd_ip.output->buffer_free() < n)
      g_usleep(10000);

    if (!running)
      break;

    spotifyd_ip.output->write_audio(buffer, n);
  }

  spotifyd_ip.output->close_audio();

  g_thread_exit(NULL);

  return NULL;
}

static void play_stream(char *filename)
{
  // mpris_open_uri(filename): // TODO

  pipe_fd = open("/run/user/1000/spotifyd.pcm", O_RDONLY | O_NONBLOCK);
  if (pipe_fd < 0)
    return;

  if (spotifyd_ip.output->open_audio(PCM_FMT, PCM_RATE, PCM_CHANNELS) == 0)
  {
    close(pipe_fd);
    pipe_fd = -1;
    return;
  }

  char *title = g_path_get_basename(filename);

  spotifyd_ip.set_info(title, -1, 320 * 1000, PCM_RATE, PCM_CHANNELS);
  g_free(title);

  running = 1;
  paused = 0;
  pthread_create(&decoder_thread, NULL, decoder_loop, NULL);
}

static void stop_stream()
{
  if (!running)
    return;

  running = FALSE;
  pthread_join(decoder_thread, NULL);

  if (pipe_fd >= 0)
  {
    close(pipe_fd);
    pipe_fd = -1;
  }

  // spotify_mpris_stop();
}

static void pause_stream(short p)
{
  paused = p;
  spotifyd_ip.output->pause(p);
}

static int get_time()
{
  if (!running)
    return -1;

  return spotifyd_ip.output->output_time();
}

static int is_our_file(char *filename)
{
  if (filename == NULL)
    return FALSE;

  return g_ascii_strncasecmp(filename, "spotify://", 10);
}

static char *spotify_uri_from_url(const char *url)
{
  /* spotify://track/ID -> spotify:track:ID */
  char *uri = g_strdup(url + strlen("spotify://"));
  g_strdelimit(uri, "/", ':');
  return g_strconcat("spotify:", uri, NULL); /* leaks uri — see below */
}

static void get_song_info(char *filename, char **title, int *length)
{

  (*title) = spotify_uri_from_url(filename);
  (*length) = -1;
}

InputPlugin *get_iplugin_info(void)
{
  spotifyd_ip.description = g_strdup_printf("Spotifyd Player %s", VERSION);
  return &spotifyd_ip;
}

static InputPlugin spotifyd_ip = {
    .description = NULL,
    .is_our_file = is_our_file,
    .play_file = play_stream,
    .stop = stop_stream,
    .pause = pause_stream,
    .get_time = get_time,
    .get_song_info = get_song_info,
    .seek = seek_stream,
};