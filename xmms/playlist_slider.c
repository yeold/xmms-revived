/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
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

void playlistslider_draw(Widget * w)
{
	PlaylistSlider *ps = (PlaylistSlider *) w;
	SkinIndex src;
	GdkPixmap *obj;
	gint y;

	if (get_playlist_length() > ps->ps_list->pl_num_visible)
		y = (ps->ps_list->pl_first * (ps->ps_widget.height - 19)) / (get_playlist_length() - ps->ps_list->pl_num_visible);
	else
		y = 0;

	src = SKIN_PLEDIT;
	obj = ps->ps_widget.parent;

	if (ps->ps_back_image)
	{
		if (skin_get_id() != ps->ps_skin_id)
			ps->ps_skin_id = skin_get_id();
		else if (ps->ps_widget.height == ps->ps_prev_height)
			gdk_draw_image(obj, ps->ps_widget.gc,
				       ps->ps_back_image, 0, 0,
				       ps->ps_widget.x,
				       ps->ps_widget.y + ps->ps_prev_y, 8, 18);
		gdk_image_destroy(ps->ps_back_image);
	}

	ps->ps_prev_y = y;
	ps->ps_prev_height = ps->ps_widget.height;
	ps->ps_back_image = gdk_image_get(obj, ps->ps_widget.x, ps->ps_widget.y + y, 8, 18);
	skin_draw_pixmap(obj, ps->ps_widget.gc, src, ps->ps_is_draging ? 61 : 52, 53, ps->ps_widget.x, ps->ps_widget.y + y, 8, 18);
}

void playlistslider_button_press_cb(GtkWidget * widget, GdkEventButton * event, PlaylistSlider * ps)
{
	gint y;

	if (inside_widget(event->x, event->y, &ps->ps_widget))
	{
		if(event->button == 1)
		{
			y = event->y - ps->ps_widget.y;
			if (y >= ps->ps_prev_y && y < ps->ps_prev_y + 18)
			{
				ps->ps_is_draging = TRUE;
				ps->ps_drag_y = y - ps->ps_prev_y;
			}
			else
			{
				if (y < ps->ps_prev_y)
				{
					ps->ps_list->pl_first -= ps->ps_list->pl_num_visible / 2;
					if (ps->ps_list->pl_first < 0)
						ps->ps_list->pl_first = 0;
				}
				else if (y >= ps->ps_prev_y + 18)
				{
					ps->ps_list->pl_first += ps->ps_list->pl_num_visible / 2;
					if (ps->ps_list->pl_first >= get_playlist_length() - ps->ps_list->pl_num_visible)
					{
						ps->ps_list->pl_first = get_playlist_length() - ps->ps_list->pl_num_visible;
						if (ps->ps_list->pl_first < 0)
							ps->ps_list->pl_first = 0;
					}
				}
				draw_widget(ps->ps_list);
			}
			draw_widget(ps);
		}
	}
}

void playlistslider_button_release_cb(GtkWidget * widget, GdkEventButton * event, PlaylistSlider * ps)
{
	if (event->button != 1)
		return;
	ps->ps_is_draging = 0;
	draw_widget(ps);
}

void playlistslider_motion_cb(GtkWidget * widget, GdkEventMotion * event, PlaylistSlider * ps)
{
	gint y, pos;

	if (!ps->ps_is_draging)
		return;

	y = event->y - ps->ps_widget.y - ps->ps_drag_y;
	if (y < 0)
		y = 0;
	if (y >= ps->ps_widget.height - 18)
		y = ps->ps_widget.height - 19;

	pos = (y * (get_playlist_length() - ps->ps_list->pl_num_visible)) / (ps->ps_widget.height - 19);
	if (pos != ps->ps_list->pl_first)
	{
		ps->ps_list->pl_first = pos;
		draw_widget(ps);
		draw_widget(ps->ps_list);
	}
}

PlaylistSlider *create_playlistslider(GList ** wlist, GdkPixmap * parent, GdkGC * gc, gint x, gint y, gint h, PlayList_List * list)
{
	PlaylistSlider *ps;

	ps = (PlaylistSlider *) g_malloc0(sizeof (PlaylistSlider));
	ps->ps_widget.parent = parent;
	ps->ps_widget.gc = gc;
	ps->ps_widget.x = x;
	ps->ps_widget.y = y;
	ps->ps_widget.width = 8;
	ps->ps_widget.height = h;
	ps->ps_widget.visible = 1;
	ps->ps_widget.button_press_cb = GTK_SIGNAL_FUNC(playlistslider_button_press_cb);
	ps->ps_widget.button_release_cb = GTK_SIGNAL_FUNC(playlistslider_button_release_cb);
	ps->ps_widget.motion_cb = GTK_SIGNAL_FUNC(playlistslider_motion_cb);
	ps->ps_widget.draw = playlistslider_draw;
	ps->ps_list = list;
	add_widget(wlist, ps);
	return ps;
}
