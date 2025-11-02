#ifndef IDCIN_H
#define IDCIN_H

#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "xmms/plugin.h"

extern InputPlugin idcin_ip;

typedef struct
{
	int count;
	unsigned char used;
	int children[2];
} hnode_t;

typedef struct
{
	int bit_count;
	unsigned long bits;
} hcbits_t;

typedef struct idcmap_st
{
	unsigned char r_cmap[260], g_cmap[260], b_cmap[260];
	struct idcmap_st *next;
} idcmap_t;

typedef struct
{
	unsigned long pos;
	idcmap_t *cmap;
} frame_info_t;

typedef struct
{
	int going, eof;
	FILE *fp;
	int width, height;
	int snd_rate, snd_width, snd_channels;
	unsigned char *img_buf, *huff_data;
	unsigned long num_frames;
	frame_info_t frames[5000];
	idcmap_t *cmap;
} CinFile;

static int idcin_is_our_file(char *filename);
static void idcin_init(void);
static void idcin_play_file(char *filename);
static void idcin_stop(void);
static void idcin_pause(short p);
static void idcin_seek(int time);
static int idcin_get_time(void);

#endif

