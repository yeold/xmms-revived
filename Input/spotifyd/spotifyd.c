#include "config.h"
#include "libxmms/titlestring.h"
#include "libxmms/util.h"
#include "xmms/i18n.h"
#include "xmms/plugin.h"

#include <glib.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>

static InputPlugin spotifyd_ip;

static pthread_t decoder_thread;
static volatile gboolean is_playing = FALSE;
static volatile gboolean eof_reached = FALSE;
static volatile int seek_to = -1;

static unsigned sample_rate;
static unsigned channels;
static unsigned bits_per_sample;

static int song_len;
static gint16 pcm_buf[65536];

static void seek_stream(int time)
{
  seek_to = time;
  while (seek_to != -1 && is_playing)
    xmms_usleep(10000);
}

static void *decoder_loop(void *arg)
{
  while (is_playing)
  {
    if (seek_to != -1)
    {
      seek_to = -1;
    }

    if (eof_reached)
    {
      if (!spotifyd_ip.output->buffer_playing())
        break; /* fully drained: done */
      xmms_usleep(10000);
      continue; /* drain, but keep servicing seeks */
    }
  }

  is_playing = FALSE;
  pthread_exit(NULL);
}

static void play_stream(char *filename)
{
  char *title;

  song_len = -1;
  eof_reached = FALSE;
  AFormat fmt;

  fmt = (bits_per_sample > 16) ? FMT_S32_NE : FMT_S16_NE;

  if (spotifyd_ip.output->open_audio(fmt, sample_rate, channels) == 0)
  {

    return;
  }

  title = g_path_get_basename(filename);

  spotifyd_ip.set_info(title, song_len, sample_rate * channels * bits_per_sample, sample_rate, channels);
  g_free(title);

  is_playing = TRUE;
  pthread_create(&decoder_thread, NULL, decoder_loop, NULL);
}

static void stop_stream()
{
  if (!is_playing)
    return;

  is_playing = FALSE;
  pthread_join(decoder_thread, NULL);

  spotifyd_ip.output->close_audio();
}

static void pause_stream(short p)
{
  spotifyd_ip.output->pause(p);
}

static int get_time()
{
  if (!is_playing && !spotifyd_ip.output->buffer_playing())
    return -1;

  return spotifyd_ip.output->output_time();
}

static int is_our_file(char *filename)
{
  if (filename == NULL)
    return FALSE;

  return g_ascii_strncasecmp(filename, "spotify://", 10);
}

static void get_song_info(char *filename, char **title, int *length)
{

  (*title) = g_path_get_basename(filename);
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