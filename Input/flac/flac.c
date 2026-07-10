#include "config.h"
#include "libxmms/titlestring.h"
#include "libxmms/util.h"
#include "xmms/i18n.h"
#include "xmms/plugin.h"
#include <FLAC/stream_decoder.h>

#include "flac.h"

FLAC__StreamDecoder *decoder;

static InputPlugin flac_ip;

static void init_flac()
{
  decoder = FLAC__stream_decoder_new();
}

static void play_flac()
{
}

static int is_our_file(char *filename)
{
  char *ext, *tmp;

  tmp = strrchr(filename, '/');
  ext = strrchr(filename, '.');

  if (ext)
  {
    if (!strcasecmp(ext, ".flac"))
      return 1;
  }

  return 0;
}

static void cleanup_flac()
{
  FLAC__stream_decoder_delete(decoder);
}

static void get_song_info(char *filename, char **title, int *length)
{
  FLAC__stream_decoder_s

      (*title) = get_title(filename);
  (*length) = -1;
}

static InputPlugin flac_ip = {
    NULL, NULL, NULL,     init_flac, NULL, NULL,         is_our_file, NULL, play_flac, stop, wav_pause,
    seek, NULL, get_time, NULL,      NULL, cleanup_flac, NULL,        NULL, NULL,      NULL, get_song_info,
};