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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "config.h"
#include <gnome.h>
#include <applet-widget.h>

#include "xmms/i18n.h"
#include "libxmms/xmmsctrl.h"
#include "xmms-dock-master.xpm"
#include "gnomexmms.xpm"

typedef struct
{
	gint x, y, width, height, pressed_x, pressed_y, normal_x, normal_y;
	gboolean focus, pressed;
	void (*callback) (void);
}
Button;

void action_play(void);
void action_pause(void);
void action_eject(void);
void action_prev(void);
void action_next(void);
void action_stop(void);

Button buttons[] =
{
	{21 - 9, 28 - 9, 9, 11, 20, 64, 0, 64, FALSE, FALSE, action_play},	/* PLAY */
	{33 - 9, 28 - 9, 9, 11, 30, 64, 10, 64, FALSE, FALSE, action_pause},	/* PAUSE */
	{45 - 9, 28 - 9, 9, 11, 20, 75, 0, 75, FALSE, FALSE, action_eject},	/* EJECT */
	{21 - 9, 42 - 9, 9, 11, 20, 86, 0, 86, FALSE, FALSE, action_prev},	/* PREV */
	{33 - 9, 42 - 9, 9, 11, 30, 86, 10, 86, FALSE, FALSE, action_next},	/* NEXT */
	{45 - 9, 42 - 9, 9, 11, 30, 75, 10, 75, FALSE, FALSE, action_stop}	/* STOP */
};

#define NUM_BUTTONS 6

GList *button_list;

#define VOLSLIDER_X		11-9
#define VOLSLIDER_Y		12-9
#define VOLSLIDER_WIDTH		6
#define VOLSLIDER_HEIGHT	40

#define SEEKSLIDER_X		21-9
#define SEEKSLIDER_Y		16-9
#define SEEKSLIDER_WIDTH	30
#define SEEKSLIDER_HEIGHT	7
#define SEEKSLIDER_KNOB_WIDTH	3
#define SEEKSLIDER_MAX		(SEEKSLIDER_WIDTH - SEEKSLIDER_KNOB_WIDTH)

gboolean volslider_dragging = FALSE;
gint volslider_pos = 0;
gboolean seekslider_visible = FALSE, seekslider_dragging = FALSE;
gint seekslider_pos = -1, seekslider_drag_offset = 0;
gint timeout_tag = 0;

void init(void);

GtkWidget *applet, *window, *vbox;
GdkPixmap *pixmap, *launch_pixmap;
GdkBitmap *mask, *launch_mask;
GdkGC *dock_gc;

gint xmms_session = 0;
gchar *xmms_cmd = "xmms";
gboolean xmms_running = FALSE;

gboolean single_click = FALSE;

enum {
	DROP_PLAINTEXT,
	DROP_URLENCODED,
	DROP_STRING,
	NUM_DROP_TYPES
};

static GtkTargetEntry drop_types[] =
{
	{"text/plain",    0, DROP_PLAINTEXT},
	{"text/uri-list", 0, DROP_URLENCODED},
	{"STRING",        0, DROP_STRING}
};

void action_play(void)
{
	xmms_remote_play(xmms_session);
}

void action_pause(void)
{
	xmms_remote_pause(xmms_session);
}

void action_eject(void)
{
	xmms_remote_eject(xmms_session);
}

void action_prev(void)
{
	xmms_remote_playlist_prev(xmms_session);
}

void action_next(void)
{
	xmms_remote_playlist_next(xmms_session);
}

void action_stop(void)
{
	xmms_remote_stop(xmms_session);
}

gboolean inside_region(gint mx, gint my, gint x, gint y, gint w, gint h)
{
	if ((mx >= x && mx < x + w) && (my >= y && my < y + h))
		return TRUE;

	return FALSE;
}

void draw_button(Button * button)
{
	if (button->pressed)
		gdk_draw_pixmap(window->window, dock_gc, pixmap, button->pressed_x, button->pressed_y, button->x, button->y, button->width, button->height);
	else
		gdk_draw_pixmap(window->window, dock_gc, pixmap, button->normal_x, button->normal_y, button->x, button->y, button->width, button->height);
}

void draw_buttons(GList * list)
{
	for (; list; list = g_list_next(list))
		draw_button(list->data);
}

void draw_volslider(void)
{
	gdk_draw_pixmap(window->window, dock_gc, pixmap, 48, 65, VOLSLIDER_X, VOLSLIDER_Y, VOLSLIDER_WIDTH, VOLSLIDER_HEIGHT);
	gdk_draw_pixmap(window->window, dock_gc, pixmap, 42, 65 + VOLSLIDER_HEIGHT - volslider_pos, VOLSLIDER_X, VOLSLIDER_Y + VOLSLIDER_HEIGHT - volslider_pos, VOLSLIDER_WIDTH, volslider_pos);
}

void draw_seekslider(void)
{
	gint slider_x;

	if (seekslider_visible)
	{
		gdk_draw_pixmap(window->window, dock_gc, pixmap, 2, 114, 19 - 9, 12 - 9, 35, 14);
		if (seekslider_pos < SEEKSLIDER_MAX / 3)
			slider_x = 44;
		else if (seekslider_pos < (SEEKSLIDER_MAX * 2) / 3)
			slider_x = 47;
		else
			slider_x = 50;
		gdk_draw_pixmap(window->window, dock_gc, pixmap, slider_x, 112, SEEKSLIDER_X + seekslider_pos, SEEKSLIDER_Y, 3, SEEKSLIDER_HEIGHT);
	}
	else
		gdk_draw_pixmap(window->window, dock_gc, pixmap, 2, 100, 19 - 9, 12 - 9, 35, 14);
}

void redraw_window(void)
{
	if (xmms_running)
	{
		gdk_draw_pixmap(window->window, dock_gc, pixmap, 9, 9, 0, 0, 64, 64);
		draw_buttons(button_list);
		draw_volslider();
		draw_seekslider();
	}
	else
	{
		gdk_draw_pixmap(window->window, dock_gc, launch_pixmap, 0, 0, 0, 0, 64, 64);
	}
}

void expose_cb(GtkWidget * w, GdkEventExpose * event, gpointer data)
{
	redraw_window();
}

void button_press_cb(GtkWidget * w, GdkEventButton * event, gpointer data)
{
	GList *node;
	Button *btn;
	gint pos;
	gchar *cmd;

	if (xmms_running)
	{
		for (node = button_list; node; node = g_list_next(node))
		{
			btn = node->data;
			if (inside_region(event->x, event->y, btn->x, btn->y, btn->width, btn->height))
			{
				btn->focus = TRUE;
				btn->pressed = TRUE;
				draw_button(btn);
			}
		}
		if (inside_region(event->x, event->y, VOLSLIDER_X, VOLSLIDER_Y, VOLSLIDER_WIDTH, VOLSLIDER_HEIGHT))
		{
			volslider_pos = VOLSLIDER_HEIGHT - (event->y - VOLSLIDER_Y + 17);
			xmms_remote_set_main_volume(xmms_session, (volslider_pos * 100) / VOLSLIDER_HEIGHT);
			draw_volslider();
			volslider_dragging = TRUE;
		}
		if (inside_region(event->x, event->y, SEEKSLIDER_X, SEEKSLIDER_Y, SEEKSLIDER_WIDTH, SEEKSLIDER_HEIGHT) && seekslider_visible)
		{
			pos = event->x - SEEKSLIDER_X + 17;

			if (pos >= seekslider_pos && pos < seekslider_pos + SEEKSLIDER_KNOB_WIDTH)
				seekslider_drag_offset = pos - seekslider_pos;
			else
			{
				seekslider_drag_offset = 1;
				seekslider_pos = pos - seekslider_drag_offset;
				if (seekslider_pos < 0)
					seekslider_pos = 0;
				if (seekslider_pos > SEEKSLIDER_MAX)
					seekslider_pos = SEEKSLIDER_MAX;
			}
			draw_seekslider();
			seekslider_dragging = TRUE;
		}
	}
	else if (event->type == GDK_2BUTTON_PRESS)
	{
		cmd = g_strconcat(xmms_cmd, " &", NULL);
		system(cmd);
		g_free(cmd);
	}

}

void button_release_cb(GtkWidget * w, GdkEventButton * event, gpointer data)
{
	GList *node;
	Button *btn;
	gint len;

	if (event->button != 1)
		return;

	for (node = button_list; node; node = g_list_next(node))
	{
		btn = node->data;
		if (btn->pressed)
		{
			btn->focus = FALSE;
			btn->pressed = FALSE;
			draw_button(btn);
			if (btn->callback)
				btn->callback();
		}
	}
	volslider_dragging = FALSE;
	if (seekslider_dragging)
	{
		len = xmms_remote_get_playlist_time(xmms_session, xmms_remote_get_playlist_pos(xmms_session));
		xmms_remote_jump_to_time(xmms_session, ((seekslider_pos * len) / SEEKSLIDER_MAX));
		seekslider_dragging = FALSE;
	}
}

void motion_notify_cb(GtkWidget * w, GdkEventMotion * event, gpointer data)
{
	GList *node;
	Button *btn;
	gboolean inside;

	for (node = button_list; node; node = g_list_next(node))
	{
		btn = node->data;
		if (btn->focus)
		{
			inside = inside_region(event->x, event->y, btn->x, btn->y, btn->width, btn->height);
			if ((inside && !btn->pressed) || (!inside && btn->pressed))
			{
				btn->pressed = inside;
				draw_button(btn);
			}
		}
	}

	if (volslider_dragging)
	{
		volslider_pos = VOLSLIDER_HEIGHT - (event->y - VOLSLIDER_Y + 17);
		if (volslider_pos < 0)
			volslider_pos = 0;
		if (volslider_pos > VOLSLIDER_HEIGHT)
			volslider_pos = VOLSLIDER_HEIGHT;
		xmms_remote_set_main_volume(xmms_session, (volslider_pos * 100) / VOLSLIDER_HEIGHT);
		draw_volslider();
	}
	if (seekslider_dragging)
	{
		seekslider_pos = (event->x - SEEKSLIDER_X) - seekslider_drag_offset + 17;
		if (seekslider_pos < 0)
			seekslider_pos = 0;
		if (seekslider_pos > SEEKSLIDER_MAX)
			seekslider_pos = SEEKSLIDER_MAX;
		draw_seekslider();
	}
}

static void update_tooltip(void)
{
	static int pl_pos = -1;
	static char *filename;
	int new_pos;
	
	new_pos = xmms_remote_get_playlist_pos(xmms_session);
	if (new_pos == 0)
	{
		/*
		 * Need to do some extra checking, as we get 0 also on
		 * a empty playlist
		 */
		char *current = xmms_remote_get_playlist_file(xmms_session, 0);
		if (!filename && current)
		{
			filename = current;
			new_pos = -1;
		}
		else if (filename && !current)
		{
			g_free(filename);
			filename = NULL;
			new_pos = -1;
		}
		else if (filename && current && strcmp(filename, current))
		{
			g_free(filename);
			filename = current;
			new_pos = -1;
		}
	}
		
	if (pl_pos != new_pos)
	{
		char *title =
			xmms_remote_get_playlist_title(xmms_session, new_pos);
		char *tip = NULL;

		if (title)
		{
			int len = xmms_remote_get_playlist_time(xmms_session,
								new_pos);
			if (len > -1)
				tip = g_strdup_printf("%s (%d.%02d)", title,
						      len / 60000,
						      (len / 1000) % 60);
			else
				tip = g_strdup(title);
		}
		applet_widget_set_tooltip(APPLET_WIDGET(applet), tip);
		g_free(tip);
		pl_pos = new_pos;
	}

}

gint timeout_func(gpointer data)
{
	gint new_pos, pos, len;
	gboolean running, playing;

	running = xmms_remote_is_running(xmms_session);

	if (running)
	{
		if (!xmms_running)
		{
			xmms_running = running;
			redraw_window();
		}
		if (!volslider_dragging)
		{
			int vl, vr;
			xmms_remote_get_volume(xmms_session, &vl, &vr);
			new_pos = ((vl > vr ? vl : vr) * 40) / 100;
			if (new_pos < 0)
				new_pos = 0;
			if (new_pos > VOLSLIDER_HEIGHT)
				new_pos = VOLSLIDER_HEIGHT;

			if (volslider_pos != new_pos)
			{
				volslider_pos = new_pos;
				draw_volslider();
			}
		}

		update_tooltip();

		playing = xmms_remote_is_playing(xmms_session);
		if (!playing && seekslider_visible)
		{
			seekslider_visible = FALSE;
			seekslider_dragging = FALSE;
			seekslider_pos = -1;
			draw_seekslider();
		}
		else if (playing)
		{
			seekslider_visible = TRUE;
			len = xmms_remote_get_playlist_time(xmms_session, xmms_remote_get_playlist_pos(xmms_session));
			if (len == -1 && seekslider_visible)
			{
				seekslider_visible = FALSE;
				seekslider_dragging = FALSE;
				seekslider_pos = -1;
				draw_seekslider();
			}
			else if (!seekslider_dragging)
			{
				pos = xmms_remote_get_output_time(xmms_session);
				if (len != 0)
					new_pos = (pos * SEEKSLIDER_MAX) / len;
				else
					new_pos = 0;
				if (new_pos < 0)
					new_pos = 0;
				if (new_pos > SEEKSLIDER_MAX)
					new_pos = SEEKSLIDER_MAX;
				if (seekslider_pos != new_pos)
				{
					seekslider_pos = new_pos;
					draw_seekslider();
				}
			}
		}
	}
	else
	{
		if (xmms_running)
		{
			xmms_running = FALSE;
			applet_widget_set_tooltip(APPLET_WIDGET(applet), NULL);
			redraw_window();
		}
	}

	return TRUE;
}

void drag_data_received(GtkWidget * widget,
			GdkDragContext * context,
			gint x, gint y,
			GtkSelectionData * selection_data,
			guint info, guint time)
{
	if (selection_data->data)
	{
		xmms_remote_playlist_clear(xmms_session);
		xmms_remote_playlist_add_url_string(xmms_session, (gchar *) selection_data->data);
		xmms_remote_play(xmms_session);
	}
}

void init(void)
{
	GdkColor bg_color;
	gint i;

	volslider_dragging = FALSE;
	volslider_pos = 0;

	seekslider_visible = FALSE;
	seekslider_dragging = FALSE;
	seekslider_pos = -1;
	seekslider_drag_offset = 0;

	for (i = 0; i < NUM_BUTTONS; i++)
		button_list = g_list_append(button_list, &buttons[i]);

	window = gtk_drawing_area_new();
	gtk_widget_set_usize(window, 48, 48);
	gtk_widget_set_app_paintable(window, TRUE);
	vbox = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), window, TRUE, TRUE, 0);
	gtk_widget_show(window);
	gtk_widget_set_events(window, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
	gtk_signal_connect(GTK_OBJECT(window), "expose_event", GTK_SIGNAL_FUNC(expose_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "button_press_event", GTK_SIGNAL_FUNC(button_press_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "button_release_event", GTK_SIGNAL_FUNC(button_release_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "motion_notify_event", GTK_SIGNAL_FUNC(motion_notify_cb), NULL);

	gtk_drag_dest_set(window, GTK_DEST_DEFAULT_ALL, drop_types, NUM_DROP_TYPES, GDK_ACTION_COPY);
	gtk_signal_connect(GTK_OBJECT(window), "drag_data_received", GTK_SIGNAL_FUNC(drag_data_received), NULL);

	bg_color.red = 0;
	bg_color.green = 0;
	bg_color.blue = 0;
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &bg_color, FALSE, TRUE);
	gdk_window_set_background(applet->window, &bg_color);
	gdk_window_clear(applet->window);
	dock_gc = gdk_gc_new(applet->window);

	/* FIXME: add colormap and window */
	pixmap = gdk_pixmap_create_from_xpm_d(applet->window, &mask, NULL, xmms_dock_master_xpm);

	launch_pixmap = gdk_pixmap_create_from_xpm_d(applet->window, &launch_mask, NULL, gnomexmms);
	gtk_widget_show(vbox);
	timeout_tag = gtk_timeout_add(100, timeout_func, NULL);
}

static void about(AppletWidget * applet, gpointer data)
{
	static const char *authors[] =
	{"Anders Carlsson <andersca@gnu.org>",
	 "Hiroshi Takekawa <takekawa@sr3.t.u-tokyo.ac.jp>", NULL};
	GtkWidget *about_box;

	about_box = gnome_about_new(
		_("xmms applet"), VERSION,
		_("Copyright (C) Anders Carlsson 1999), Hiroshi Takekawa 2001"),
		authors,
		_("A simple xmms gnome panel applet by Anders Carlsson.\n"
		  "Some code is from wmxmms by Mikael Alm.\n"
		  "Tooltip support by Hiroshi Takekawa."), NULL);

	gtk_widget_show(about_box);
}

int main(int argc, char **argv)
{
	gtk_set_locale();
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif

	applet_widget_init("gnomexmms", VERSION, argc, argv,
			   NULL, 0, NULL);

	applet = applet_widget_new("gnomexmms");
	gtk_widget_realize(applet);

	init();

	applet_widget_add(APPLET_WIDGET(applet), vbox);
	gtk_widget_show(applet);

	applet_widget_register_stock_callback(APPLET_WIDGET(applet),
					    "about", GNOME_STOCK_MENU_ABOUT,
					      _("About..."), about, NULL);

	applet_widget_gtk_main();
	return 0;
}


#if defined(USE_DMALLOC)

/*
 * When allocating via, say, g_strdup the wrapper macro must free the
 * previously allocated string through the original g_free function.
 */
void g_free_orig (gpointer mem)
{
#undef g_free
	void g_free (gpointer);
	g_free (mem);
}

#endif
