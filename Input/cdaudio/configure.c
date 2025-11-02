#include "xmms/i18n.h"
#include "cdaudio.h"
#include "libxmms/titlestring.h"

static GtkWidget *cdda_configure_win;
static GtkWidget *volume_oss, *dev_entry, *dev_dir_entry, *cdi_name;
static GtkWidget *cdi_use_cddb, *cdi_cddb_server, *cdi_use_cdin, *cdi_cdin_server;
GtkWidget *cdi_name_enable_vbox, *cdi_name_override;

void cdda_cddb_show_server_dialog(GtkWidget *w, gpointer data);
void cdda_cddb_show_network_window(GtkWidget *w, gpointer data);
void cdda_cddb_set_server(gchar *new_server);

static void cdda_configurewin_ok_cb(GtkWidget * w, gpointer data)
{
	ConfigFile *cfgfile;
	gchar *tmp;

	g_free(cdda_cfg.device);
	cdda_cfg.device = g_strdup(gtk_entry_get_text(GTK_ENTRY(dev_entry)));

	g_free(cdda_cfg.directory);
	tmp = gtk_entry_get_text(GTK_ENTRY(dev_dir_entry));

	if (tmp[strlen(tmp) - 1] == '/')
		cdda_cfg.directory = g_strdup(tmp);
	else
		cdda_cfg.directory = g_strconcat(tmp, "/", NULL);
	cdda_cfg.use_oss_mixer = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(volume_oss));

	cdda_cfg.title_override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cdi_name_override));
	g_free(cdda_cfg.name_format);
	cdda_cfg.name_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(cdi_name)));

	cdda_cfg.use_cddb = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cdi_use_cddb));
	cdda_cddb_set_server(gtk_entry_get_text(GTK_ENTRY(cdi_cddb_server)));

	cdda_cfg.use_cdin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cdi_use_cdin));
	if (strcmp(cdda_cfg.cdin_server, gtk_entry_get_text(GTK_ENTRY(cdi_cdin_server))))
	{
		g_free(cdda_cfg.cdin_server);
		cdda_cfg.cdin_server = g_strdup(gtk_entry_get_text(GTK_ENTRY(cdi_cdin_server)));
	}

	cfgfile = xmms_cfg_open_default_file();

	xmms_cfg_write_string(cfgfile, "CDDA", "device", cdda_cfg.device);
	xmms_cfg_write_string(cfgfile, "CDDA", "directory", cdda_cfg.directory);
	xmms_cfg_write_boolean(cfgfile, "CDDA", "use_oss_mixer", cdda_cfg.use_oss_mixer);
	xmms_cfg_write_boolean(cfgfile, "CDDA", "title_override", cdda_cfg.title_override);
	xmms_cfg_write_string(cfgfile, "CDDA", "name_format", cdda_cfg.name_format);
	xmms_cfg_write_boolean(cfgfile, "CDDA", "use_cddb", cdda_cfg.use_cddb);
	xmms_cfg_write_string(cfgfile, "CDDA", "cddb_server", cdda_cfg.cddb_server);
	xmms_cfg_write_int(cfgfile, "CDDA", "cddb_protocol_level", cdda_cfg.cddb_protocol_level);
	xmms_cfg_write_boolean(cfgfile, "CDDA", "use_cdin", cdda_cfg.use_cdin);
	xmms_cfg_write_string(cfgfile, "CDDA", "cdin_server", cdda_cfg.cdin_server);
	xmms_cfg_write_default_file(cfgfile);
	xmms_cfg_free(cfgfile);

	gtk_widget_destroy(cdda_configure_win);
}

static void cdi_name_override_cb(GtkWidget * w, gpointer data)
{
	gtk_widget_set_sensitive(cdi_name_enable_vbox, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cdi_name_override)));
}

void cdda_configure(void)
{
	GtkWidget *vbox, *notebook;
	GtkWidget *dev_vbox, *dev_frame, *dev_table, *dev_label, *dev_dir_label;
	GtkWidget *volume_frame, *volume_box, *volume_drive;
	GtkWidget *cdi_vbox;
	GtkWidget *cdi_cddb_frame, *cdi_cddb_vbox, *cdi_cddb_hbox;
	GtkWidget *cdi_cddb_server_hbox, *cdi_cddb_server_label;
	GtkWidget *cdi_cddb_server_list, *cdi_cddb_debug_win;
	GtkWidget *cdi_cdin_frame, *cdi_cdin_vbox;
	GtkWidget *cdi_cdin_server_hbox, *cdi_cdin_server_label;
	GtkWidget *cdi_name_frame, *cdi_name_vbox, *cdi_name_hbox;
	GtkWidget *cdi_name_label, *cdi_desc;
	GtkWidget *bbox, *ok, *cancel;

	if (cdda_configure_win)
		return;
	
	cdda_configure_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(cdda_configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &cdda_configure_win);
	gtk_window_set_title(GTK_WINDOW(cdda_configure_win), _("CD Audio Player Configuration"));
	gtk_window_set_policy(GTK_WINDOW(cdda_configure_win), FALSE, FALSE, FALSE);
	gtk_window_set_position(GTK_WINDOW(cdda_configure_win), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(cdda_configure_win), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(cdda_configure_win), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	/*
	 * Device config
	 */
	dev_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(dev_vbox), 5);

	dev_frame = gtk_frame_new(_("Device:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), dev_frame, FALSE, FALSE, 0);

	dev_table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dev_table), 5);
	gtk_container_add(GTK_CONTAINER(dev_frame), dev_table);
	gtk_table_set_row_spacings(GTK_TABLE(dev_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(dev_table), 5);

	dev_label = gtk_label_new(_("Device:"));
	gtk_misc_set_alignment(GTK_MISC(dev_label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dev_table), dev_label, 0, 1, 0, 1);

	dev_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(dev_entry), cdda_cfg.device);
	gtk_table_attach_defaults(GTK_TABLE(dev_table), dev_entry, 1, 2, 0, 1);

	dev_dir_label = gtk_label_new(_("Directory:"));
	gtk_misc_set_alignment(GTK_MISC(dev_dir_label), 1.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(dev_table), dev_dir_label, 0, 1, 1, 2);

	dev_dir_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(dev_dir_entry), cdda_cfg.directory);
	gtk_table_attach_defaults(GTK_TABLE(dev_table), dev_dir_entry, 1, 2, 1, 2);

	/*
	 * Volume config
	 */

	volume_frame = gtk_frame_new(_("Volume setting:"));
	gtk_box_pack_start(GTK_BOX(dev_vbox), volume_frame, FALSE, FALSE, 0);

	volume_box = gtk_vbox_new(5, FALSE);
	gtk_container_add(GTK_CONTAINER(volume_frame), volume_box);

	volume_oss = gtk_radio_button_new_with_label(NULL, _("OSS Mixer"));
	gtk_box_pack_start(GTK_BOX(volume_box), volume_oss, FALSE, FALSE, 0);
#if !defined(HAVE_SYS_SOUNDCARD_H) && !defined(HAVE_MACHINE_SOUNDCARD_H)
	gtk_widget_set_sensitive(volume_oss, FALSE);
#endif
	if (cdda_cfg.use_oss_mixer)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(volume_oss), TRUE);

	volume_drive = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(volume_oss), _("CDROM drive"));
	gtk_box_pack_start(GTK_BOX(volume_box), volume_drive, FALSE, FALSE, 0);
	if (!cdda_cfg.use_oss_mixer)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(volume_drive), TRUE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dev_vbox, gtk_label_new(_("Device")));

	/*
	 * CD Info config
	 */
	cdi_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(cdi_vbox), 5);


	/* CDDB */
	cdi_cddb_frame = gtk_frame_new(_("CDDB:"));
	gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_cddb_frame, FALSE, FALSE, 0);

	cdi_cddb_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(cdi_cddb_vbox), 5);
	gtk_container_add(GTK_CONTAINER(cdi_cddb_frame), cdi_cddb_vbox);

	cdi_cddb_hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(cdi_cddb_hbox), 0);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_vbox), cdi_cddb_hbox, FALSE, FALSE, 0);
	cdi_use_cddb = gtk_check_button_new_with_label(_("Use CDDB"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_use_cddb), cdda_cfg.use_cddb);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_hbox), cdi_use_cddb, FALSE, FALSE, 0);
	cdi_cddb_server_list = gtk_button_new_with_label(_("Get server list"));
	gtk_box_pack_end(GTK_BOX(cdi_cddb_hbox), cdi_cddb_server_list, FALSE, FALSE, 0);
	cdi_cddb_debug_win = gtk_button_new_with_label(_("Show network window"));
	gtk_signal_connect(GTK_OBJECT(cdi_cddb_debug_win), "clicked", GTK_SIGNAL_FUNC(cdda_cddb_show_network_window), NULL);
	gtk_box_pack_end(GTK_BOX(cdi_cddb_hbox), cdi_cddb_debug_win, FALSE, FALSE, 0);

	cdi_cddb_server_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_vbox), cdi_cddb_server_hbox, FALSE, FALSE, 0);

	cdi_cddb_server_label = gtk_label_new(_("CDDB server:"));
	gtk_box_pack_start(GTK_BOX(cdi_cddb_server_hbox), cdi_cddb_server_label, FALSE, FALSE, 0);

	cdi_cddb_server = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cdi_cddb_server), cdda_cfg.cddb_server);
	gtk_box_pack_start(GTK_BOX(cdi_cddb_server_hbox), cdi_cddb_server, TRUE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(cdi_cddb_server_list), "clicked", GTK_SIGNAL_FUNC(cdda_cddb_show_server_dialog), cdi_cddb_server);

	/*
	 * CDindex
	 */
	cdi_cdin_frame = gtk_frame_new(_("CD Index:"));
	gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_cdin_frame, FALSE, FALSE, 0);

	cdi_cdin_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(cdi_cdin_vbox), 5);
	gtk_container_add(GTK_CONTAINER(cdi_cdin_frame), cdi_cdin_vbox);

	cdi_use_cdin = gtk_check_button_new_with_label(_("Use CD Index"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_use_cdin), cdda_cfg.use_cdin);
	gtk_box_pack_start(GTK_BOX(cdi_cdin_vbox), cdi_use_cdin, FALSE, FALSE, 0);

	cdi_cdin_server_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(cdi_cdin_vbox), cdi_cdin_server_hbox, FALSE, FALSE, 0);

	cdi_cdin_server_label = gtk_label_new(_("CD Index server:"));
	gtk_box_pack_start(GTK_BOX(cdi_cdin_server_hbox), cdi_cdin_server_label, FALSE, FALSE, 0);

	cdi_cdin_server = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cdi_cdin_server), cdda_cfg.cdin_server);
	gtk_box_pack_start(GTK_BOX(cdi_cdin_server_hbox), cdi_cdin_server, TRUE, TRUE, 0);
#ifndef WITH_CDINDEX
	gtk_widget_set_sensitive(cdi_cdin_frame, FALSE);
#endif	

	/*
	 * Track names
	 */
	cdi_name_frame = gtk_frame_new(_("Track names:"));
	gtk_box_pack_start(GTK_BOX(cdi_vbox), cdi_name_frame, FALSE, FALSE, 0);

	cdi_name_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(cdi_name_frame), cdi_name_vbox);
	gtk_container_border_width(GTK_CONTAINER(cdi_name_vbox), 5);
	cdi_name_override = gtk_check_button_new_with_label(_("Override generic titles"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cdi_name_override), cdda_cfg.title_override);
	gtk_box_pack_start(GTK_BOX(cdi_name_vbox), cdi_name_override, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(cdi_name_override), "clicked", cdi_name_override_cb, NULL);

	cdi_name_enable_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(cdi_name_vbox), cdi_name_enable_vbox);
	gtk_widget_set_sensitive(cdi_name_enable_vbox, cdda_cfg.title_override);

	cdi_name_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(cdi_name_enable_vbox), cdi_name_hbox, FALSE, FALSE, 0);
	cdi_name_label = gtk_label_new(_("Name format:"));
	gtk_box_pack_start(GTK_BOX(cdi_name_hbox), cdi_name_label, FALSE, FALSE, 0);
	cdi_name = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(cdi_name), cdda_cfg.name_format);
	gtk_box_pack_start(GTK_BOX(cdi_name_hbox), cdi_name, TRUE, TRUE, 0);

	cdi_desc = xmms_titlestring_descriptions("patn", 2);
	gtk_box_pack_start(GTK_BOX(cdi_name_enable_vbox), cdi_desc, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cdi_vbox, gtk_label_new(_("CD Info")));

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(cdda_configurewin_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(cdda_configure_win));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(cdda_configure_win);
}
