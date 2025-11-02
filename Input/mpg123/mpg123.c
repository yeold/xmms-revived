#include "mpg123.h"
#include "libxmms/configfile.h"
#include "libxmms/titlestring.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

static const long outscale = 32768;

static struct frame fr, temp_fr;

PlayerInfo *mpg123_info = NULL;
static pthread_t decode_thread;

static gboolean audio_error = FALSE, output_opened = FALSE, dopause = FALSE;
gint mpg123_bitrate, mpg123_frequency, mpg123_length, mpg123_layer, mpg123_lsf;
gchar *mpg123_title = NULL, *mpg123_filename = NULL;
static gint disp_bitrate, skip_frames = 0;
gboolean mpg123_stereo, mpg123_mpeg25;
gint mpg123_mode;

const gchar *mpg123_id3_genres[GENRE_MAX] =
{
	N_("Blues"), N_("Classic Rock"), N_("Country"), N_("Dance"),
	N_("Disco"), N_("Funk"), N_("Grunge"), N_("Hip-Hop"),
	N_("Jazz"), N_("Metal"), N_("New Age"), N_("Oldies"),
	N_("Other"), N_("Pop"), N_("R&B"), N_("Rap"), N_("Reggae"),
	N_("Rock"), N_("Techno"), N_("Industrial"), N_("Alternative"),
	N_("Ska"), N_("Death Metal"), N_("Pranks"), N_("Soundtrack"),
	N_("Euro-Techno"), N_("Ambient"), N_("Trip-Hop"), N_("Vocal"),
	N_("Jazz+Funk"), N_("Fusion"), N_("Trance"), N_("Classical"),
	N_("Instrumental"), N_("Acid"), N_("House"), N_("Game"),
	N_("Sound Clip"), N_("Gospel"), N_("Noise"), N_("Alt"),
	N_("Bass"), N_("Soul"), N_("Punk"), N_("Space"),
	N_("Meditative"), N_("Instrumental Pop"),
	N_("Instrumental Rock"), N_("Ethnic"), N_("Gothic"),
	N_("Darkwave"), N_("Techno-Industrial"), N_("Electronic"),
	N_("Pop-Folk"), N_("Eurodance"), N_("Dream"),
	N_("Southern Rock"), N_("Comedy"), N_("Cult"),
	N_("Gangsta Rap"), N_("Top 40"), N_("Christian Rap"),
	N_("Pop/Funk"), N_("Jungle"), N_("Native American"),
	N_("Cabaret"), N_("New Wave"), N_("Psychedelic"), N_("Rave"),
	N_("Showtunes"), N_("Trailer"), N_("Lo-Fi"), N_("Tribal"),
	N_("Acid Punk"), N_("Acid Jazz"), N_("Polka"), N_("Retro"),
	N_("Musical"), N_("Rock & Roll"), N_("Hard Rock"), N_("Folk"),
	N_("Folk/Rock"), N_("National Folk"), N_("Swing"),
	N_("Fast-Fusion"), N_("Bebob"), N_("Latin"), N_("Revival"),
	N_("Celtic"), N_("Bluegrass"), N_("Avantgarde"),
	N_("Gothic Rock"), N_("Progressive Rock"),
	N_("Psychedelic Rock"), N_("Symphonic Rock"), N_("Slow Rock"),
	N_("Big Band"), N_("Chorus"), N_("Easy Listening"),
	N_("Acoustic"), N_("Humour"), N_("Speech"), N_("Chanson"),
	N_("Opera"), N_("Chamber Music"), N_("Sonata"), N_("Symphony"),
	N_("Booty Bass"), N_("Primus"), N_("Porn Groove"),
	N_("Satire"), N_("Slow Jam"), N_("Club"), N_("Tango"),
	N_("Samba"), N_("Folklore"), N_("Ballad"), N_("Power Ballad"),
	N_("Rhythmic Soul"), N_("Freestyle"), N_("Duet"),
	N_("Punk Rock"), N_("Drum Solo"), N_("A Cappella"),
	N_("Euro-House"), N_("Dance Hall"), N_("Goa"),
	N_("Drum & Bass"), N_("Club-House"), N_("Hardcore"),
	N_("Terror"), N_("Indie"), N_("BritPop"), N_("Negerpunk"),
	N_("Polsk Punk"), N_("Beat"), N_("Christian Gangsta Rap"),
	N_("Heavy Metal"), N_("Black Metal"), N_("Crossover"),
	N_("Contemporary Christian"), N_("Christian Rock"),
	N_("Merengue"), N_("Salsa"), N_("Thrash Metal"),
	N_("Anime"), N_("JPop"), N_("Synthpop")
};

double mpg123_compute_tpf(struct frame *fr)
{
	const int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = bs[fr->lay];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->lsf);
	return tpf;
}

void set_mpg123_synth_functions(struct frame *fr)
{
	typedef int (*func) (real *, int, unsigned char *, int *);
	typedef int (*func_mono) (real *, unsigned char *, int *);
#ifdef USE_3DNOW
	typedef void (*func_dct36)(real *,real *,real *,real *,real *);
#endif
	int ds = fr->down_sample;
	int p8 = 0;

#ifdef USE_3DNOW
	static func funcs[3][4] =
	{
#else
	static func funcs[2][4] =
	{
#endif
		{mpg123_synth_1to1,
		 mpg123_synth_2to1,
		 mpg123_synth_4to1,
		 mpg123_synth_ntom},
		{mpg123_synth_1to1_8bit,
		 mpg123_synth_2to1_8bit,
		 mpg123_synth_4to1_8bit,
		 mpg123_synth_ntom_8bit}
#ifdef USE_3DNOW
  	       ,{ mpg123_synth_1to1_3dnow,
  		  mpg123_synth_2to1,
 		  mpg123_synth_4to1,
  		  mpg123_synth_ntom }
#endif
	};

	static func_mono funcs_mono[2][2][4] =
	{
		{
			{mpg123_synth_1to1_mono2stereo,
			 mpg123_synth_2to1_mono2stereo,
			 mpg123_synth_4to1_mono2stereo,
			 mpg123_synth_ntom_mono2stereo},
			{mpg123_synth_1to1_8bit_mono2stereo,
			 mpg123_synth_2to1_8bit_mono2stereo,
			 mpg123_synth_4to1_8bit_mono2stereo,
			 mpg123_synth_ntom_8bit_mono2stereo}},
		{
			{mpg123_synth_1to1_mono,
			 mpg123_synth_2to1_mono,
			 mpg123_synth_4to1_mono,
			 mpg123_synth_ntom_mono},
			{mpg123_synth_1to1_8bit_mono,
			 mpg123_synth_2to1_8bit_mono,
			 mpg123_synth_4to1_8bit_mono,
			 mpg123_synth_ntom_8bit_mono}}
	};

#ifdef USE_3DNOW	
	static func_dct36 funcs_dct36[2] = {dct36 , dct36_3dnow};
#endif

	if (mpg123_cfg.resolution == 8)
		p8 = 1;
	fr->synth = funcs[p8][ds];
	fr->synth_mono = funcs_mono[1][p8][ds];

#ifdef USE_3DNOW
       /* check cpuflags bit 31 (3DNow!) */
	if( (mpg123_cfg.use_3dnow < 2 ) && 
	    ((mpg123_cfg.use_3dnow == 1 ) ||
	     (support_3dnow() == TRUE)) &&
	    (mpg123_cfg.resolution != 8) )
	{
		fr->synth = funcs[2][ds]; /* 3DNow! optimized synth_1to1() */
		fr->dct36 = funcs_dct36[1]; /* 3DNow! optimized dct36() */
	}
	else
	{
		fr->dct36 = funcs_dct36[0];
	}
#endif
	if (p8)
	{
		mpg123_make_conv16to8_table();
	}
}

static void init(void)
{
	ConfigFile *cfg;

	mpg123_make_decode_tables(outscale);

	mpg123_cfg.resolution = 16;
	mpg123_cfg.channels = 2;
	mpg123_cfg.downsample = 0;
	mpg123_cfg.downsample_custom = 44100;
	mpg123_cfg.http_buffer_size = 128;
	mpg123_cfg.http_prebuffer = 25;
	mpg123_cfg.proxy_port = 8080;
	mpg123_cfg.proxy_use_auth = FALSE;
	mpg123_cfg.proxy_user = NULL;
	mpg123_cfg.proxy_pass = NULL;
	mpg123_cfg.cast_title_streaming = FALSE;
	mpg123_cfg.use_udp_channel = TRUE;
	mpg123_cfg.title_override = FALSE;
	mpg123_cfg.disable_id3v2 = FALSE;
	mpg123_cfg.detect_by_content = FALSE;
	mpg123_cfg.use_3dnow = 0;

	cfg = xmms_cfg_open_default_file();

	xmms_cfg_read_int(cfg, "MPG123", "resolution", &mpg123_cfg.resolution);
	xmms_cfg_read_int(cfg, "MPG123", "channels", &mpg123_cfg.channels);
	xmms_cfg_read_int(cfg, "MPG123", "downsample", &mpg123_cfg.downsample);
	/*xmms_cfg_read_int(cfg,"MPG123","downsample_custom",&mpg123_cfg.downsample_custom); */
	xmms_cfg_read_int(cfg, "MPG123", "http_buffer_size", &mpg123_cfg.http_buffer_size);
	xmms_cfg_read_int(cfg, "MPG123", "http_prebuffer", &mpg123_cfg.http_prebuffer);
	xmms_cfg_read_boolean(cfg, "MPG123", "save_http_stream", &mpg123_cfg.save_http_stream);
	if (!xmms_cfg_read_string(cfg, "MPG123", "save_http_path", &mpg123_cfg.save_http_path))
		mpg123_cfg.save_http_path = g_strdup(g_get_home_dir());
	xmms_cfg_read_boolean(cfg, "MPG123", "cast_title_streaming", &mpg123_cfg.cast_title_streaming);
	xmms_cfg_read_boolean(cfg, "MPG123", "use_udp_channel", &mpg123_cfg.use_udp_channel);

	xmms_cfg_read_boolean(cfg, "MPG123", "use_proxy", &mpg123_cfg.use_proxy);
	if (!xmms_cfg_read_string(cfg, "MPG123", "proxy_host", &mpg123_cfg.proxy_host))
		mpg123_cfg.proxy_host = g_strdup("localhost");
	xmms_cfg_read_int(cfg, "MPG123", "proxy_port", &mpg123_cfg.proxy_port);
	xmms_cfg_read_boolean(cfg, "MPG123", "proxy_use_auth", &mpg123_cfg.proxy_use_auth);
	xmms_cfg_read_string(cfg, "MPG123", "proxy_user", &mpg123_cfg.proxy_user);
	xmms_cfg_read_string(cfg, "MPG123", "proxy_pass", &mpg123_cfg.proxy_pass);

	xmms_cfg_read_boolean(cfg, "MPG123", "title_override", &mpg123_cfg.title_override);
	xmms_cfg_read_boolean(cfg, "MPG123", "disable_id3v2", &mpg123_cfg.disable_id3v2);
	if (!xmms_cfg_read_string(cfg, "MPG123", "id3_format", &mpg123_cfg.id3_format))
		mpg123_cfg.id3_format = g_strdup("%p - %t");
	xmms_cfg_read_boolean(cfg, "MPG123", "detect_by_content", &mpg123_cfg.detect_by_content);
	xmms_cfg_read_int(cfg, "MPG123", "use_3dnow", &mpg123_cfg.use_3dnow);

	xmms_cfg_free(cfg);
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
			{
/* we'll be looking for the fmt-chunk which comes before the data-chunk */
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
					/* we could go on looking for
                                           other things, but all we
                                           wanted was the wavid */
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

#define DET_BUF_SIZE 1024

static gboolean mpg123_detect_by_content(gchar *filename)
{
	FILE *file;
	guchar tmp[4];
	guint32 head;
	struct frame fr;
	guchar buf[DET_BUF_SIZE];
	gint in_buf, i;

	if((file = fopen(filename, "rb")) == NULL)
		return FALSE;
	if (fread(tmp, 1, 4, file) != 4)
		goto done;
	head = convert_to_header(tmp);
	while(!mpg123_head_check(head))
	{
		/*
		 * The mpeg-stream can start anywhere in the file,
		 * so we check the entire file
		 */
		/* Optimize this */
		in_buf = fread(buf, 1, DET_BUF_SIZE, file);
		if(in_buf == 0)
			goto done;

		for (i = 0; i < in_buf; i++)
		{
			head <<= 8;
			head |= buf[i];
			if(mpg123_head_check(head))
			{
				fseek(file, i+1-in_buf, SEEK_CUR);
				break;
			}
		}
	}
	if (mpg123_decode_header(&fr, head))
	{
		/*
		 * We found something which looks like a MPEG-header.
		 * We check the next frame too, to be sure
		 */
		
		if (fseek(file, fr.framesize, SEEK_CUR) != 0)
			goto done;
		if (fread(tmp, 1, 4, file) != 4)
			goto done;
		head = convert_to_header(tmp);
		if (mpg123_head_check(head) && mpg123_decode_header(&fr, head))
		{
			fclose(file);
			return TRUE;
		}
	}

 done:
	fclose(file);
	return FALSE;
}

static int is_our_file(char *filename)
{
	char *ext;
	guint16 wavid;

	if (!strncasecmp(filename, "http://", 7))
	{			/* We assume all http:// (except those ending in .ogg) are mpeg -- why do we do that? */
		ext = strrchr(filename, '.');
		if (ext) 
		{
			if (!strncasecmp(ext, ".ogg", 4)) 
				return FALSE;
			if (!strncasecmp(ext, ".rm", 3) || 
			    !strncasecmp(ext, ".ra", 3)  ||
			    !strncasecmp(ext, ".rpm", 4)  ||
			    !strncasecmp(ext, ".ram", 4))
				return FALSE;
		}
		return TRUE;
	}
	if(mpg123_cfg.detect_by_content)
		return (mpg123_detect_by_content(filename));
	ext = strrchr(filename, '.');
	if (ext)
	{
		if (!strncasecmp(ext, ".mp2", 4) || !strncasecmp(ext, ".mp3", 4))
		{
			return TRUE;
		}
		if (!strncasecmp(ext, ".wav", 4))
		{
			wavid = read_wav_id(filename);
			if (wavid == 85 || wavid == 80)
			{	/* Microsoft says 80, files say 85... */
				return TRUE;
			}
		}
	}
	return FALSE;
}

static void play_frame(struct frame *fr)
{
	if (fr->error_protection)
	{
		bsi.wordpointer += 2;
		/*  mpg123_getbits(16); */	/* skip crc */
	}
	if(!fr->do_layer(fr))
	{
		skip_frames = 2;
		mpg123_info->output_audio = FALSE;
	}
	else
	{
		if(!skip_frames)
			mpg123_info->output_audio = TRUE;
		else
			skip_frames--;
	}
}

static const char *get_id3_genre(unsigned char genre_code)
{
	if (genre_code < GENRE_MAX)
		return gettext(mpg123_id3_genres[genre_code]);

	return "";
}

guint mpg123_strip_spaces(char *src, size_t n)
/* strips trailing spaces from string of length n
   returns length of adjusted string */
{
	gchar *space = NULL,	/* last space in src */
	     *start = src;

	while (n--)
		switch (*src++)
		{
			case '\0':
				n = 0;	/* breaks out of while loop */

				src--;
				break;
			case ' ':
				if (space == NULL)
					space = src - 1;
				break;
			default:
				space = NULL;	/* don't terminate intermediate spaces */

				break;
		}
	if (space != NULL)
	{
		src = space;
		*src = '\0';
	}
	return src - start;
}

/*
 * Function extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
static gchar *extname(const char *filename)
{
	gchar *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

/*
 * Function id3v1_to_id3v2 (v1, v2)
 *
 *    Convert ID3v1 tag `v1' to ID3v2 tag `v2'.
 *
 */
void mpg123_id3v1_to_id3v2(struct id3v1tag_t *v1, struct id3tag_t *v2)
{
	memset(v2,0,sizeof(struct id3tag_t));
	strncpy(v2->title, v1->title, 30);
	strncpy(v2->artist, v1->artist, 30);
	strncpy(v2->album, v1->album, 30);
	strncpy(v2->comment, v1->u.v1_0.comment, 30);
	strncpy(v2->genre, get_id3_genre(v1->genre), sizeof (v2->genre));
	g_strstrip(v2->title);
	g_strstrip(v2->artist);
	g_strstrip(v2->album);
	g_strstrip(v2->comment);
	g_strstrip(v2->genre);
	v2->year = atoi(v1->year);

	/* Check for v1.1 tags. */
	if (v1->u.v1_1.__zero == 0)
		v2->track_number = v1->u.v1_1.track_number;
	else
		v2->track_number = 0;
}

static char* mpg123_getstr(char* str)
{
	if (str && strlen(str) > 0)
		return str;
	return NULL;
}

/*
 * Function mpg123_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
gchar *mpg123_format_song_title(struct id3tag_t *tag, gchar * filename)
{
	gchar *ret = NULL;
	TitleInput *input;

	XMMS_NEW_TITLEINPUT(input);

	if (tag)
	{
		input->performer = mpg123_getstr(tag->artist);
		input->album_name = mpg123_getstr(tag->album);
		input->track_name = mpg123_getstr(tag->title);
		input->year = tag->year;
		input->track_number = tag->track_number;
		input->genre = mpg123_getstr(tag->genre);
		input->comment = mpg123_getstr(tag->comment);
	}
	input->file_name = g_basename(filename);
	input->file_path = filename;
	input->file_ext = extname(filename);
	ret = xmms_get_titlestring(mpg123_cfg.title_override ?
				   mpg123_cfg.id3_format :
				   xmms_get_gentitle_format(), input);
	g_free(input);

	if (!ret)
	{
		/*
		 * Format according to filename.
		 */
		ret = g_strdup(g_basename(filename));
		if (extname(ret) != NULL)
			*(extname(ret) - 1) = '\0';	/* removes period */
	}

	return ret;
}

/*
 * Function mpg123_get_id3v2 (id3d, tag)
 *
 *    Get desired contents from the indicated id3tag and store it in
 *    `tag'. 
 *
 */
void mpg123_get_id3v2(id3_t * id3d, struct id3tag_t *tag)
{
	id3_frame_t *id3frm;
	gchar *txt;
	gint tlen, num;

#define ID3_SET(_tid,_fld) 						\
{									\
	id3frm = id3_get_frame( id3d, _tid, 1 );			\
	if (id3frm) {							\
		txt = _tid == ID3_TCON ? id3_get_content(id3frm)	\
		    : id3_get_text(id3frm);				\
		if(txt)							\
		{							\
			tlen = strlen(txt);				\
			if ( tlen >= sizeof(tag->_fld) ) 		\
				tlen = sizeof(tag->_fld)-1;		\
			strncpy( tag->_fld, txt, tlen );		\
			tag->_fld[tlen] = 0;				\
		}							\
		else							\
			tag->_fld[0] = 0;				\
	} else {							\
		tag->_fld[0] = 0;					\
	}								\
}

#define ID3_SET_NUM(_tid,_fld)				\
{							\
	id3frm = id3_get_frame(id3d, _tid, 1);		\
	if (id3frm) {					\
		num = id3_get_text_number(id3frm);	\
		tag->_fld = num >= 0 ? num : 0;		\
	} else						\
		tag->_fld = 0;				\
}

	ID3_SET		(ID3_TIT2, title);
	ID3_SET		(ID3_TPE1, artist);
	if (strlen(tag->artist) == 0)
		ID3_SET(ID3_TPE2, artist);
	ID3_SET		(ID3_TALB, album);
	ID3_SET_NUM	(ID3_TYER, year);
	ID3_SET_NUM	(ID3_TRCK, track_number);
	ID3_SET		(ID3_TXXX, comment);
	ID3_SET		(ID3_TCON, genre);
}

/*
 * Function get_song_title (fd, filename)
 *
 *    Get song title of file.  File position of `fd' will be
 *    clobbered.  `fd' may be NULL, in which case `filename' is opened
 *    separately.  The returned song title must be subsequently freed
 *    using g_free().
 *
 */
static gchar *get_song_title(FILE * fd, char *filename)
{
	FILE *file = fd;
	char *ret = NULL;
	struct id3v1tag_t id3v1tag;
	struct id3tag_t id3tag;

	if (file || (file = fopen(filename, "rb")) != 0)
	{
		id3_t *id3 = NULL;

		/*
		 * Try reading ID3v2 tag.
		 */
		if (!mpg123_cfg.disable_id3v2)
		{
			fseek(file, 0, SEEK_SET);
			id3 = id3_open_fp(file, 0);
			if (id3)
			{
				mpg123_get_id3v2(id3, &id3tag);
				ret = mpg123_format_song_title(&id3tag, filename);
				id3_close(id3);
			}
		}

		/*
		 * Try reading ID3v1 tag.
		 */
		if (!id3 && (fseek(file, -1 * sizeof (id3v1tag), SEEK_END) == 0) &&
		    (fread(&id3v1tag, 1, sizeof (id3v1tag), file) == sizeof (id3v1tag)) &&
		    (strncmp(id3v1tag.tag, "TAG", 3) == 0))
		{
			mpg123_id3v1_to_id3v2(&id3v1tag, &id3tag);
			ret = mpg123_format_song_title(&id3tag, filename);
		}

		if (!fd)
			/*
			 * File was opened in this function.
			 */
			fclose(file);
	}

	if (ret == NULL)
		/*
		 * Unable to get ID3 tag.
		 */
		ret = mpg123_format_song_title(NULL, filename);

	return ret;
}

static guint get_song_time(FILE * file)
{
	guint32 head;
	guchar tmp[4], *buf;
	struct frame frm;
	XHEADDATA xing_header;
	double tpf, bpf;
	guint32 len;

	if (!file)
		return -1;

	fseek(file, 0, SEEK_SET);
	if (fread(tmp, 1, 4, file) != 4)
		return 0;
	head = convert_to_header(tmp);
	while (!mpg123_head_check(head))
	{
		head <<= 8;
		if (fread(tmp, 1, 1, file) != 1)
			return 0;
		head |= tmp[0];
	}
	if (mpg123_decode_header(&frm, head))
	{
		buf = g_malloc(frm.framesize + 4);
		fseek(file, -4, SEEK_CUR);
		fread(buf, 1, frm.framesize + 4, file);
		xing_header.toc = NULL;
		tpf = mpg123_compute_tpf(&frm);
		if (mpg123_get_xing_header(&xing_header, buf))
		{
			g_free(buf);
			return ((guint) (tpf * xing_header.frames * 1000));
		}
		g_free(buf);
		bpf = mpg123_compute_bpf(&frm);
		fseek(file, 0, SEEK_END);
		len = ftell(file);
		fseek(file, -128, SEEK_END);
		fread(tmp, 1, 3, file);
		if (!strncmp(tmp, "TAG", 3))
			len -= 128;
		return ((guint) ((guint)(len / bpf) * tpf * 1000));
	}
	return 0;
}

static void get_song_info(char *filename, char **title_real, int *len_real)
{
	FILE *file;

	(*len_real) = -1;
	(*title_real) = NULL;

	/*
	 * TODO: Getting song info from http streams.
	 */
	if (strncasecmp(filename, "http://", 7))
	{
		if ((file = fopen(filename, "rb")) != NULL)
		{
			(*len_real) = get_song_time(file);
			(*title_real) = get_song_title(file, filename);
			fclose(file);
		}
	}
}

static int open_output(void)
{
	int r;
	AFormat fmt = mpg123_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8;
	int freq = mpg123_freqs[fr.sampling_frequency] >> mpg123_cfg.downsample;
	int channels = mpg123_cfg.channels == 2 ? fr.stereo : 1;
	r = mpg123_ip.output->open_audio(fmt, freq, channels);

	if (r && dopause)
	{
		mpg123_ip.output->pause(TRUE);
		dopause = FALSE;
	}

	return r;
}

static void *decode_loop(void *arg)
{
	gboolean have_xing_header = FALSE, vbr = FALSE;
	gint disp_count = 0, temp_time;
	gchar *filename = arg;
	XHEADDATA xing_header;
	unsigned char xing_toc[100];

	/* This is used by fileinfo on http streams */
	mpg123_bitrate = 0;

	mpg123_pcm_sample = g_malloc0(32768);
	mpg123_pcm_point = 0;
	mpg123_filename = filename;

	mpg123_read_frame_init();

	mpg123_open_stream(filename, -1);
	if (mpg123_info->eof || !mpg123_read_frame(&fr))
		mpg123_info->eof = TRUE;
	if (!mpg123_info->eof && mpg123_info->going)
	{
		if (mpg123_cfg.channels == 2)
			fr.single = -1;
		else
			fr.single = 3;

		fr.down_sample = mpg123_cfg.downsample;
		fr.down_sample_sblimit = SBLIMIT >> mpg123_cfg.downsample;
		set_mpg123_synth_functions(&fr);
		mpg123_init_layer3(fr.down_sample_sblimit);

		mpg123_info->tpf = mpg123_compute_tpf(&fr);
		if (strncasecmp(filename, "http://", 7))
		{
			xing_header.toc = xing_toc;
			if (mpg123_stream_check_for_xing_header(&fr, &xing_header))
			{
				mpg123_info->num_frames = xing_header.frames;
				have_xing_header = TRUE;
				mpg123_read_frame(&fr);
			}
		}

		for(;;)
		{
			memcpy(&temp_fr,&fr,sizeof(struct frame));
			if (!mpg123_read_frame(&temp_fr))
			{
				mpg123_info->eof = TRUE;
				break;
			}
			if (fr.lay != temp_fr.lay ||
			    fr.sampling_frequency != temp_fr.sampling_frequency ||
	        	    fr.stereo != temp_fr.stereo || fr.lsf != temp_fr.lsf)
				memcpy(&fr,&temp_fr,sizeof(struct frame));
			else
				break;
		}
		if(!have_xing_header && strncasecmp(filename, "http://", 7))
		{
			mpg123_info->num_frames = mpg123_calc_numframes(&fr);
		}

		memcpy(&fr,&temp_fr,sizeof(struct frame));
		mpg123_bitrate = disp_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
		mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
		mpg123_stereo = fr.stereo;
		mpg123_layer = fr.lay;
		mpg123_lsf = fr.lsf;
		mpg123_mpeg25 = fr.mpeg25;
		mpg123_mode = fr.mode;

		if (strncasecmp(filename, "http://", 7))
		{
			mpg123_length = mpg123_info->num_frames * mpg123_info->tpf * 1000;
			if (!mpg123_title)
				mpg123_title = get_song_title(NULL,filename);
		}
		else
		{
			if (!mpg123_title)
				mpg123_title = mpg123_http_get_title(filename);
			mpg123_length = -1;
		}
		mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_freqs[fr.sampling_frequency], fr.stereo);
		output_opened = TRUE;
		if (!open_output())
		{
			audio_error = TRUE;
			mpg123_info->eof = TRUE;
		}
		else
			play_frame(&fr);
	}

	mpg123_info->first_frame = FALSE;
	while (mpg123_info->going)
	{
		if (mpg123_info->jump_to_time != -1)
		{
			int jumped = -1;

			if (have_xing_header)
				jumped = mpg123_stream_jump_to_byte(&fr, mpg123_seek_point(xing_toc, xing_header.bytes, ((double) mpg123_info->jump_to_time * 100.0) / ((double) mpg123_info->num_frames * mpg123_info->tpf)));
			else if (vbr && mpg123_length > 0)
				jumped = mpg123_stream_jump_to_byte(&fr, ((guint64)mpg123_info->jump_to_time * 1000 * mpg123_info->filesize) / mpg123_length);
			else
				jumped = mpg123_stream_jump_to_frame(&fr, mpg123_info->jump_to_time / mpg123_info->tpf);

			if (jumped > -1)
			{
				mpg123_ip.output->flush(mpg123_info->jump_to_time * 1000);
				mpg123_info->eof = FALSE;
			}
			mpg123_info->jump_to_time = -1;

		}
		if (!mpg123_info->eof)
		{
			if (mpg123_read_frame(&fr) != 0)
			{
				if(fr.lay != mpg123_layer || fr.lsf != mpg123_lsf)
				{
					memcpy(&temp_fr,&fr,sizeof(struct frame));
					if(mpg123_read_frame(&temp_fr) != 0)
					{
						if(fr.lay == temp_fr.lay && fr.lsf == temp_fr.lsf)
						{
							mpg123_layer = fr.lay;
							mpg123_lsf = fr.lsf;
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							set_mpg123_synth_functions(&fr);
						}
						else
						{
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							skip_frames = 2;
							mpg123_info->output_audio = FALSE;
							continue;
						}
						
					}
				}
				if(mpg123_freqs[fr.sampling_frequency] != mpg123_frequency || mpg123_stereo != fr.stereo)
				{
					memcpy(&temp_fr,&fr,sizeof(struct frame));
					if(mpg123_read_frame(&temp_fr) != 0)
					{
						if(fr.sampling_frequency == temp_fr.sampling_frequency && temp_fr.stereo == fr.stereo)
						{
							mpg123_ip.output->buffer_free();
							mpg123_ip.output->buffer_free();
							while(mpg123_ip.output->buffer_playing() && mpg123_info->going && mpg123_info->jump_to_time == -1)
								xmms_usleep(20000);
							if(!mpg123_info->going)
								break;
							temp_time = mpg123_ip.output->output_time();
							mpg123_ip.output->close_audio();
							mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
							mpg123_stereo = fr.stereo;
							if (!mpg123_ip.output->open_audio(mpg123_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8,
								mpg123_freqs[fr.sampling_frequency] >> mpg123_cfg.downsample,
				  				mpg123_cfg.channels == 2 ? fr.stereo : 1))
							{
								audio_error = TRUE;
								mpg123_info->eof = TRUE;
							}
							mpg123_ip.output->flush(temp_time);
							mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_frequency, mpg123_stereo);
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							set_mpg123_synth_functions(&fr);
						}
						else
						{
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							skip_frames = 2;
							mpg123_info->output_audio = FALSE;
							continue;
						}
					}					
				}
				
				if (tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index] != mpg123_bitrate)
					mpg123_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
				
				if (!disp_count)
				{
					disp_count = 20;
					if (mpg123_bitrate != disp_bitrate)
					{
						disp_bitrate = mpg123_bitrate;
						if(!have_xing_header && strncasecmp(filename,"http://",7))
						{
							double rel = mpg123_relative_pos();
							if (rel)
							{
								mpg123_length = mpg123_ip.output->written_time() / rel;
								vbr = TRUE;
							}

							if (rel == 0 || !(mpg123_length > 0))
							{
								mpg123_info->num_frames = mpg123_calc_numframes(&fr);
								mpg123_info->tpf = mpg123_compute_tpf(&fr);
								mpg123_length = mpg123_info->num_frames * mpg123_info->tpf * 1000;
							}


						}
						mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_frequency, mpg123_stereo);
					}
				}
				else
					disp_count--;
				play_frame(&fr);
			}
			else
			{
				mpg123_ip.output->buffer_free();
				mpg123_ip.output->buffer_free();
				mpg123_info->eof = TRUE;
				xmms_usleep(10000);
			}
		}
		else
		{
			xmms_usleep(10000);
		}
	}
	g_free(mpg123_title);
	mpg123_title = NULL;
	mpg123_stream_close();
	if (output_opened && !audio_error)
		mpg123_ip.output->close_audio();
	g_free(mpg123_pcm_sample);
	mpg123_filename = NULL;
	g_free(filename);
	pthread_exit(NULL);
}

static void play_file(char *filename)
{
	memset(&fr, 0, sizeof (struct frame));
	memset(&temp_fr, 0, sizeof (struct frame));

	mpg123_info = g_malloc0(sizeof (PlayerInfo));
	mpg123_info->going = 1;
	mpg123_info->first_frame = TRUE;
	mpg123_info->output_audio = TRUE;
	mpg123_info->jump_to_time = -1;
	skip_frames = 0;
	audio_error = FALSE;
	output_opened = FALSE;
	dopause = FALSE;

	pthread_create(&decode_thread, NULL, decode_loop, g_strdup(filename));
}

static void stop(void)
{
	if (mpg123_info && mpg123_info->going)
	{
		mpg123_info->going = FALSE;
		pthread_join(decode_thread, NULL);
		g_free(mpg123_info);
		mpg123_info = NULL;
	}
}

static void seek(int time)
{
	mpg123_info->jump_to_time = time;

	while (mpg123_info->jump_to_time != -1)
		xmms_usleep(10000);
}

static void do_pause(short p)
{
	if (output_opened)
		mpg123_ip.output->pause(p);
	else
		dopause = p;
}

static int get_time(void)
{
	if (audio_error)
		return -2;
	if (!mpg123_info)
		return -1;
	if (!mpg123_info->going || (mpg123_info->eof && !mpg123_ip.output->buffer_playing()))
		return -1;
	return mpg123_ip.output->output_time();
}

static void aboutbox(void)
{
	static GtkWidget *aboutbox;

	if (aboutbox != NULL)
		return;
	
	aboutbox = xmms_show_message(
		_("About MPEG Layer 1/2/3 plugin"),
		_("mpg123 decoding engine by Michael Hipp <mh@mpg123.de>\n"
		  "Plugin by The XMMS team"),
		_("Ok"), FALSE, NULL, NULL);

	gtk_signal_connect(GTK_OBJECT(aboutbox), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &aboutbox);
}

InputPlugin mpg123_ip =
{
	NULL,
	NULL,
	NULL, /* Description */
	init,
	aboutbox,
	mpg123_configure,
	is_our_file,
	NULL,
	play_file,
	stop,
	do_pause,
	seek,
	mpg123_set_eq,
	get_time,
	NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	get_song_info,
	mpg123_file_info_box,	/* file_info_box */
	NULL
};

InputPlugin *get_iplugin_info(void)
{
	mpg123_ip.description =
		g_strdup_printf(_("MPEG Layer 1/2/3 Player %s"), VERSION);
	return &mpg123_ip;
}
