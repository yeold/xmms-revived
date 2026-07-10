#include "config.h"
#include "libxmms/titlestring.h"
#include "libxmms/util.h"
#include "xmms/i18n.h"
#include "xmms/plugin.h"

#include <FLAC/stream_decoder.h>
#include <string.h>

#include "flac.h"

FLAC__StreamDecoder *decoder;

static InputPlugin flac_ip;

static void init_flac()
{
  decoder = FLAC__stream_decoder_new();
}

static void play_flac(char *filename)
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

  // (*title) = get_title(filename);
  (*length) = -1;
}

InputPlugin *get_iplugin_info(void)
{
  flac_ip.description = g_strdup_printf("FLAC Player %s", VERSION);
  return &flac_ip;
}

static InputPlugin flac_ip = {
    NULL, NULL, NULL, init_flac, NULL, NULL,         is_our_file, NULL, play_flac, NULL, NULL,
    NULL, NULL, NULL, NULL,      NULL, cleanup_flac, NULL,        NULL, NULL,      NULL, get_song_info,
};