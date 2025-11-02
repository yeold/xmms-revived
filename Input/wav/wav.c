/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "wav.h"
#include "libxmms/util.h"
#include "libxmms/titlestring.h"
#include "xmms/i18n.h"

InputPlugin wav_ip =
{
	NULL,
	NULL,
	NULL, /* Description */
	wav_init,
	NULL,
	NULL,
	is_our_file,
	NULL,
	play_file,
	stop,
	wav_pause,
	seek,
	NULL,
	get_time,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	get_song_info,
	NULL,			/* file_info_box */
	NULL
};

WaveFile *wav_file = NULL;
static pthread_t decode_thread;
static gboolean audio_error = FALSE;

InputPlugin *get_iplugin_info(void)
{
	wav_ip.description = g_strdup_printf(_("Wave Player %s"), VERSION);
	return &wav_ip;
}

static void wav_init(void)
{
}

/* needed for is_our_file() */
static int read_n_bytes(FILE * file, guint8 * buf, int n)
{

	if (fread(buf, 1, n, file) != n)
	{
		return FALSE;
	}
	return TRUE;
}

static guint32 convert_to_header(guint8 * buf)
{

	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static guint32 convert_to_long(guint8 * buf)
{

	return (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
}

static guint16 read_wav_id(char *filename)
{
	FILE *file;
	guint16 wavid;
	guint8 buf[4];
	guint32 head;
	long seek;

	if (!(file = fopen(filename, "rb")))
	{			/* Could not open file */
		return 0;
	}
	if (!(read_n_bytes(file, buf, 4)))
	{
		fclose(file);
		return 0;
	}
	head = convert_to_header(buf);
	if (head == ('R' << 24) + ('I' << 16) + ('F' << 8) + 'F')
	{			/* Found a riff -- maybe WAVE */
		if (fseek(file, 4, SEEK_CUR) != 0)
		{		/* some error occured */
			fclose(file);
			return 0;
		}
		if (!(read_n_bytes(file, buf, 4)))
		{
			fclose(file);
			return 0;
		}
		head = convert_to_header(buf);
		if (head == ('W' << 24) + ('A' << 16) + ('V' << 8) + 'E')
		{		/* Found a WAVE */
			seek = 0;
			do
			{	/* we'll be looking for the fmt-chunk which comes before the data-chunk */
				/* A chunk consists of an header identifier (4 bytes), the length of the chunk
				   (4 bytes), and the chunkdata itself, padded to be an even number of bytes.
				   We'll skip all chunks until we find the "data"-one which could contain
				   mpeg-data */
				if (seek != 0)
				{
					if (fseek(file, seek, SEEK_CUR) != 0)
					{	/* some error occured */
						fclose(file);
						return 0;
					}
				}
				if (!(read_n_bytes(file, buf, 4)))
				{
					fclose(file);
					return 0;
				}
				head = convert_to_header(buf);
				if (!(read_n_bytes(file, buf, 4)))
				{
					fclose(file);
					return 0;
				}
				seek = convert_to_long(buf);
				seek = seek + (seek % 2);	/* Has to be even (padding) */
				if (seek >= 2 && head == ('f' << 24) + ('m' << 16) + ('t' << 8) + ' ')
				{
					if (!(read_n_bytes(file, buf, 2)))
					{
						fclose(file);
						return 0;
					}
					wavid = buf[0] + 256 * buf[1];
					seek -= 2;
					/* we could go on looking for other things, but all we wanted was the wavid */
					fclose(file);
					return wavid;
				}
			}
			while (head != ('d' << 24) + ('a' << 16) + ('t' << 8) + 'a');
			/* it's RIFF WAVE */
		}
		/* it's RIFF */
	}
	/* it's not even RIFF */
	fclose(file);
	return 0;
}

static int is_our_file(char *filename)
{
	gchar *ext;

	ext = strrchr(filename, '.');
	if (ext)
		if (!strcasecmp(ext, ".wav"))
			if (read_wav_id(filename) == WAVE_FORMAT_PCM)
				return TRUE;
	return FALSE;
}

static gchar *get_title(gchar *filename)
{
	TitleInput *input;
	gchar *temp, *ext, *title;

	XMMS_NEW_TITLEINPUT(input);

	temp = g_strdup(filename);
	ext = strrchr(temp, '.');
	if (ext)
		*ext = '\0';
	input->file_name = g_basename(temp);
	input->file_ext = ext ? ext+1 : NULL;
	input->file_path = temp;

	title = xmms_get_titlestring(xmms_get_gentitle_format(), input);
	if ( title == NULL )
		title = g_strdup(input->file_name);

	g_free(temp);
	g_free(input);

	return title;
}

static int read_le_long(FILE * file, long *ret)
{
	unsigned char buf[4];

	if (fread(buf, 1, 4, file) != 4)
		return 0;

	*ret = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return TRUE;
}

static int read_le_short(FILE * file, short *ret)
{
	unsigned char buf[2];

	if (fread(buf, 1, 2, file) != 2)
		return 0;

	*ret = (buf[1] << 8) | buf[0];
	return TRUE;
}

static void *play_loop(void *arg)
{
	char data[2048 * 2];
	int bytes, blk_size, rate;
	int actual_read;

	blk_size = 512 * (wav_file->bits_per_sample / 8) * wav_file->channels;
	rate = wav_file->samples_per_sec * wav_file->channels * (wav_file->bits_per_sample / 8);
	while (wav_file->going)
	{
		if (!wav_file->eof)
		{
			bytes = blk_size;
			if (wav_file->length - wav_file->position < bytes)
				bytes = wav_file->length - wav_file->position;
			if (bytes > 0)
			{
				actual_read = fread(data, 1, bytes, wav_file->file);

				if (actual_read == -1)
				{
					wav_file->eof = 1;
					wav_ip.output->buffer_free();
					wav_ip.output->buffer_free();
				}
				else
				{
					wav_ip.add_vis_pcm(wav_ip.output->written_time(), (wav_file->bits_per_sample == 16) ? FMT_S16_LE : FMT_U8,
					   wav_file->channels, bytes, data);
					while(wav_ip.output->buffer_free() < bytes && wav_file->going && wav_file->seek_to == -1)
						xmms_usleep(10000);
					if(wav_file->going && wav_file->seek_to == -1)
						wav_ip.output->write_audio(data, bytes);
					wav_file->position += actual_read;
				}
			}
			else
			{
				wav_file->eof = TRUE;
				wav_ip.output->buffer_free();
				wav_ip.output->buffer_free();
				xmms_usleep(10000);
			}
		}
		else
			xmms_usleep(10000);
		if (wav_file->seek_to != -1)
		{
			wav_file->position = wav_file->seek_to * rate;
			fseek(wav_file->file, wav_file->position + wav_file->data_offset, SEEK_SET);
			wav_ip.output->flush(wav_file->seek_to * 1000);
			wav_file->seek_to = -1;
		}

	}
	fclose(wav_file->file);
	pthread_exit(NULL);
}

static void play_file(char *filename)
{
	char magic[4], *name;
	unsigned long len;
	int rate;

	audio_error = FALSE;

	wav_file = g_malloc(sizeof (WaveFile));
	memset(wav_file, 0, sizeof (WaveFile));
	if ((wav_file->file = fopen(filename, "rb")))
	{
		fread(magic, 1, 4, wav_file->file);
		if (strncmp(magic, "RIFF", 4))
		{
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		read_le_long(wav_file->file, &len);
		fread(magic, 1, 4, wav_file->file);
		if (strncmp(magic, "WAVE", 4))
		{
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		for (;;)
		{
			fread(magic, 1, 4, wav_file->file);
			if (!read_le_long(wav_file->file, &len))
			{
				fclose(wav_file->file);
				g_free(wav_file);
				wav_file = NULL;
				return;
			}
			if (!strncmp("fmt ", magic, 4))
				break;
			fseek(wav_file->file, len, SEEK_CUR);
		}
		if (len < 16)
		{
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		read_le_short(wav_file->file, &wav_file->format_tag);
		switch (wav_file->format_tag)
		{
			case WAVE_FORMAT_UNKNOWN:
			case WAVE_FORMAT_ALAW:
			case WAVE_FORMAT_MULAW:
			case WAVE_FORMAT_ADPCM:
			case WAVE_FORMAT_OKI_ADPCM:
			case WAVE_FORMAT_DIGISTD:
			case WAVE_FORMAT_DIGIFIX:
			case IBM_FORMAT_MULAW:
			case IBM_FORMAT_ALAW:
			case IBM_FORMAT_ADPCM:
				fclose(wav_file->file);
				g_free(wav_file);
				wav_file = NULL;
				return;
		}
		read_le_short(wav_file->file, &wav_file->channels);
		read_le_long(wav_file->file, &wav_file->samples_per_sec);
		read_le_long(wav_file->file, &wav_file->avg_bytes_per_sec);
		read_le_short(wav_file->file, &wav_file->block_align);
		read_le_short(wav_file->file, &wav_file->bits_per_sample);
		if (wav_file->bits_per_sample != 8 && wav_file->bits_per_sample != 16)
		{
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		len -= 16;
		if (len)
			fseek(wav_file->file, len, SEEK_CUR);

		for (;;)
		{
			fread(magic, 4, 1, wav_file->file);

			if (!read_le_long(wav_file->file, &len))
			{
				fclose(wav_file->file);
				g_free(wav_file);
				wav_file = NULL;
				return;
			}
			if (!strncmp("data", magic, 4))
				break;
			fseek(wav_file->file, len, SEEK_CUR);
		}
		wav_file->data_offset = ftell(wav_file->file);
		wav_file->length = len;

		wav_file->position = 0;
		wav_file->going = 1;

		if (wav_ip.output->open_audio((wav_file->bits_per_sample == 16) ? FMT_S16_LE : FMT_U8, wav_file->samples_per_sec, wav_file->channels) == 0)
		{
			audio_error = TRUE;
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		name = get_title(filename);
		rate = wav_file->samples_per_sec * wav_file->channels * (wav_file->bits_per_sample / 8);
		wav_ip.set_info(name, 1000 * (wav_file->length / rate), 8 * rate, wav_file->samples_per_sec, wav_file->channels);
		g_free(name);
		wav_file->seek_to = -1;
		pthread_create(&decode_thread, NULL, play_loop, NULL);
	}
}

static void stop(void)
{
	if (wav_file)
	{
		if (wav_file->going)
		{
			wav_file->going = 0;
			pthread_join(decode_thread, NULL);
			wav_ip.output->close_audio();
			g_free(wav_file);
			wav_file = NULL;

		}
	}
}

static void wav_pause(short p)
{
	wav_ip.output->pause(p);
}

static void seek(int time)
{
	wav_file->seek_to = time;

	wav_file->eof = FALSE;

	while (wav_file->seek_to != -1)
		xmms_usleep(10000);
}

static int get_time(void)
{
	if (audio_error)
		return -2;
	if (!wav_file)
		return -1;
	if (!wav_file->going || (wav_file->eof && !wav_ip.output->buffer_playing()))
		return -1;
	else
	{
		return wav_ip.output->output_time();
	}
}

static void get_song_info(char *filename, char **title, int *length)
{
	char magic[4];
	unsigned long len;
	int rate;
	WaveFile *wav_file;

	wav_file = g_malloc(sizeof (WaveFile));
	memset(wav_file, 0, sizeof (WaveFile));
	if (!(wav_file->file = fopen(filename, "rb")))
		return;

	fread(magic, 1, 4, wav_file->file);
	if (strncmp(magic, "RIFF", 4))
	{
		fclose(wav_file->file);
		g_free(wav_file);
		wav_file = NULL;
		return;
	}
	read_le_long(wav_file->file, &len);
	fread(magic, 1, 4, wav_file->file);
	if (strncmp(magic, "WAVE", 4))
	{
		fclose(wav_file->file);
		g_free(wav_file);
		wav_file = NULL;
		return;
	}
	for (;;)
	{
		fread(magic, 1, 4, wav_file->file);
		if (!read_le_long(wav_file->file, &len))
		{
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		if (!strncmp("fmt ", magic, 4))
			break;
		fseek(wav_file->file, len, SEEK_CUR);
	}
	if (len < 16)
	{
		fclose(wav_file->file);
		g_free(wav_file);
		wav_file = NULL;
		return;
	}
	read_le_short(wav_file->file, &wav_file->format_tag);
	switch (wav_file->format_tag)
	{
		case WAVE_FORMAT_UNKNOWN:
		case WAVE_FORMAT_ALAW:
		case WAVE_FORMAT_MULAW:
		case WAVE_FORMAT_ADPCM:
		case WAVE_FORMAT_OKI_ADPCM:
		case WAVE_FORMAT_DIGISTD:
		case WAVE_FORMAT_DIGIFIX:
		case IBM_FORMAT_MULAW:
		case IBM_FORMAT_ALAW:
		case IBM_FORMAT_ADPCM:
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
	}
	read_le_short(wav_file->file, &wav_file->channels);
	read_le_long(wav_file->file, &wav_file->samples_per_sec);
	read_le_long(wav_file->file, &wav_file->avg_bytes_per_sec);
	read_le_short(wav_file->file, &wav_file->block_align);
	read_le_short(wav_file->file, &wav_file->bits_per_sample);
	if (wav_file->bits_per_sample != 8 && wav_file->bits_per_sample != 16)
	{
		fclose(wav_file->file);
		g_free(wav_file);
		wav_file = NULL;
		return;
	}
	len -= 16;
	if (len)
		fseek(wav_file->file, len, SEEK_CUR);

	for (;;)
	{
		fread(magic, 4, 1, wav_file->file);

		if (!read_le_long(wav_file->file, &len))
		{
			fclose(wav_file->file);
			g_free(wav_file);
			wav_file = NULL;
			return;
		}
		if (!strncmp("data", magic, 4))
			break;
		fseek(wav_file->file, len, SEEK_CUR);
	}
	rate = wav_file->samples_per_sec * wav_file->channels * (wav_file->bits_per_sample / 8);
	(*length) = 1000 * (len / rate);
	(*title) = get_title(filename);

	fclose(wav_file->file);
	g_free(wav_file);
	wav_file = NULL;
}
