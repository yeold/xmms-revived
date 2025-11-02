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
#include "libxmms/util.h"
#include "libxmms/titlestring.h"

static GtkWidget *prefswin, *prefswin_notebook, *prefswin_ok;
static GtkWidget *prefswin_audio_ie_cbox;
static GtkWidget *prefswin_audio_iconfig, *prefswin_audio_iabout;
static GtkWidget *prefswin_egplugins_gconfig, *prefswin_egplugins_gabout;
static GtkWidget *prefswin_egplugins_guse_cbox;
static GtkWidget *prefswin_vplugins_use_cbox, *prefswin_vplugins_list;
static GtkWidget *prefswin_vplugins_config, *prefswin_vplugins_about;
static GtkWidget *prefswin_audio_oconfig, *prefswin_audio_oabout;
static GtkWidget *prefswin_egplugins_elist, *prefswin_egplugins_econfig;
static GtkWidget *prefswin_egplugins_eabout, *prefswin_egplugins_euse_cbox;

static GtkWidget *prefswin_options_sd_entry, *prefswin_options_pbs_entry;
	
static GtkWidget *prefswin_options_font_entry, *prefswin_options_font_browse;
static GtkWidget *prefswin_options_fontset, *prefswin_mainwin_font_entry;
static GtkWidget *prefswin_mainwin_xfont, *prefswin_options_mouse_spin;
static gboolean updating_ilist = FALSE, updating_glist = FALSE, updating_vlist = FALSE;

static GtkWidget *prefswin_title_entry;

extern MenuRow *mainwin_menurow;

extern PButton *playlistwin_shade, *playlistwin_close, *equalizerwin_close;
extern PButton *mainwin_menubtn, *mainwin_minimize, *mainwin_shade, *mainwin_close;
extern TextBox *mainwin_info;
extern gboolean mainwin_focus, equalizerwin_focus, playlistwin_focus;

static gboolean is_opening = FALSE;
static gint selected_oplugin, selected_eplugin;

static GList *option_list = NULL;

void add_input_plugins(GtkCList *clist);
void add_general_plugins(GtkCList *clist);
void add_vis_plugins(GtkCList *clist);
void add_output_plugins(GtkOptionMenu *omenu);
void add_effect_plugins(GtkOptionMenu *emenu);
static void prefswin_options_write_data(void);

gint prefswin_delete_event(GtkWidget * widget, GdkEvent * event, gpointer data)
{
	gtk_widget_hide(prefswin);
	return (TRUE);
}

void prefswin_ilist_clicked(GtkCList *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	InputPlugin *ip;
	gint index;
	GList *iplist;

	if (clist->selection)
	{
		iplist = get_input_list();
		index = GPOINTER_TO_INT(clist->selection->data);
		ip = g_list_nth(iplist, index)->data;

		gtk_widget_set_sensitive(prefswin_audio_ie_cbox, 1);
		updating_ilist = TRUE;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefswin_audio_ie_cbox), (g_list_find(disabled_iplugins, ip) ? FALSE : TRUE));
		updating_ilist = FALSE;

		if (ip->configure != NULL)
			gtk_widget_set_sensitive(prefswin_audio_iconfig, 1);
		else
			gtk_widget_set_sensitive(prefswin_audio_iconfig, 0);

		if (ip->about != NULL)
			gtk_widget_set_sensitive(prefswin_audio_iabout, 1);
		else
			gtk_widget_set_sensitive(prefswin_audio_iabout, 0);

		if (event && event->type == GDK_2BUTTON_PRESS)
			gtk_signal_emit_by_name(GTK_OBJECT(prefswin_audio_iconfig), "clicked");
	}
	else
	{
		gtk_widget_set_sensitive(prefswin_audio_iconfig, 0);
		gtk_widget_set_sensitive(prefswin_audio_iabout, 0);
	}
}

void prefswin_iconfigure(GtkButton * w, gpointer data)
{
	GtkCList *clist = GTK_CLIST(data);
	if (clist->selection)
		input_configure(GPOINTER_TO_INT(clist->selection->data));
}

void prefswin_iabout(GtkButton * w, gpointer data)
{
	GtkCList *clist = GTK_CLIST(data);
	if (clist->selection)
		input_about(GPOINTER_TO_INT(clist->selection->data));
}

void prefswin_oconfigure(GtkWidget * w, gpointer data)
{
	output_configure(selected_oplugin);
}

void prefswin_oabout(GtkWidget * w, gpointer data)
{
	output_about(selected_oplugin);
}

void prefswin_econfigure(GtkButton * w, gpointer data)
{
	effect_configure(selected_eplugin);
}

void prefswin_eabout(GtkButton * w, gpointer data)
{
	effect_about(selected_eplugin);
}

void prefswin_gconfigure(GtkButton * w, gpointer data)
{
	GtkCList *clist = GTK_CLIST(data);
	gint sel = GPOINTER_TO_INT(clist->selection->data);

	general_configure(sel);
}

void prefswin_gabout(GtkButton * w, gpointer data)
{
	GtkCList *clist = GTK_CLIST(data);
	gint sel = GPOINTER_TO_INT(clist->selection->data);

	general_about(sel);
}

void prefswin_glist_clicked(GtkCList * clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	GeneralPlugin *gp;
	gint index;
	GList *gplist;

	if (clist->selection)
	{
		gplist = get_general_list();
		index = GPOINTER_TO_INT(clist->selection->data);
		gp = g_list_nth(gplist, index)->data;

		gtk_widget_set_sensitive(prefswin_egplugins_guse_cbox, 1);
		updating_glist = TRUE;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefswin_egplugins_guse_cbox), general_enabled(index));
		updating_glist = FALSE;

		if (gp && gp->configure)
			gtk_widget_set_sensitive(prefswin_egplugins_gconfig, 1);
		else
			gtk_widget_set_sensitive(prefswin_egplugins_gconfig, 0);

		if (gp && gp->about)
			gtk_widget_set_sensitive(prefswin_egplugins_gabout, 1);
		else
			gtk_widget_set_sensitive(prefswin_egplugins_gabout, 0);

		if (event && event->type == GDK_2BUTTON_PRESS)
			gtk_signal_emit_by_name(GTK_OBJECT(prefswin_egplugins_gconfig), "clicked");
	}
	else
	{
		gtk_widget_set_sensitive(prefswin_egplugins_gconfig, FALSE);
		gtk_widget_set_sensitive(prefswin_egplugins_gabout, FALSE);
	}
}

void prefswin_vconfigure(GtkButton * w, gpointer data)
{
	GtkCList *clist = GTK_CLIST(data);
	gint sel = GPOINTER_TO_INT(clist->selection->data);

	vis_configure(sel);
}

void prefswin_vabout(GtkButton * w, gpointer data)
{
	GtkCList *clist = GTK_CLIST(data);
	gint sel = GPOINTER_TO_INT(clist->selection->data);

	vis_about(sel);
}

void prefswin_vlist_clicked(GtkCList * clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	VisPlugin *vp;
	gint index;
	GList *vplist;

	if (clist->selection)
	{
		vplist = get_vis_list();
		index = GPOINTER_TO_INT(clist->selection->data);
		vp = g_list_nth(vplist, index)->data;

		gtk_widget_set_sensitive(prefswin_vplugins_use_cbox, 1);
		updating_vlist = TRUE;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefswin_vplugins_use_cbox), vis_enabled(index));
		updating_vlist = FALSE;

		if (vp && vp->configure)
			gtk_widget_set_sensitive(prefswin_vplugins_config, 1);
		else
			gtk_widget_set_sensitive(prefswin_vplugins_config, 0);

		if (vp && vp->about)
			gtk_widget_set_sensitive(prefswin_vplugins_about, 1);
		else
			gtk_widget_set_sensitive(prefswin_vplugins_about, 0);

		if (event && event->type == GDK_2BUTTON_PRESS)
			gtk_signal_emit_by_name(GTK_OBJECT(prefswin_vplugins_config), "clicked");
	}
	else
	{
		gtk_widget_set_sensitive(prefswin_vplugins_config, FALSE);
		gtk_widget_set_sensitive(prefswin_vplugins_about, FALSE);
	}
}


void prefswin_rt_callback(GtkToggleButton * w, gpointer data)
{
	if (gtk_toggle_button_get_active(w) && !is_opening)
	{
		xmms_show_message(
			_("Warning"),
			_("Realtime priority is a way for XMMS to make sure it always\n"
			  "get as much CPU time as needed for decoding audio files without\n"
			  "any \"skips\".\n\n"
			  "This requires that XMMS is run with root privileges and\n"
			  "may, although it's very unusal, lock up your computer.\n\n"
			  "You have been warned, to activate this you need to\n"
			  "restart XMMS."), _("Ok"), FALSE, NULL, NULL);
	}
}

static void prefswin_toggle_wm_decorations(void)
{
	gboolean hide_player = !cfg.player_visible;
	if (hide_player)
		mainwin_real_show();
	gtk_widget_hide(mainwin);
	if (cfg.playlist_visible)
		gtk_widget_hide(playlistwin);
	if (cfg.equalizer_visible)
		gtk_widget_hide(equalizerwin);
	mainwin_recreate();
	equalizerwin_recreate();
	playlistwin_recreate();
	gtk_widget_show(mainwin);
	if (hide_player)
		mainwin_real_hide();
	if (cfg.playlist_visible)
		gtk_widget_show(playlistwin);
	if (cfg.equalizer_visible)
		gtk_widget_show(equalizerwin);
	hint_set_always(cfg.always_on_top);
	hint_set_sticky(cfg.sticky);
	hint_set_skip_winlist(equalizerwin);
	hint_set_skip_winlist(playlistwin);
	gtk_window_set_transient_for(GTK_WINDOW(prefswin), GTK_WINDOW(mainwin));
}

void prefswin_apply_changes(void)
{
	gboolean show_wm_old = cfg.show_wm_decorations;
	g_free(cfg.playlist_font);
	g_free(cfg.mainwin_font);
	g_free(cfg.gentitle_format);
	prefswin_options_write_data();
	cfg.snap_distance = CLAMP(atoi(gtk_entry_get_text(GTK_ENTRY(prefswin_options_sd_entry))), 0, 1000);
	cfg.playlist_font = g_strdup(gtk_entry_get_text(GTK_ENTRY(prefswin_options_font_entry)));
	cfg.mainwin_font = g_strdup(gtk_entry_get_text(GTK_ENTRY(prefswin_mainwin_font_entry)));
	cfg.gentitle_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(prefswin_title_entry)));
	cfg.pause_between_songs_time = CLAMP(atoi(gtk_entry_get_text(GTK_ENTRY(prefswin_options_pbs_entry))), 0, 1000);
	cfg.use_eplugins = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prefswin_egplugins_euse_cbox));
	cfg.mouse_change = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prefswin_options_mouse_spin));

	set_current_output_plugin(selected_oplugin);
	set_current_effect_plugin(selected_eplugin);

	equalizerwin_set_doublesize(cfg.doublesize && cfg.eq_doublesize_linked);

	if (cfg.dim_titlebar)
	{
		mainwin_menubtn->pb_allow_draw = mainwin_focus;
		mainwin_minimize->pb_allow_draw = mainwin_focus;
		mainwin_shade->pb_allow_draw = mainwin_focus;
		mainwin_close->pb_allow_draw = mainwin_focus;
		equalizerwin_close->pb_allow_draw = equalizerwin_focus;
		playlistwin_shade->pb_allow_draw = playlistwin_focus;
		playlistwin_close->pb_allow_draw = playlistwin_focus;
	}
	else
	{
		mainwin_menubtn->pb_allow_draw = TRUE;
		mainwin_minimize->pb_allow_draw = TRUE;
		mainwin_shade->pb_allow_draw = TRUE;
		mainwin_close->pb_allow_draw = TRUE;
		equalizerwin_close->pb_allow_draw = TRUE;
		playlistwin_shade->pb_allow_draw = TRUE;
		playlistwin_close->pb_allow_draw = TRUE;
	}
	if (cfg.get_info_on_load)
		playlist_start_get_info_scan();

	if (mainwin_info->tb_timeout_tag)
	{
		textbox_set_scroll(mainwin_info, FALSE);
		textbox_set_scroll(mainwin_info, TRUE);
	}

	if (show_wm_old != cfg.show_wm_decorations)
		prefswin_toggle_wm_decorations();

	textbox_set_xfont(mainwin_info, cfg.mainwin_use_xfont, cfg.mainwin_font);
	playlist_list_set_font(cfg.playlist_font);
	playlistwin_update_list();

	draw_main_window(TRUE);
	draw_playlist_window(TRUE);
	draw_equalizer_window(TRUE);

	save_config();
}

void prefswin_ok_cb(GtkWidget * w, gpointer data)
{
	prefswin_apply_changes();
	gtk_widget_hide(prefswin);
}

void prefswin_cancel_cb(GtkWidget * w, gpointer data)
{
 	gtk_widget_hide(prefswin);
}

void prefswin_apply_cb(GtkWidget * w, gpointer data)
{
	prefswin_apply_changes();
}

void prefswin_font_browse_ok(GtkWidget * w, gpointer data)
{
	GtkFontSelectionDialog *fontsel = GTK_FONT_SELECTION_DIALOG(data);
	gchar *fontname;

	fontname = gtk_font_selection_dialog_get_font_name(fontsel);
	
	if (fontname)
		gtk_entry_set_text(GTK_ENTRY(prefswin_options_font_entry), fontname);

	gtk_widget_destroy(GTK_WIDGET(fontsel));
}

void prefswin_font_browse_cb(GtkWidget * w, gpointer data)
{
	static GtkWidget *fontsel;

	if (fontsel != NULL)
		return;

	fontsel = gtk_font_selection_dialog_new(_("Select playlist font:"));
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fontsel), gtk_entry_get_text(GTK_ENTRY(prefswin_options_font_entry)));
	gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fontsel)->ok_button), "clicked", GTK_SIGNAL_FUNC(prefswin_font_browse_ok), fontsel);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fontsel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(fontsel));
	gtk_signal_connect(GTK_OBJECT(fontsel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fontsel);
	gtk_widget_show(fontsel);
}

void prefswin_mainwin_font_browse_ok(GtkWidget * w, gpointer data)
{
	GtkFontSelectionDialog *fontsel = GTK_FONT_SELECTION_DIALOG(data);
	gchar *fontname;

	fontname = gtk_font_selection_dialog_get_font_name(fontsel);

	if (fontname)
		gtk_entry_set_text(GTK_ENTRY(prefswin_mainwin_font_entry), fontname);

	gtk_widget_destroy(GTK_WIDGET(fontsel));
}

void prefswin_mainwin_font_browse_cb(GtkWidget * w, gpointer data)
{
	static GtkWidget *fontsel;

	if (!fontsel)
	{
		fontsel = gtk_font_selection_dialog_new(_("Select main window font:"));
		gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fontsel), gtk_entry_get_text(GTK_ENTRY(prefswin_mainwin_font_entry)));
		gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fontsel)->ok_button), "clicked", GTK_SIGNAL_FUNC(prefswin_mainwin_font_browse_ok), fontsel);
		gtk_signal_connect_object(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fontsel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(fontsel));
		gtk_signal_connect(GTK_OBJECT(fontsel), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &fontsel);
		gtk_widget_show(fontsel);
	}
}

void prefswin_egplugins_guse_cb(GtkToggleButton * w, gpointer data)
{
	gint sel;
	GtkAdjustment *adj;
	gfloat pos;
	GtkCList *clist = GTK_CLIST(data);

	if (!clist->selection || updating_glist)
		return;

	sel = GPOINTER_TO_INT(clist->selection->data);

	enable_general_plugin(sel, gtk_toggle_button_get_active(w));
	adj = gtk_clist_get_vadjustment(clist);
	pos = adj->value;
	add_general_plugins(clist);
	gtk_adjustment_set_value(adj, pos);
	gtk_clist_set_vadjustment(clist, adj);
	gtk_clist_select_row(clist, sel, 0);
}

void prefswin_vplugins_rescan(void)
{
	gint sel;
	GtkAdjustment *adj;
	gfloat pos;
	GtkCList *clist = GTK_CLIST(prefswin_vplugins_list);

	if (clist->selection)
		sel = GPOINTER_TO_INT(clist->selection->data);
	else
		sel = -1;
	adj = gtk_clist_get_vadjustment(clist);
	pos = adj->value;
	add_vis_plugins(clist);
	gtk_adjustment_set_value(adj, pos);
	gtk_clist_set_vadjustment(clist, adj);
	if(sel != -1)
		gtk_clist_select_row(clist, sel, 0);
}
		
void prefswin_vplugins_use_cb(GtkToggleButton * w, gpointer data)
{
	gint sel;
	GtkCList *clist = GTK_CLIST(data);

	if (!clist->selection || updating_vlist)
		return;

	sel = GPOINTER_TO_INT(clist->selection->data);
	
	enable_vis_plugin(sel, gtk_toggle_button_get_active(w));
	
	prefswin_vplugins_rescan();
}


void prefswin_ip_toggled(GtkToggleButton * w, gpointer data)
{
	InputPlugin *selected;
	gint sel;
	GtkAdjustment *adj;
	gfloat pos;
	GtkCList *clist = GTK_CLIST(data);

	if (!clist->selection || updating_ilist)
		return;

	sel = GPOINTER_TO_INT(clist->selection->data);

	selected = (InputPlugin *) (g_list_nth(get_input_list(), sel)->data);

	if (!gtk_toggle_button_get_active(w))
		disabled_iplugins = g_list_append(disabled_iplugins, selected);
	else if (g_list_find(disabled_iplugins, selected))
		disabled_iplugins = g_list_remove(disabled_iplugins, selected);
	adj = gtk_clist_get_vadjustment(clist);
	pos = adj->value;
	add_input_plugins(clist);
	gtk_adjustment_set_value(adj, pos);
	gtk_clist_set_vadjustment(clist, adj);
	gtk_clist_select_row(clist, sel, 0);
}

static GtkWidget * prefswin_option_new(gboolean * cfg)
{
	struct option_info *info;
	info = g_malloc(sizeof(struct option_info));
	info->button = gtk_check_button_new();
	info->cfg = cfg;
	option_list = g_list_prepend(option_list, info);

	return info->button;
}

static GtkWidget * prefswin_option_new_with_label(gboolean * cfg, gchar * label)
{
	GtkWidget *buttonw, *labelw;
	buttonw = prefswin_option_new(cfg);
	labelw = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(labelw), 0.0, 0.5);
	gtk_container_add(GTK_CONTAINER(buttonw), labelw);

	return buttonw;
}

static GtkWidget * prefswin_option_new_with_label_to_table(gboolean * cfg, gchar * label, GtkTable * table, gint x, gint y)
{
	GtkWidget *buttonw;
	buttonw = prefswin_option_new_with_label(cfg, label);
	gtk_table_attach_defaults(table, buttonw, x, x + 1, y, y + 1);

	return buttonw;
}

static GtkWidget * prefswin_option_new_to_table(gboolean * cfg, GtkTable * table, gint x, gint y)
{
	GtkWidget *buttonw;
	buttonw = prefswin_option_new(cfg);
	gtk_table_attach_defaults(table, buttonw, x, x + 1, y, y + 1);

	return buttonw;
}

static void prefswin_options_read_data(void)
{
	GList *node;

	for (node = option_list; node; node = g_list_next(node))
	{
		struct option_info *info = node->data;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(info->button), *(info->cfg));
	}
}

static void prefswin_options_write_data(void)
{
	GList *node;

	for (node = option_list; node; node = g_list_next(node))
	{
		struct option_info *info = node->data;
		*info->cfg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->button));
	}
}

void create_prefs_window(void)
{
	GtkWidget *prefswin_audio_vbox;
	GtkWidget *prefswin_audio_ilist, *prefswin_audio_iframe, *prefswin_audio_ivbox;
	GtkWidget *prefswin_audio_ihbox, *prefswin_audio_ihbbox;
	GtkWidget *prefswin_audio_oframe, *prefswin_audio_ovbox, *prefswin_audio_olist;
	GtkWidget *prefswin_audio_ohbox, *prefswin_egplugins_vbox;
	GtkWidget *prefswin_egplugins_eframe, *prefswin_egplugins_evbox;
	GtkWidget *prefswin_egplugins_ehbox;
	GtkWidget *prefswin_egplugins_gframe, *prefswin_egplugins_gvbox;
	GtkWidget *prefswin_egplugins_ghbox, *prefswin_egplugins_glist;
	
	GtkWidget *prefswin_vplugins_box, *prefswin_vplugins_vbox;
	GtkWidget *prefswin_vplugins_frame, *prefswin_vplugins_hbox;
	
	GtkWidget *prefswin_options_frame, *prefswin_options_vbox;
	GtkWidget *prefswin_mainwin_frame, *prefswin_mainwin_vbox;
	GtkWidget *prefswin_fonts_vbox, *prefswin_fonts_playlist_frame;
	GtkWidget *prefswin_fonts_options_vbox, *prefswin_fonts_options_frame;
	GtkWidget *prefswin_mainwin_font_hbox, *prefswin_mainwin_font_browse;
	GtkWidget *scrolled_win;
	GtkWidget *prefswin_vbox, *prefswin_hbox, *prefswin_cancel, *prefswin_apply;

	GtkWidget *prefswin_title_frame;
	GtkWidget *prefswin_title_box, *prefswin_title_vbox;

	GtkWidget *options_table;
	GtkWidget *options_giod, *options_giol, *options_rt;
	GtkWidget *options_sw, *options_sw_box, *options_sw_label;
	GtkWidget *options_pbs, *options_pbs_box, *options_pbs_label;
	GtkWidget *options_pbs_label2, *options_sd_label;
	GtkWidget *options_gi_box, *options_gi_label;
	GtkWidget *options_font_hbox, *options_font_vbox;
	GtkWidget *options_mouse_box, *options_mouse_label;
	GtkObject *options_mouse_adj;
	GtkWidget *prefswin_title_desc, *prefswin_title_label;

	char *titles[1];

	prefswin = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(prefswin), _("Preferences"));
/*  	gtk_window_set_position(GTK_WINDOW(prefswin), GTK_WIN_POS_CENTER); */
	gtk_window_set_policy(GTK_WINDOW(prefswin), FALSE, FALSE, FALSE);
	gtk_window_set_transient_for(GTK_WINDOW(prefswin), GTK_WINDOW(mainwin));
	gtk_signal_connect(GTK_OBJECT(prefswin), "delete_event", GTK_SIGNAL_FUNC(prefswin_delete_event), NULL);
	gtk_container_border_width(GTK_CONTAINER(prefswin), 10);

	prefswin_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(prefswin), prefswin_vbox);
	prefswin_notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(prefswin_vbox), prefswin_notebook, TRUE, TRUE, 0);

	/*
	 * Audio I/O Page
	 */

	prefswin_audio_vbox = gtk_vbox_new(FALSE, 0);

	/*
	 * Input plugins
	 */

	prefswin_audio_iframe = gtk_frame_new(_("Input Plugins"));
	gtk_container_border_width(GTK_CONTAINER(prefswin_audio_iframe), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_vbox), prefswin_audio_iframe, TRUE, TRUE, 0);

	prefswin_audio_ivbox = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(prefswin_audio_ivbox), 5);
	gtk_container_add(GTK_CONTAINER(prefswin_audio_iframe), prefswin_audio_ivbox);

	titles[0] = _("Input plugins");
	prefswin_audio_ilist = gtk_clist_new_with_titles(1, titles);
	gtk_widget_set_usize(prefswin_audio_ilist, -1, 80);
	gtk_clist_column_titles_passive(GTK_CLIST(prefswin_audio_ilist));
	gtk_clist_set_selection_mode(GTK_CLIST(prefswin_audio_ilist), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_ilist), "select_row", GTK_SIGNAL_FUNC(prefswin_ilist_clicked), NULL);
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_ilist), "unselect_row", GTK_SIGNAL_FUNC(prefswin_ilist_clicked), NULL);
	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), prefswin_audio_ilist);
	gtk_container_border_width(GTK_CONTAINER(scrolled_win), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ivbox), scrolled_win, TRUE, TRUE, 0);

	prefswin_audio_ihbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ivbox), prefswin_audio_ihbox, FALSE, FALSE, 5);

	prefswin_audio_ihbbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(prefswin_audio_ihbbox), GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(prefswin_audio_ihbbox), 10);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(prefswin_audio_ihbbox), 85, 17);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ihbox), prefswin_audio_ihbbox, TRUE, TRUE, 0);

	prefswin_audio_iconfig = gtk_button_new_with_label(_("Configure"));
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_iconfig), "clicked", GTK_SIGNAL_FUNC(prefswin_iconfigure), prefswin_audio_ilist);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ihbbox), prefswin_audio_iconfig, TRUE, TRUE, 0);
	prefswin_audio_iabout = gtk_button_new_with_label(_("About"));
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_iabout), "clicked", GTK_SIGNAL_FUNC(prefswin_iabout), prefswin_audio_ilist);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ihbbox), prefswin_audio_iabout, TRUE, TRUE, 0);

	prefswin_audio_ie_cbox = gtk_check_button_new_with_label(_("Enable plugin"));
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_ie_cbox), "toggled", GTK_SIGNAL_FUNC(prefswin_ip_toggled), prefswin_audio_ilist);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ihbox), prefswin_audio_ie_cbox, FALSE, FALSE, 10);

	/* 
	 * Output plugin
	 */

	prefswin_audio_oframe = gtk_frame_new(_("Output Plugin"));
	gtk_container_border_width(GTK_CONTAINER(prefswin_audio_oframe), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_vbox), prefswin_audio_oframe, FALSE, FALSE, 0);

	prefswin_audio_ovbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(prefswin_audio_ovbox), 5);
	gtk_container_add(GTK_CONTAINER(prefswin_audio_oframe), prefswin_audio_ovbox);

	prefswin_audio_olist = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ovbox), prefswin_audio_olist, TRUE, TRUE, 0);

	prefswin_audio_ohbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(prefswin_audio_ohbox), GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(prefswin_audio_ohbox), 10);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(prefswin_audio_ohbox), 85, 17);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ovbox), prefswin_audio_ohbox, FALSE, FALSE, 0);

	prefswin_audio_oconfig = gtk_button_new_with_label(_("Configure"));
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_oconfig), "clicked", GTK_SIGNAL_FUNC(prefswin_oconfigure), NULL);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ohbox), prefswin_audio_oconfig, TRUE, TRUE, 0);

	prefswin_audio_oabout = gtk_button_new_with_label(_("About"));
	gtk_signal_connect(GTK_OBJECT(prefswin_audio_oabout), "clicked", GTK_SIGNAL_FUNC(prefswin_oabout), NULL);
	gtk_box_pack_start(GTK_BOX(prefswin_audio_ohbox), prefswin_audio_oabout, TRUE, TRUE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(prefswin_notebook), prefswin_audio_vbox, gtk_label_new(_("Audio I/O Plugins")));

	/*
	 * Plugins page
	 */

	prefswin_egplugins_vbox = gtk_vbox_new(FALSE, 0);

	/*
	 * Effect plugins
	 */

	prefswin_egplugins_eframe = gtk_frame_new(_("Effect Plugins"));
	gtk_container_border_width(GTK_CONTAINER(prefswin_egplugins_eframe), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_vbox), prefswin_egplugins_eframe, FALSE, FALSE, 0);

	prefswin_egplugins_evbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(prefswin_egplugins_evbox), 5);
	gtk_container_add(GTK_CONTAINER(prefswin_egplugins_eframe), prefswin_egplugins_evbox);

	prefswin_egplugins_elist = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_evbox), prefswin_egplugins_elist, TRUE, TRUE, 0);

	prefswin_egplugins_ehbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(prefswin_egplugins_ehbox), GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(prefswin_egplugins_ehbox), 10);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(prefswin_egplugins_ehbox), 85, 17);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_evbox), prefswin_egplugins_ehbox, FALSE, FALSE, 0);

	prefswin_egplugins_econfig = gtk_button_new_with_label(_("Configure"));
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_econfig), "clicked", GTK_SIGNAL_FUNC(prefswin_econfigure), NULL);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_ehbox), prefswin_egplugins_econfig, TRUE, TRUE, 0);

	prefswin_egplugins_eabout = gtk_button_new_with_label(_("About"));
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_eabout), "clicked", GTK_SIGNAL_FUNC(prefswin_eabout), NULL);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_ehbox), prefswin_egplugins_eabout, TRUE, TRUE, 0);

	prefswin_egplugins_euse_cbox = gtk_check_button_new_with_label(_("Use plugins"));
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_ehbox), prefswin_egplugins_euse_cbox, TRUE, TRUE, 0);

	/*
	 * General plugins
	 */

	prefswin_egplugins_gframe = gtk_frame_new(_("General Plugins"));
	gtk_container_border_width(GTK_CONTAINER(prefswin_egplugins_gframe), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_vbox), prefswin_egplugins_gframe, TRUE, TRUE, 0);

	prefswin_egplugins_gvbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(prefswin_egplugins_gvbox), 5);
	gtk_container_add(GTK_CONTAINER(prefswin_egplugins_gframe), prefswin_egplugins_gvbox);
	titles[0] = _("General plugins");
	prefswin_egplugins_glist = gtk_clist_new_with_titles(1, titles);
	gtk_clist_column_titles_passive(GTK_CLIST(prefswin_egplugins_glist));
	gtk_clist_set_selection_mode(GTK_CLIST(prefswin_egplugins_glist), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_glist), "select_row", GTK_SIGNAL_FUNC(prefswin_glist_clicked), NULL);
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_glist), "unselect_row", GTK_SIGNAL_FUNC(prefswin_glist_clicked), NULL);
	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), prefswin_egplugins_glist);
	gtk_container_border_width(GTK_CONTAINER(scrolled_win), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_gvbox), scrolled_win, TRUE, TRUE, 0);

	prefswin_egplugins_ghbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(prefswin_egplugins_ghbox), GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(prefswin_egplugins_ghbox), 10);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(prefswin_egplugins_ghbox), 85, 17);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_gvbox), prefswin_egplugins_ghbox, FALSE, FALSE, 0);

	prefswin_egplugins_gconfig = gtk_button_new_with_label(_("Configure"));
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_gconfig), "clicked", GTK_SIGNAL_FUNC(prefswin_gconfigure), prefswin_egplugins_glist);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_ghbox), prefswin_egplugins_gconfig, TRUE, TRUE, 0);

	prefswin_egplugins_gabout = gtk_button_new_with_label(_("About"));
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_gabout), "clicked", GTK_SIGNAL_FUNC(prefswin_gabout), prefswin_egplugins_glist);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_ghbox), prefswin_egplugins_gabout, TRUE, TRUE, 0);

	prefswin_egplugins_guse_cbox = gtk_check_button_new_with_label(_("Enable plugin"));
	gtk_signal_connect(GTK_OBJECT(prefswin_egplugins_guse_cbox), "toggled", GTK_SIGNAL_FUNC(prefswin_egplugins_guse_cb), prefswin_egplugins_glist);
	gtk_box_pack_start(GTK_BOX(prefswin_egplugins_ghbox), prefswin_egplugins_guse_cbox, TRUE, TRUE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(prefswin_notebook), prefswin_egplugins_vbox, gtk_label_new(_("Effect/General Plugins")));

	/*
	 * Visualization plugins page
	 */
	
	prefswin_vplugins_box = gtk_vbox_new(FALSE, 0);
	
	prefswin_vplugins_frame = gtk_frame_new(_("Visualization Plugins"));
	gtk_container_border_width(GTK_CONTAINER(prefswin_vplugins_frame), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_vplugins_box), prefswin_vplugins_frame, TRUE, TRUE, 0);

	prefswin_vplugins_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(prefswin_vplugins_vbox), 5);
	gtk_container_add(GTK_CONTAINER(prefswin_vplugins_frame), prefswin_vplugins_vbox);

	titles[0] = _("Visualization plugins");
	prefswin_vplugins_list = gtk_clist_new_with_titles(1, titles);
	gtk_clist_column_titles_passive(GTK_CLIST(prefswin_vplugins_list));
	gtk_clist_set_selection_mode(GTK_CLIST(prefswin_vplugins_list), GTK_SELECTION_SINGLE);
	gtk_signal_connect(GTK_OBJECT(prefswin_vplugins_list), "select_row", GTK_SIGNAL_FUNC(prefswin_vlist_clicked), NULL);
	gtk_signal_connect(GTK_OBJECT(prefswin_vplugins_list), "unselect_row", GTK_SIGNAL_FUNC(prefswin_vlist_clicked), NULL);

	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_win), prefswin_vplugins_list);
	gtk_container_border_width(GTK_CONTAINER(scrolled_win), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(prefswin_vplugins_vbox), scrolled_win, TRUE, TRUE, 0);

	prefswin_vplugins_hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(prefswin_vplugins_hbox), GTK_BUTTONBOX_START);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(prefswin_vplugins_hbox), 10);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(prefswin_vplugins_hbox), 85, 17);
	gtk_box_pack_start(GTK_BOX(prefswin_vplugins_vbox), prefswin_vplugins_hbox, FALSE, FALSE, 0);

	prefswin_vplugins_config = gtk_button_new_with_label(_("Configure"));
	gtk_signal_connect(GTK_OBJECT(prefswin_vplugins_config), "clicked", GTK_SIGNAL_FUNC(prefswin_vconfigure), prefswin_vplugins_list);
	gtk_box_pack_start(GTK_BOX(prefswin_vplugins_hbox), prefswin_vplugins_config, TRUE, TRUE, 0);

	prefswin_vplugins_about = gtk_button_new_with_label(_("About"));
	gtk_signal_connect(GTK_OBJECT(prefswin_vplugins_about), "clicked", GTK_SIGNAL_FUNC(prefswin_vabout), prefswin_vplugins_list);
	gtk_box_pack_start(GTK_BOX(prefswin_vplugins_hbox), prefswin_vplugins_about, TRUE, TRUE, 0);

	prefswin_vplugins_use_cbox = gtk_check_button_new_with_label(_("Enable plugin"));
	gtk_signal_connect(GTK_OBJECT(prefswin_vplugins_use_cbox), "toggled", GTK_SIGNAL_FUNC(prefswin_vplugins_use_cb), prefswin_vplugins_list);
	gtk_box_pack_start(GTK_BOX(prefswin_vplugins_hbox), prefswin_vplugins_use_cbox, TRUE, TRUE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(prefswin_notebook), prefswin_vplugins_box, gtk_label_new(_("Visualization Plugins")));

	/*
	 * Options page
	 */

	prefswin_options_vbox = gtk_vbox_new(FALSE, 0);
	prefswin_options_frame = gtk_frame_new(_("Options"));
	gtk_box_pack_start(GTK_BOX(prefswin_options_vbox), prefswin_options_frame, FALSE, FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(prefswin_options_frame), 5);
	options_table = gtk_table_new(10, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(prefswin_options_frame), options_table);
	gtk_container_border_width(GTK_CONTAINER(options_table), 5);

	options_gi_box = gtk_hbox_new(FALSE, 5);
	options_gi_label = gtk_label_new(_("Read info on"));
	gtk_box_pack_start(GTK_BOX(options_gi_box), options_gi_label, FALSE, FALSE, 0);
	options_giod = prefswin_option_new_with_label(&cfg.get_info_on_demand, _("demand"));
	gtk_box_pack_start(GTK_BOX(options_gi_box), options_giod, FALSE, FALSE, 0);
	options_giol = prefswin_option_new_with_label(&cfg.get_info_on_load, _("load"));
	gtk_box_pack_start(GTK_BOX(options_gi_box), options_giol, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(options_table), options_gi_box, 0, 1, 0, 1);
	prefswin_option_new_with_label_to_table(&cfg.allow_multiple_instances, _("Allow multiple instances"), GTK_TABLE(options_table), 1, 0);

	prefswin_option_new_with_label_to_table(&cfg.open_rev_order, _("Reverse file order in fileselector"), GTK_TABLE(options_table), 0, 1);
	prefswin_option_new_with_label_to_table(&cfg.always_show_cb, _("Always show clutterbar"), GTK_TABLE(options_table), 1, 1);

	prefswin_option_new_with_label_to_table(&cfg.convert_twenty, _("Convert %20 to space"), GTK_TABLE(options_table), 0, 2);
	prefswin_option_new_with_label_to_table(&cfg.save_window_position, _("Save window positions"), GTK_TABLE(options_table), 1, 2);

	prefswin_option_new_with_label_to_table(&cfg.convert_underscore, _("Convert underscore to space"), GTK_TABLE(options_table), 0, 3);
	prefswin_option_new_with_label_to_table(&cfg.show_numbers_in_pl, _("Show numbers in playlist"), GTK_TABLE(options_table), 1, 3);

	prefswin_option_new_with_label_to_table(&cfg.dim_titlebar, _("Dim titlebar when inactive"), GTK_TABLE(options_table), 0, 4);
	prefswin_option_new_with_label_to_table(&cfg.save_playlist_position, _("Save playlist position"), GTK_TABLE(options_table), 1, 4);

	prefswin_option_new_with_label_to_table(&cfg.sort_jump_to_file, _("Sort \"Jump to file\" alphabetical"), GTK_TABLE(options_table), 0, 5);
	prefswin_option_new_with_label_to_table(&cfg.eq_doublesize_linked, _("Equalizer doublesize linked"), GTK_TABLE(options_table), 1, 5);

	options_rt = prefswin_option_new_with_label_to_table(&cfg.use_realtime, _("Use realtime priority when available"), GTK_TABLE(options_table), 0, 6);
	gtk_signal_connect(GTK_OBJECT(options_rt), "toggled", GTK_SIGNAL_FUNC(prefswin_rt_callback), NULL);
	prefswin_option_new_with_label_to_table(&cfg.smooth_title_scroll, _("Smooth title scroll"), GTK_TABLE(options_table), 1, 6);

	options_pbs = prefswin_option_new_to_table(&cfg.pause_between_songs, GTK_TABLE(options_table), 0, 7);
	options_pbs_box = gtk_hbox_new(FALSE, 5);
	options_pbs_label = gtk_label_new(_("Pause between songs for"));
	gtk_box_pack_start(GTK_BOX(options_pbs_box), options_pbs_label, FALSE, FALSE, 0);
	prefswin_options_pbs_entry = gtk_entry_new_with_max_length(3);
	gtk_widget_set_usize(prefswin_options_pbs_entry, 30, -1);
	gtk_box_pack_start(GTK_BOX(options_pbs_box), prefswin_options_pbs_entry, FALSE, FALSE, 0);
	/* I18N: s = seconds */
	options_pbs_label2 = gtk_label_new(_("s"));
	gtk_box_pack_start(GTK_BOX(options_pbs_box), options_pbs_label2, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(options_pbs), options_pbs_box);

	options_sw = prefswin_option_new_to_table(&cfg.snap_windows, GTK_TABLE(options_table), 1, 7);
	options_sw_box = gtk_hbox_new(FALSE, 5);
	options_sw_label = gtk_label_new(_("Snap windows at"));
	gtk_box_pack_start(GTK_BOX(options_sw_box), options_sw_label, FALSE, FALSE, 0);
	prefswin_options_sd_entry = gtk_entry_new_with_max_length(3);
	gtk_widget_set_usize(prefswin_options_sd_entry, 30, -1);
	gtk_box_pack_start(GTK_BOX(options_sw_box), prefswin_options_sd_entry, FALSE, FALSE, 0);
	options_sd_label = gtk_label_new(_("pixels"));
	gtk_box_pack_start(GTK_BOX(options_sw_box), options_sd_label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(options_sw), options_sw_box);


	prefswin_option_new_with_label_to_table(&cfg.show_wm_decorations, _("Do not hide windowmanager decorations"), GTK_TABLE(options_table), 0, 8);
	prefswin_option_new_with_label_to_table(&cfg.use_backslash_as_dir_delimiter, _("Use \'\\\' as a directory delimiter"), GTK_TABLE(options_table), 1, 8);

#if 0
	prefswin_option_new_with_label_to_table(&cfg.playlist_transparent, _("Transparent playlist window"), GTK_TABLE(options_table), 0, 9);
#endif
	options_mouse_box = gtk_hbox_new(FALSE, 5);
	options_mouse_label = gtk_label_new(_("Mouse Wheel adjusts Volume by (%)"));
	gtk_box_pack_start(GTK_BOX(options_mouse_box), options_mouse_label, FALSE, FALSE, 0);
	options_mouse_adj = gtk_adjustment_new(cfg.mouse_change, 1, 100, 1, 1, 1);
	prefswin_options_mouse_spin = gtk_spin_button_new(GTK_ADJUSTMENT(options_mouse_adj), 1, 0);
        gtk_widget_set_usize(prefswin_options_mouse_spin, 45, -1);
	gtk_box_pack_start(GTK_BOX(options_mouse_box), prefswin_options_mouse_spin, FALSE, FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(options_table), options_mouse_box, 0, 1, 9, 10);
	
	prefswin_option_new_with_label_to_table(&cfg.read_pl_metadata, _("Read meta data from playlists"), GTK_TABLE(options_table), 1, 9);

	gtk_notebook_append_page(GTK_NOTEBOOK(prefswin_notebook), prefswin_options_vbox, gtk_label_new(_("Options")));

	/*
	 * Fonts page
	 */

	prefswin_fonts_vbox = gtk_vbox_new(FALSE, 0);
	prefswin_fonts_options_frame = gtk_frame_new(_("Options"));
	gtk_box_pack_start(GTK_BOX(prefswin_fonts_vbox), prefswin_fonts_options_frame, FALSE, FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(prefswin_fonts_options_frame), 5);
	prefswin_fonts_options_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(prefswin_fonts_options_frame), prefswin_fonts_options_vbox);
	prefswin_options_fontset = prefswin_option_new_with_label(&cfg.use_fontsets, _("Use fontsets (Enable for multi-byte charset support)"));
	gtk_box_pack_start_defaults(GTK_BOX(prefswin_fonts_options_vbox), prefswin_options_fontset);
	
	prefswin_fonts_playlist_frame = gtk_frame_new(_("Playlist"));
	gtk_container_set_border_width(GTK_CONTAINER(prefswin_fonts_playlist_frame), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_fonts_vbox), prefswin_fonts_playlist_frame, FALSE, FALSE, 0);
	options_font_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width(GTK_CONTAINER(options_font_vbox), 5);
	gtk_container_add(GTK_CONTAINER(prefswin_fonts_playlist_frame), options_font_vbox);
	options_font_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start_defaults(GTK_BOX(options_font_vbox), options_font_hbox);
	prefswin_options_font_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(options_font_hbox), prefswin_options_font_entry, TRUE, TRUE, 0);
	prefswin_options_font_browse = gtk_button_new_with_label(_("Browse"));
	gtk_signal_connect(GTK_OBJECT(prefswin_options_font_browse), "clicked", GTK_SIGNAL_FUNC(prefswin_font_browse_cb), NULL);
	gtk_widget_set_usize(prefswin_options_font_browse, 85, 17);
	gtk_box_pack_start(GTK_BOX(options_font_hbox), prefswin_options_font_browse, FALSE, TRUE, 0);


	prefswin_mainwin_frame = gtk_frame_new(_("Main Window"));
	gtk_box_pack_start(GTK_BOX(prefswin_fonts_vbox), prefswin_mainwin_frame, FALSE, FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(prefswin_mainwin_frame), 5);
	prefswin_mainwin_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(prefswin_mainwin_frame), prefswin_mainwin_vbox);
	gtk_container_border_width(GTK_CONTAINER(prefswin_mainwin_vbox), 5);

	prefswin_mainwin_xfont = prefswin_option_new_with_label(&cfg.mainwin_use_xfont, _("Use X font"));
	gtk_box_pack_start_defaults(GTK_BOX(prefswin_mainwin_vbox), prefswin_mainwin_xfont);

	prefswin_mainwin_font_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start_defaults(GTK_BOX(prefswin_mainwin_vbox), prefswin_mainwin_font_hbox);
	prefswin_mainwin_font_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(prefswin_mainwin_font_hbox), prefswin_mainwin_font_entry, TRUE, TRUE, 0);
	prefswin_mainwin_font_browse = gtk_button_new_with_label(_("Browse"));
	gtk_signal_connect(GTK_OBJECT(prefswin_mainwin_font_browse), "clicked", GTK_SIGNAL_FUNC(prefswin_mainwin_font_browse_cb), NULL);
	gtk_widget_set_usize(prefswin_mainwin_font_browse, 85, 17);
	gtk_box_pack_start(GTK_BOX(prefswin_mainwin_font_hbox), prefswin_mainwin_font_browse, FALSE, TRUE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(prefswin_notebook), prefswin_fonts_vbox, gtk_label_new(_("Fonts")));

	/*
	 * Title page
	 */

	prefswin_title_frame = gtk_frame_new(_("Title"));
	gtk_container_border_width(GTK_CONTAINER(prefswin_title_frame), 5);
	prefswin_title_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(prefswin_title_frame), prefswin_title_vbox);
	gtk_container_border_width(GTK_CONTAINER(prefswin_title_vbox), 5);

	prefswin_title_box = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(prefswin_title_vbox), prefswin_title_box, FALSE, FALSE, 0);
	prefswin_title_label = gtk_label_new(_("Title format:"));
	gtk_box_pack_start(GTK_BOX(prefswin_title_box), prefswin_title_label, FALSE, FALSE, 0);
	prefswin_title_entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(prefswin_title_box), prefswin_title_entry, TRUE, TRUE, 0);

	prefswin_title_desc = xmms_titlestring_descriptions("pagfFetndyc", 2);
	gtk_box_pack_start(GTK_BOX(prefswin_title_vbox), prefswin_title_desc, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(prefswin_notebook), prefswin_title_frame, gtk_label_new(_("Title")));


	/* 
	 * Ok, Cancel & Apply 
	 */

	prefswin_hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(prefswin_hbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(prefswin_hbox), 5);
	gtk_box_pack_start(GTK_BOX(prefswin_vbox), prefswin_hbox, FALSE, FALSE, 0);

	prefswin_ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect(GTK_OBJECT(prefswin_ok), "clicked", GTK_SIGNAL_FUNC(prefswin_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(prefswin_ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(prefswin_hbox), prefswin_ok, TRUE, TRUE, 0);
	prefswin_cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect(GTK_OBJECT(prefswin_cancel), "clicked", GTK_SIGNAL_FUNC(prefswin_cancel_cb), NULL);
	GTK_WIDGET_SET_FLAGS(prefswin_cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(prefswin_hbox), prefswin_cancel, TRUE, TRUE, 0);
	prefswin_apply = gtk_button_new_with_label(_("Apply"));
	gtk_signal_connect(GTK_OBJECT(prefswin_apply), "clicked", GTK_SIGNAL_FUNC(prefswin_apply_cb), NULL);
	GTK_WIDGET_SET_FLAGS(prefswin_apply, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(prefswin_hbox), prefswin_apply, TRUE, TRUE, 0);

	add_input_plugins(GTK_CLIST(prefswin_audio_ilist));
	add_output_plugins(GTK_OPTION_MENU(prefswin_audio_olist));
	add_general_plugins(GTK_CLIST(prefswin_egplugins_glist));
	add_effect_plugins(GTK_OPTION_MENU(prefswin_egplugins_elist));
	add_vis_plugins(GTK_CLIST(prefswin_vplugins_list));
}

void prefswin_effect_cb(GtkWidget * w, gpointer item)
{
	EffectPlugin *cp;
	GList *effect;

	selected_eplugin = GPOINTER_TO_INT(item);
	effect = get_effect_list();
	cp = (EffectPlugin *) g_list_nth(effect, GPOINTER_TO_INT(item))->data;

	if (cp->configure != NULL)
		gtk_widget_set_sensitive(prefswin_egplugins_econfig, 1);
	else
		gtk_widget_set_sensitive(prefswin_egplugins_econfig, 0);

	if (cp->about != NULL)
		gtk_widget_set_sensitive(prefswin_egplugins_eabout, 1);
	else
		gtk_widget_set_sensitive(prefswin_egplugins_eabout, 0);
}

void prefswin_output_cb(GtkWidget * w, gpointer item)
{
	OutputPlugin *cp;
	GList *output;

	selected_oplugin = GPOINTER_TO_INT(item);
	output = get_output_list();
	cp = (OutputPlugin *) g_list_nth(output, GPOINTER_TO_INT(item))->data;

	if (cp->configure != NULL)
		gtk_widget_set_sensitive(prefswin_audio_oconfig, 1);
	else
		gtk_widget_set_sensitive(prefswin_audio_oconfig, 0);

	if (cp->about != NULL)
		gtk_widget_set_sensitive(prefswin_audio_oabout, 1);
	else
		gtk_widget_set_sensitive(prefswin_audio_oabout, 0);
}

void gen_module_description(gchar * file, gchar * desc, gchar ** full_desc)
{
	(*full_desc) = g_strdup_printf("%s   [%s]", desc, g_basename(file));
}

void add_output_plugins(GtkOptionMenu *omenu)
{
	GList *olist = get_output_list();
	GtkWidget *menu, *item;
	gchar *description;
	OutputPlugin *op, *cp = get_current_output_plugin();
	gint i = 0;

	if (!olist)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(omenu), FALSE);
		gtk_widget_set_sensitive(prefswin_audio_oconfig, FALSE);
		gtk_widget_set_sensitive(prefswin_audio_oabout, FALSE);
		return;
	}

	menu = gtk_menu_new();
	while (olist)
	{
		op = (OutputPlugin *) olist->data;

		if (olist->data == cp)
			selected_oplugin = i;

		gen_module_description(op->filename, op->description, &description);
		item = gtk_menu_item_new_with_label(description);
		g_free(description);

		gtk_signal_connect(GTK_OBJECT(item), "activate",
				   GTK_SIGNAL_FUNC(prefswin_output_cb),
				   GINT_TO_POINTER(i));
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);
		olist = olist->next;
		i++;
	}
	gtk_option_menu_remove_menu(omenu);
	gtk_option_menu_set_menu(omenu, menu);
	gtk_option_menu_set_history(omenu, selected_oplugin);

	if (cp->configure != NULL)
		gtk_widget_set_sensitive(prefswin_audio_oconfig, TRUE);
	else
		gtk_widget_set_sensitive(prefswin_audio_oconfig, FALSE);

	if (cp->about != NULL)
		gtk_widget_set_sensitive(prefswin_audio_oabout, TRUE);
	else
		gtk_widget_set_sensitive(prefswin_audio_oabout, FALSE);

}

void add_effect_plugins(GtkOptionMenu *emenu)
{
	GList *elist = get_effect_list();
	GtkWidget *menu, *item;
	gchar *description;
	EffectPlugin *ep, *cp = get_current_effect_plugin();
	gint i = 0;

	if (!elist)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(emenu), FALSE);
		gtk_widget_set_sensitive(prefswin_egplugins_euse_cbox, FALSE);
		gtk_widget_set_sensitive(prefswin_egplugins_econfig, FALSE);
		gtk_widget_set_sensitive(prefswin_egplugins_eabout, FALSE);
		return;
	}

	menu = gtk_menu_new();

	while (elist)
	{
		ep = (EffectPlugin *) elist->data;

		
		gen_module_description(ep->filename, ep->description, &description);

		item = gtk_menu_item_new_with_label(_(description));
		g_free(description);
		gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(prefswin_effect_cb), GINT_TO_POINTER(i));
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);
		if (elist->data == cp)
			selected_eplugin = i;
		elist = elist->next;
		i++;
	}

	gtk_option_menu_remove_menu(emenu);
	gtk_option_menu_set_menu(emenu, menu);
	gtk_option_menu_set_history(emenu, selected_eplugin);

	if (cp->configure != NULL)
		gtk_widget_set_sensitive(prefswin_egplugins_econfig, 1);
	else
		gtk_widget_set_sensitive(prefswin_egplugins_econfig, 0);

	if (cp->about != NULL)
		gtk_widget_set_sensitive(prefswin_egplugins_eabout, 1);
	else
		gtk_widget_set_sensitive(prefswin_egplugins_eabout, 0);
}

void add_general_plugins(GtkCList *clist)
{
	GList *glist = get_general_list();
	gchar *description, *temp;
	GeneralPlugin *gp;
	gint i = 0;

	gtk_clist_clear(clist);

	while (glist)
	{
		gp = (GeneralPlugin *) glist->data;
		gen_module_description(gp->filename, gp->description, &description);
		if (general_enabled(i))
		{
			temp = g_strconcat(description, _(" (enabled)"), NULL);
			g_free(description);
			description = temp;
		}

		gtk_clist_append(clist, &description);
		g_free(description);
		glist = glist->next;
		i++;
	}
	gtk_widget_set_sensitive(prefswin_egplugins_guse_cbox, 0);
	gtk_widget_set_sensitive(prefswin_egplugins_gconfig, 0);
	gtk_widget_set_sensitive(prefswin_egplugins_gabout, 0);
}

void add_vis_plugins(GtkCList *clist)
{
	GList *glist = get_vis_list();
	gchar *description, *temp;
	VisPlugin *vp;
	gint i = 0;

	gtk_clist_clear(clist);

	while (glist)
	{
		vp = (VisPlugin *) glist->data;
		gen_module_description(vp->filename, vp->description, &description);
		if (vis_enabled(i))
		{
			temp = g_strconcat(description, _(" (enabled)"), NULL);
			g_free(description);
			description = temp;
		}

		gtk_clist_append(clist, &description);
		g_free(description);
		glist = glist->next;
		i++;
	}
	gtk_widget_set_sensitive(prefswin_vplugins_use_cbox, 0);
	gtk_widget_set_sensitive(prefswin_vplugins_config, 0);
	gtk_widget_set_sensitive(prefswin_vplugins_about, 0);
}

void add_input_plugins(GtkCList *clist)
{
	GList *ilist = get_input_list();
	gchar *description, *temp;
	InputPlugin *ip;

	gtk_clist_clear(clist);
	while (ilist)
	{
		ip = (InputPlugin *) ilist->data;

		gen_module_description(ip->filename, ip->description, &description);
		if (g_list_find(disabled_iplugins, ip))
		{
			temp = g_strconcat(description, _(" (disabled)"), NULL);
			g_free(description);
			description = temp;
		}
		gtk_clist_append(clist, &description);
		g_free(description);
		ilist = ilist->next;
	}
	gtk_widget_set_sensitive(prefswin_audio_ie_cbox, 0);
	gtk_widget_set_sensitive(prefswin_audio_iconfig, 0);
	gtk_widget_set_sensitive(prefswin_audio_iabout, 0);
}

void show_prefs_window(void)
{
	char temp[10];

	if (GTK_WIDGET_VISIBLE(prefswin))
	{
		gdk_window_raise(prefswin->window);
		return;
	}

	is_opening = TRUE;

	gtk_entry_set_text(GTK_ENTRY(prefswin_options_font_entry), cfg.playlist_font);
	gtk_entry_set_text(GTK_ENTRY(prefswin_mainwin_font_entry), cfg.mainwin_font);
	gtk_entry_set_text(GTK_ENTRY(prefswin_title_entry), cfg.gentitle_format);
	sprintf(temp, "%u", cfg.snap_distance);
	gtk_entry_set_text(GTK_ENTRY(prefswin_options_sd_entry), temp);
	prefswin_options_read_data();
	sprintf(temp, "%u", cfg.pause_between_songs_time);
	gtk_entry_set_text(GTK_ENTRY(prefswin_options_pbs_entry), temp);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(prefswin_options_mouse_spin), cfg.mouse_change);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefswin_egplugins_euse_cbox), cfg.use_eplugins);

 	gtk_widget_show_all(prefswin);
	gtk_widget_grab_default(prefswin_ok);

	GDK_THREADS_LEAVE();
	while(g_main_iteration(FALSE));
	GDK_THREADS_ENTER();

	is_opening = FALSE;
}

void prefswin_show_vis_plugins_page(void)
{
	gtk_notebook_set_page(GTK_NOTEBOOK(prefswin_notebook), 2);
}


