#include "flac.h"
#include "config.h"
#include "libxmms/titlestring.h"
#include "libxmms/util.h"
#include "xmms/i18n.h"
#include "xmms/plugin.h"

#include <FLAC/format.h>
#include <FLAC/ordinals.h>
#include <FLAC/stream_decoder.h>
#include <glib.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>

static InputPlugin flac_ip;

static FLAC__StreamDecoder *decoder = NULL;
static pthread_t decoder_thread;
static volatile gboolean is_playing = FALSE;
static volatile gboolean eof_reached = FALSE;
static volatile int seek_to = -1;

static unsigned sample_rate;
static unsigned channels;
static unsigned bits_per_sample;

static int song_len;
static gint16 pcm_buf[65536];

static FLAC__StreamDecoderWriteStatus flac_write_cb(const FLAC__StreamDecoder *dec, const FLAC__Frame *frame,
                                                    const FLAC__int32 *const buffer[], void *client_data)
{
  unsigned samples = frame->header.blocksize;
  unsigned ch = frame->header.channels;
  unsigned shift = frame->header.bits_per_sample -
                   16; // shitfing it down from 24 bit to 16 bit since XMMS can only handle 16 at the moment
  unsigned i;
  unsigned c;
  gint16 *ptr = pcm_buf;
  int bytes;

  for (i = 0; i < samples; i++)
  {
    for (c = 0; c < ch; c++)
    {
      *ptr++ = (frame->header.bits_per_sample > 16) ? (gint16)(buffer[c][i] >> shift) : (gint16)buffer[c][i];
    }
  }

  bytes = samples * ch * 2;

  flac_ip.add_vis_pcm(flac_ip.output->written_time(), FMT_S16_NE, ch, bytes, pcm_buf);

  while (is_playing && seek_to == -1 && flac_ip.output->buffer_free() < bytes)
    xmms_usleep(10000);

  if (!is_playing)
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  if (seek_to != -1)
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE; /* skip write; decode_loop will seek */

  flac_ip.output->write_audio((char *)pcm_buf, bytes);

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_metadata_cb(const FLAC__StreamDecoder *dec, const FLAC__StreamMetadata *meta, void *client_data)
{
  if (meta->type == FLAC__METADATA_TYPE_STREAMINFO)
  {
    sample_rate = meta->data.stream_info.sample_rate;
    channels = meta->data.stream_info.channels;
    bits_per_sample = meta->data.stream_info.bits_per_sample;
    if (sample_rate > 0)
      song_len = (int)(meta->data.stream_info.total_samples * 1000 / sample_rate);
  }
}

static void flac_error_cb(const FLAC__StreamDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
  g_warning("flac plugin: decoder error: %s", FLAC__StreamDecoderErrorStatusString[status]);
}

static void seek_flac(int time)
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
      FLAC__uint64 target = (FLAC__uint64)seek_to * sample_rate;

      if (FLAC__stream_decoder_seek_absolute(decoder, target))
      {
        flac_ip.output->flush(seek_to * 1000);
        eof_reached = FALSE;
      }
      else
      {
        g_warning("flac: seek to %d s failed, decoder_state=%s", seek_to,
                  FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
        if (FLAC__stream_decoder_get_state(decoder) == FLAC__STREAM_DECODER_SEEK_ERROR)
          FLAC__stream_decoder_flush(decoder);
      }
      seek_to = -1;
    }

    if (eof_reached)
    {
      if (!flac_ip.output->buffer_playing())
        break; /* fully drained: done */
      xmms_usleep(10000);
      continue; /* drain, but keep servicing seeks */
    }

    if (!FLAC__stream_decoder_process_single(decoder))
    {
      g_warning("flac: process_single failed, decoder_state=%s",
                FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
      break;
    }

    if (FLAC__stream_decoder_get_state(decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
      eof_reached = TRUE;
  }

  is_playing = FALSE;
  pthread_exit(NULL);
}

static void play_flac(char *filename)
{
  char *title;

  song_len = -1;
  eof_reached = FALSE;

  decoder = FLAC__stream_decoder_new();
  if (!decoder)
    return;

  if (FLAC__stream_decoder_init_file(decoder, filename, flac_write_cb, flac_metadata_cb, flac_error_cb, NULL) !=
      FLAC__STREAM_DECODER_INIT_STATUS_OK)
  {
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    return;
  }

  if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder))
  {
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    return;
  }

  if (flac_ip.output->open_audio(FMT_S16_NE, sample_rate, channels) == 0)
  {
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
    return;
  }

  title = g_path_get_basename(filename);

  flac_ip.set_info(title, song_len, sample_rate * channels * bits_per_sample, sample_rate, channels);
  g_free(title);

  is_playing = TRUE;
  pthread_create(&decoder_thread, NULL, decoder_loop, NULL);
}

static void stop_flac()
{
  if (!is_playing && !decoder)
    return;

  is_playing = FALSE;
  pthread_join(decoder_thread, NULL);

  flac_ip.output->close_audio();
  if (decoder)
  {
    FLAC__stream_decoder_delete(decoder);
    decoder = NULL;
  }
}

static void pause_flac(short p)
{
  flac_ip.output->pause(p);
}

static int get_time_flac()
{
  if (!is_playing && !flac_ip.output->buffer_playing())
    return -1;

  return flac_ip.output->output_time();
}

static int is_our_file(char *filename)
{
  char *ext = strrchr(filename, '.');
  return ext && !strcasecmp(ext, ".flac");
}

static void get_song_info(char *filename, char **title, int *length)
{

  (*title) = g_path_get_basename(filename);
  (*length) = -1;
}

InputPlugin *get_iplugin_info(void)
{
  flac_ip.description = g_strdup_printf("FLAC Player %s", VERSION);
  return &flac_ip;
}

static InputPlugin flac_ip = {
    .description = NULL,
    .is_our_file = is_our_file,
    .play_file = play_flac,
    .stop = stop_flac,
    .pause = pause_flac,
    .get_time = get_time_flac,
    .get_song_info = get_song_info,
    .seek = seek_flac,
};