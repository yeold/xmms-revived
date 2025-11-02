/* -------------------------------------------------------------------------
 *           Id Software Cinematic Player
 *
 *       Plugin is written by: Tim Ferguson, 1999.
 *
 * An attempt to incorporate video playback into what is primarily designed
 * as an audio only player :)
 *
 * This plugin plays an Id Software Quake II cinematic file (.cin).  If you
 * own an original copy of Quake II, these video files can be found in
 * /cdrom/Install/Data/baseq2/video/ *.cin.  In particular, ntro.cin and
 * end.cin work well.  There are also cinematic sequences on Quake II
 * mission packs 1 and 2, and some amateur sequences can be found on
 * ftp.cdrom.com.  I have tested the plugin with an astro.cin and a
 * cave.cin.  Please note there is a large delay before the sequence starts
 * playing, since a cinematic file does not have an index and must therefore
 * be scanned to create one.  The implementation of syncronised playback
 * is very crude.  It works on my Celeron @ 450 with a 24 bit display, but
 * I cannot guarantee the video will be synchronised or the audio wont drop
 * on slower systems.
 *
 * This cinematic file type is VERY simple as can be seen by inspecting the
 * code.  I chose this file format to see if it could be done using the xmms
 * frame work.  Potentially, any other video format could be replaced with
 * this one (mpeg, avi, mov, rm), or perhaps some form of video plugin
 * support could be added (with full screen playback would be nice).
 *
 * Currently the code seems to work, although it is a little unstable at
 * times.  Rapid seeking or file skipping can cause the plugin to hang.
 * ------------------------------------------------------------------------- */
#include "idcin.h"
#include "libxmms/util.h"
#include "config.h"
#include "xmms/i18n.h"

InputPlugin idcin_ip =
{
	NULL,
	NULL,
	NULL, /* Description */
	idcin_init,
	NULL,
	NULL,
	idcin_is_our_file,
	NULL,
	idcin_play_file,
	idcin_stop,
	idcin_pause,
	idcin_seek,
	NULL,
	idcin_get_time,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,			/* get_song_info */
	NULL,			/* file_info_box */
	NULL
};

CinFile cin;
static pthread_t idcin_decode_thread;
static GtkWidget *window, *drawing_area, *vbox;
#define HUF_TOKENS 256
static hnode_t huff_nodes[256][HUF_TOKENS*2];
static int num_huff_nodes[256];
static long cur_frame, aud_frame, seek_to;

/* -------------------------------------------------------------------------
 *  Decodes input Huffman data using the Huffman table.
 *
 *  Input:   data = encoded data to be decoded.
 *           len = length of encoded data.
 *           image = a buffer for the decoded image data.
 * ------------------------------------------------------------------------- */
void huff_decode(unsigned char *data, long len, unsigned char *image)
{
	hnode_t *hnodes;
	long i, dec_len;
	int prev;
	register unsigned int v = 0;
	register int bit_pos, node_num, dat_pos;
	idcmap_t *cmap = cin.frames[cur_frame].cmap;

		/* read count */
	dec_len  = (*data++ & 0xff);
	dec_len |= (*data++ & 0xff) << 8;
	dec_len |= (*data++ & 0xff) << 16;
	dec_len |= (*data++ & 0xff) << 24;

	prev = bit_pos = dat_pos = 0;
	for(i = 0; i < dec_len; i++)
	{
		node_num = num_huff_nodes[prev];
		hnodes = huff_nodes[prev];

		while(node_num >= HUF_TOKENS)
		{
			if(!bit_pos)
			{
				if(dat_pos > len)
				{
					printf(_("Huffman decode error.\n"));
					return;
				}
				bit_pos = 8;
				v = *data++;
				dat_pos++;
			}

			node_num = hnodes[node_num].children[v & 0x01];
			v >>= 1;
			bit_pos--;
		}

		*image++ = cmap->r_cmap[node_num];
		*image++ = cmap->g_cmap[node_num];
		*image++ = cmap->b_cmap[node_num];
		prev = node_num;
	}
}


/* -------------------------------------------------------------------------
 *  Find the lowest probability node in a Huffman table, and mark it as
 *  being assigned to a higher probability.
 *  Returns the node index of the lowest unused node, or -1 if all nodes
 *  are used.
 * ------------------------------------------------------------------------- */
int huff_smallest_node(hnode_t *hnodes, int num_hnodes)
{
	int i;
	int best, best_node;

	best = 99999999;
	best_node = -1;
	for(i = 0; i < num_hnodes; i++)
	{
		if(hnodes[i].used) continue;
		if(!hnodes[i].count) continue;
		if(hnodes[i].count < best)
		{
			best = hnodes[i].count;
			best_node = i;
		}
	}

	if(best_node == -1) return -1;
	hnodes[best_node].used = 1;
	return best_node;
}


/* -------------------------------------------------------------------------
 *  Build the Huffman tree using the generated/loaded probabilities histogram.
 *
 *  On completion:
 *   huff_nodes[prev][i < HUF_TOKENS] - are the nodes at the base of the tree.
 *   huff_nodes[prev][i >= HUF_TOKENS] - are used to construct the tree.
 *   num_huff_nodes[prev] - contains the index to the root node of the tree.
 *     That is: huff_nodes[prev][num_huff_nodes[prev]] is the root node.
 * ------------------------------------------------------------------------- */
void huff_build_tree(int prev)
{
hnode_t *node, *hnodes;
int num_hnodes, i;

	num_hnodes = HUF_TOKENS;
	hnodes = huff_nodes[prev];
	for(i = 0; i < HUF_TOKENS * 2; i++) hnodes[i].used = 0;

	while (1)
	{
		node = &hnodes[num_hnodes];		/* next free node */

			/* pick two lowest counts */
		node->children[0] = huff_smallest_node(hnodes, num_hnodes);
		if(node->children[0] == -1) break;	/* reached the root node */

		node->children[1] = huff_smallest_node(hnodes, num_hnodes);
		if(node->children[1] == -1) break;	/* reached the root node */

			/* combine nodes probability for new node */
		node->count = hnodes[node->children[0]].count + 
			hnodes[node->children[1]].count;
		num_hnodes++;
	}

	num_huff_nodes[prev] = num_hnodes - 1;
}


/* ---------------------------------------------------------------------- */
InputPlugin *get_iplugin_info(void)
{
	idcin_ip.description =
		g_strdup_printf(_("Id Software .cin player %s"), VERSION);
	return &idcin_ip;
}


/* ---------------------------------------------------------------------- */
static unsigned long read_long(FILE *fp)
{
	unsigned long ret;

	ret  = ((fgetc(fp)) & 0xff);
	ret |= ((fgetc(fp)) & 0xff) << 8;
	ret |= ((fgetc(fp)) & 0xff) << 16;
	ret |= ((fgetc(fp)) & 0xff) << 24;
	return(ret);
}


/* ---------------------------------------------------------------------- */
static int idcin_is_our_file(char *filename)
{
	gchar *ext;

	ext = strrchr(filename, '.');
	if(ext)
		if(!strcasecmp(ext, ".cin"))
			return TRUE;
	return FALSE;
}


/* ---------------------------------------------------------------------- */
static void idcin_init(void)
{
	cin.fp = NULL;
	cin.img_buf = NULL;
	cin.huff_data = NULL;
	cin.cmap = NULL;
}


/* ---------------------------------------------------------------------- */
static void idcin_video_frame(int frame)
{
	long command, huff_count, pos;

	pos = ftell(cin.fp);
	fseek(cin.fp, cin.frames[cur_frame].pos, SEEK_SET);

	command = read_long(cin.fp);
	if(command == 1)
		fseek(cin.fp, 256 * 3, SEEK_CUR);

	huff_count = read_long(cin.fp);
	fread(cin.huff_data, 1, huff_count, cin.fp);
	fseek(cin.fp, pos, SEEK_SET);

	huff_decode(cin.huff_data, huff_count, cin.img_buf);

	GDK_THREADS_ENTER();
	gdk_draw_rgb_image(drawing_area->window, drawing_area->style->white_gc,
			   0, 0, cin.width, cin.height, GDK_RGB_DITHER_NONE,
			   cin.img_buf, cin.width * 3);
	GDK_THREADS_LEAVE();
}


/* ---------------------------------------------------------------------- */
static void *idcin_play_loop(void *arg)
{
	char data[15000];
	long command, huff_count, snd_count, cnt;
	unsigned long last_frame;

	last_frame = -1;

	while(cin.going)
	{
		if(!cin.eof)
		{
					/* Add audio to buffer */
			cnt = snd_count = (((aud_frame+1) * cin.snd_rate/14) - (aud_frame * cin.snd_rate/14))
				* cin.snd_width * cin.snd_channels;

			command = read_long(cin.fp);
			if(command == 2) { cin.eof = TRUE; }
			else if(command == 1) fseek(cin.fp, 256 * 3, SEEK_CUR);

			if(!cin.eof)
			{
				huff_count = read_long(cin.fp);
				fseek(cin.fp, huff_count, SEEK_CUR);
				fread(data, snd_count, 1, cin.fp);
/*
				idcin_ip.add_vis_pcm(idcin_ip.output->written_time(), (cin.snd_width == 2) ? FMT_S16_LE : FMT_S8,
				   cin.snd_channels, snd_count, data);
*/
				while(idcin_ip.output->buffer_free() < cnt && cin.going && seek_to == -1)
					xmms_usleep(10000);

				if(cin.going && seek_to == -1)
					idcin_ip.output->write_audio(data, cnt);
			}
			aud_frame++;
		}
		else if(cin.going && seek_to == -1) xmms_usleep(10000);

				/* Check to see if video needs changing */
		cur_frame = (idcin_ip.output->output_time() * 14)/1000;

		if(cur_frame != last_frame && cur_frame < cin.num_frames && cin.going && seek_to == -1)
		{
					/* new frame needs reading, decoding and displaying */
			idcin_video_frame(cur_frame);
			last_frame = cur_frame;
		}

		if(seek_to != -1)
		{
			aud_frame = cur_frame = seek_to * 14;
			fseek(cin.fp, cin.frames[cur_frame].pos, SEEK_SET);
			idcin_ip.output->flush(seek_to * 1000);
			seek_to = -1;
		}
	}

	pthread_exit(NULL);
}


/* ---------------------------------------------------------------------- */
static void idcin_parse_file(CinFile *cin)
{
	int i, j, frame;
	long command, huff_count, snd_count;
	idcmap_t *cmap;

	cin->width = read_long(cin->fp);
	cin->height = read_long(cin->fp);
	cin->snd_rate = read_long(cin->fp);
	cin->snd_width = read_long(cin->fp);
	cin->snd_channels = read_long(cin->fp);

	g_free(cin->img_buf);
	cin->img_buf = (unsigned char *)
	    g_malloc(cin->width * cin->height * 3 + 1000);
	g_free(cin->huff_data);
	cin->huff_data = (unsigned char *)
	    g_malloc(cin->width * cin->height * 2 + 1024);

	for(i = 0; i < 256; i++)
	{
		for(j = 0; j < HUF_TOKENS; j++)
			huff_nodes[i][j].count = getc(cin->fp);
		huff_build_tree(i);
	}

	frame = 0;

	while(1)
	{
		snd_count = (((frame+1) * cin->snd_rate/14) - (frame * cin->snd_rate/14))
			* cin->snd_width * cin->snd_channels;
		cin->frames[frame].pos = ftell(cin->fp);

		command = read_long(cin->fp);
		if(command == 2) break;
		if(command == 1)
		{
			cmap = g_malloc(sizeof(idcmap_t));
			cmap->next = cin->cmap;
			cin->cmap = cmap;
			for(i = 0; i < 256; i++)
			{
				cin->cmap->r_cmap[i] = getc(cin->fp);
				cin->cmap->g_cmap[i] = getc(cin->fp);
				cin->cmap->b_cmap[i] = getc(cin->fp);
			}
		}

		cin->frames[frame].cmap = cin->cmap;

		huff_count = read_long(cin->fp);
		fseek(cin->fp, huff_count + snd_count, SEEK_CUR);
		frame++;
	}

	fseek(cin->fp, cin->frames[0].pos, SEEK_SET);

	cin->num_frames = frame;
}


/* ---------------------------------------------------------------------- */
static void idcin_play_file(char *filename)
{
	if(cin.fp != NULL) fclose(cin.fp);

	if((cin.fp = fopen(filename, "rb")) == NULL) return;

	idcin_parse_file(&cin);

	cin.going = TRUE;
	cin.eof = FALSE;
	aud_frame = cur_frame = 0;
	seek_to = -1;

	if(idcin_ip.output->open_audio((cin.snd_width == 2) ? FMT_S16_LE : FMT_S8, cin.snd_rate, cin.snd_channels) == 0)
	{
		printf(_("Error opening audio for idcin.\n"));
		fclose(cin.fp);
		cin.fp = NULL;
		return;
	}

	idcin_ip.set_info(NULL, (cin.num_frames * 1000)/14, cin.snd_rate * cin.snd_channels * cin.snd_width * 8, cin.snd_rate, cin.snd_channels);

	window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_widget_set_name(window, _("IDCin Play"));
	gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	drawing_area = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), cin.width, cin.height);
	gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
	gtk_widget_show(drawing_area);
	gtk_widget_show(window);

	pthread_create(&idcin_decode_thread, NULL, idcin_play_loop, NULL);
}


/* ---------------------------------------------------------------------- */
static void idcin_stop(void)
{
	idcmap_t *cmap;

	if(cin.going)
	{
		cin.going = 0;
		xmms_usleep(500000);
		pthread_join(idcin_decode_thread, NULL);
		idcin_ip.output->close_audio();
		fclose(cin.fp);
		cin.fp = NULL;
		gtk_widget_destroy(window);

			/* free colour map list */
		while(cin.cmap != NULL)
		{
			cmap = cin.cmap;
			cin.cmap = cin.cmap->next;
			g_free(cmap);
		}
	}
}


/* ---------------------------------------------------------------------- */
static void idcin_pause(short p)
{
	idcin_ip.output->pause(p);
}


/* ---------------------------------------------------------------------- */
static void idcin_seek(int time)
{
	seek_to = time;
	cin.eof = 0;
	while(seek_to != -1) xmms_usleep(10000);
}


/* ---------------------------------------------------------------------- */
static int idcin_get_time(void)
{
	if(!cin.going || (cin.eof && !idcin_ip.output->buffer_playing()))
		return -1;

	return (cur_frame * 1000)/14;
}
