/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Håvard Kvålen
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
#include "xmms.h"
#include "fft.h"
#include "libxmms/titlestring.h"

static pthread_mutex_t vis_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
	gint time;
	gint nch;
	gint length; /* number of samples per channel */
	gint16 data[2][512];
}
VisNode;


struct InputPluginData *ip_data;
static GList *vis_list = NULL;

gchar *input_info_text = NULL;
extern PlayStatus *mainwin_playstatus;

void input_add_vis_pcm(int time, AFormat fmt, int nch, int length, void *ptr);
InputVisType input_get_vis_type();
void input_set_info(char *t, int len, int r, int f, int nch);
void input_set_info_text(gchar * text);

InputPlugin *get_current_input_plugin(void)
{
	return ip_data->current_input_plugin;
}

void set_current_input_plugin(InputPlugin * ip)
{
	ip_data->current_input_plugin = ip;
}

GList *get_input_list(void)
{
	return ip_data->input_list;
}

static void free_vis_data(void)
{
	GList *node;
	pthread_mutex_lock(&vis_mutex);
	node = vis_list;
	while (node)
	{
		g_free(node->data);
		node = g_list_next(node);
	}
	g_list_free(vis_list);
	vis_list = NULL;
	pthread_mutex_unlock(&vis_mutex);
}

static void convert_to_s16_ne(AFormat fmt,gpointer ptr, gint16 *left, gint16 *right,gint nch,gint max)
{
	gint16 	*ptr16;
	guint16	*ptru16;
	guint8	*ptru8;
	gint	i;
	
	switch (fmt)
	{
		case FMT_U8:
			ptru8 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++)
					left[i] = ((*ptru8++) ^ 128) << 8;
			else
				for (i = 0; i < max; i++)
				{
					left[i] = ((*ptru8++) ^ 128) << 8;
					right[i] = ((*ptru8++) ^ 128) << 8;
				}
			break;
		case FMT_S8:
			ptru8 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++)
					left[i] = (*ptru8++) << 8;
			else
				for (i = 0; i < max; i++)
				{
					left[i] = (*ptru8++) << 8;
					right[i] = (*ptru8++) << 8;
				}
			break;
		case FMT_U16_LE:
			ptru16 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++, ptru16++)
					left[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
			else
				for (i = 0; i < max; i++)
				{
					left[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
					ptru16++;
					right[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
					ptru16++;
				}
			break;
		case FMT_U16_BE:
			ptru16 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++, ptru16++)
					left[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
			else
				for (i = 0; i < max; i++)
				{
					left[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
					ptru16++;
					right[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
					ptru16++;
				}
			break;
		case FMT_U16_NE:
			ptru16 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++)
					left[i] = (*ptru16++) ^ 32768;
			else
				for (i = 0; i < max; i++)
				{
					left[i] = (*ptru16++) ^ 32768;
					right[i] = (*ptru16++) ^ 32768;
				}
			break;
		case FMT_S16_LE:
			ptr16 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++, ptr16++)
					left[i] = GINT16_FROM_LE(*ptr16);
			else
				for (i = 0; i < max; i++)
				{
					left[i] = GINT16_FROM_LE(*ptr16);
					ptr16++;
					right[i] = GINT16_FROM_LE(*ptr16);
					ptr16++;
				}
			break;
		case FMT_S16_BE:
			ptr16 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++, ptr16++)
					left[i] = GINT16_FROM_BE(*ptr16);
			else
				for (i = 0; i < max; i++)
				{
					left[i] = GINT16_FROM_BE(*ptr16);
					ptr16++;
					right[i] = GINT16_FROM_BE(*ptr16);
					ptr16++;
				}
			break;
		case FMT_S16_NE:
			ptr16 = ptr;
			if (nch == 1)
				for (i = 0; i < max; i++)
					left[i] = (*ptr16++);
			else
				for (i = 0; i < max; i++)
				{
					left[i] = (*ptr16++);
					right[i] = (*ptr16++);
				}
			break;
	}
}

InputVisType input_get_vis_type()
{
	return INPUT_VIS_OFF;
}
	
void input_add_vis(int time, unsigned char *s, InputVisType type)
{

}

void input_add_vis_pcm(int time, AFormat fmt, int nch, int length, void *ptr)
{
	VisNode *vis_node;
	gint max;
	
	max = length / nch;
	if (fmt == FMT_U16_LE || fmt == FMT_U16_BE || fmt == FMT_U16_NE ||
	    fmt == FMT_S16_LE || fmt == FMT_S16_BE || fmt == FMT_S16_NE)
		max /= 2;
	max = CLAMP(max, 0, 512);

	vis_node = g_malloc0(sizeof(VisNode));
	vis_node->time = time;
	vis_node->nch = nch;
	vis_node->length = max;
	convert_to_s16_ne(fmt,ptr,vis_node->data[0],vis_node->data[1],nch,max);
	
	pthread_mutex_lock(&vis_mutex);
	vis_list = g_list_append(vis_list, vis_node);
	pthread_mutex_unlock(&vis_mutex);
}

gboolean input_check_file(gchar * filename)
{
	GList *node;
	InputPlugin *ip;

	node = get_input_list();
	while (node)
	{
		ip = (InputPlugin *) node->data;
		if (ip && !g_list_find(disabled_iplugins, ip) &&
		    ip->is_our_file(filename))
			return TRUE;
		node = node->next;
	}
	return FALSE;
}

void input_play(char *filename)
{
	GList *node;
	InputPlugin *ip;

	node = get_input_list();
	while (node)
	{
		ip = (InputPlugin *) node->data;
		if (ip && !g_list_find(disabled_iplugins, ip) &&
		    ip->is_our_file(filename))
		{
			set_current_input_plugin(ip);
			ip->output = get_current_output_plugin();
			ip->play_file(filename);
			ip_data->playing = TRUE;
			return;

		}
		node = node->next;
	}
	/* We set the playing flag even if no inputplugin
	   recognizes the file. This way we are sure it will be skipped. */
	ip_data->playing = TRUE;
	set_current_input_plugin(NULL);
}

void input_seek(int time)
{
	if (ip_data->playing && get_current_input_plugin())
	{
		free_vis_data();
		get_current_input_plugin()->seek(time);
	}
}

void input_stop(void)
{
	if (ip_data->playing && get_current_input_plugin())
	{
		ip_data->playing = FALSE;

		if (ip_data->paused)
			input_pause();
		if (get_current_input_plugin()->stop)
			get_current_input_plugin()->stop();

		free_vis_data();
		ip_data->paused = FALSE;
		if (input_info_text)
		{
			g_free(input_info_text);
			input_info_text = NULL;
			mainwin_set_info_text();
		}
	}
	ip_data->playing = FALSE;
}

void input_pause(void)
{
	if (get_input_playing() && get_current_input_plugin())
	{
		ip_data->paused = !ip_data->paused;
		if (ip_data->paused)
			playstatus_set_status(mainwin_playstatus, STATUS_PAUSE);
		else
			playstatus_set_status(mainwin_playstatus, STATUS_PLAY);
		get_current_input_plugin()->pause(ip_data->paused);
	}
}

gint input_get_time(void)
{
	gint time;

	if (get_input_playing() && get_current_input_plugin())
		time = get_current_input_plugin()->get_time();
	else
		time = -1;
	return time;
}

void input_set_eq(int on, float preamp, float *bands)
{
	if (ip_data->playing)
		if (get_current_input_plugin() && get_current_input_plugin()->set_eq)
			get_current_input_plugin()->set_eq(on, preamp, bands);
}

void input_get_song_info(gchar * filename, gchar ** title, gint * length)
{
	GList *node;
	InputPlugin *ip = NULL;

	node = get_input_list();
	while (node)
	{
		ip = (InputPlugin *) node->data;
		if (!g_list_find(disabled_iplugins, ip) &&
		    ip->is_our_file(filename))
			break;
		node = node->next;
	}
	if (ip && node && ip->get_song_info)
		ip->get_song_info(filename, title, length);
	else
	{
		gchar *temp, *ext;
		TitleInput *input;

		XMMS_NEW_TITLEINPUT(input);
		temp = g_strdup(filename);
		ext = strrchr(temp, '.');
		if (ext)
			*ext = '\0';
		input->file_name = g_basename(temp);
		input->file_ext = ext ? ext+1 : NULL;
		input->file_path = temp;

		(*title) = xmms_get_titlestring(xmms_get_gentitle_format(),
						input);
		if ( (*title) == NULL )
		    (*title) = g_strdup(input->file_name);
		(*length) = -1;
		g_free(temp);
		g_free(input);
	}
}

void input_file_info_box(gchar * filename)
{
	GList *node;
	InputPlugin *ip;

	node = get_input_list();
	while (node)
	{
		ip = (InputPlugin *) node->data;
		if (!g_list_find(disabled_iplugins, ip) && ip->is_our_file(filename))
		{
			if (ip->file_info_box)
				ip->file_info_box(filename);
			break;
		}
		node = node->next;
	}
}

GList *input_scan_dir(gchar * dir)
{
	GList *node, *ret = NULL;
	InputPlugin *ip;

	node = get_input_list();
	while (node && !ret)
	{
		ip = (InputPlugin *) node->data;
		if (ip && ip->scan_dir && !g_list_find(disabled_iplugins, ip))
			ret = ip->scan_dir(dir);
		node = g_list_next(node);
	}
	return ret;
}

void input_get_volume(int *l, int *r)
{
	*l = -1;
	*r = -1;
	if (get_input_playing())
	{
		if (get_current_input_plugin() &&
		    get_current_input_plugin()->get_volume)
		{
			get_current_input_plugin()->get_volume(l, r);
			return;
		}
	}
	output_get_volume(l, r);

}

void input_set_volume(int l, int r)
{
	if (get_input_playing())
	{
		if (get_current_input_plugin() &&
		    get_current_input_plugin()->set_volume)
		{
			get_current_input_plugin()->set_volume(l, r);
			return;
		}
	}
	output_set_volume(l, r);
}

gboolean get_input_playing(void)
{
	return ip_data->playing;
}

gboolean get_input_paused(void)
{
	return ip_data->paused;
}

void input_update_vis(gint time)
{
	GList *node, *prev;
	VisNode *vis = NULL;
	gboolean found;

	pthread_mutex_lock(&vis_mutex);
	node = vis_list;
	found = FALSE;
	while (g_list_next(node) && !found)
	{
		vis = (VisNode *) node->data;
		if (((VisNode *) node->next->data)->time >= time &&
		    vis->time < time)
		{
			prev = g_list_previous(node);
			vis_list = g_list_remove_link(vis_list, node);
			g_list_free_1(node);
			node = prev;
			found = TRUE;
			while (node)
			{
				g_free(node->data);
				prev = g_list_previous(node);
				vis_list = g_list_remove_link(vis_list, node);
				g_list_free_1(node);
				node = prev;
			}
		}
		else
			node = g_list_next(node);
	}
	pthread_mutex_unlock(&vis_mutex);
	if (found)
	{
		vis_send_data(vis->data, vis->nch, vis->length);
		g_free(vis);
	}
	else
		vis_send_data(NULL, 0, 0);
}
	

gchar *input_get_info_text(void)
{
	return input_info_text;
}

void input_set_info_text(gchar * text)
{
	if (input_info_text)
		g_free(input_info_text);
	if (text)
		input_info_text = g_strdup(text);
	else
		input_info_text = NULL;
	mainwin_set_info_text();
}

void input_about(gint index)
{
	InputPlugin *ip;

	ip = g_list_nth(ip_data->input_list, index)->data;
	if (ip && ip->about)
		ip->about();
}

void input_configure(gint index)
{
	InputPlugin *ip;

	ip = g_list_nth(ip_data->input_list, index)->data;
	if (ip && ip->configure)
		ip->configure();
}





