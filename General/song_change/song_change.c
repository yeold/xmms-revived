#include "config.h"

#include "xmms/i18n.h"
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include <string.h>

#include "xmms/plugin.h"
#include "libxmms/configfile.h"
#include "libxmms/xmmsctrl.h"
#include "libxmms/formatter.h"

static void init(void);
static void cleanup(void);
static void configure(void);
static gint timeout_func(gpointer);
static gint timeout_tag = 0;
static gint previous_song = -1, previous_length = -2;
static gchar *cmd_line = NULL;
static gboolean possible_pl_end;
static gchar *cmd_line_end;

static GtkWidget *configure_win = NULL, *configure_vbox;
static GtkWidget *cmd_entry, *cmd_end_entry;

GeneralPlugin sc_gp =
{
	NULL,			/* handle */
	NULL,			/* filename */
	-1,			/* xmms_session */
	NULL,			/* Description */
	init,
	NULL,
	configure,
	cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
	sc_gp.description = g_strdup_printf(_("Song Change %s"), VERSION);
	return &sc_gp;
}

static void read_config(void)
{
	ConfigFile *cfgfile;

	g_free(cmd_line);
	g_free(cmd_line_end);
	cmd_line = NULL;
	cmd_line_end = NULL;

	if ((cfgfile = xmms_cfg_open_default_file()) != NULL)
	{
		xmms_cfg_read_string(cfgfile, "song_change", "cmd_line", &cmd_line);
		xmms_cfg_read_string(cfgfile, "song_change", "cmd_line_end", &cmd_line_end);
		xmms_cfg_free(cfgfile);
	}
}

static void init(void)
{
	read_config();

	previous_song = -1;
	previous_length = -2;
	timeout_tag = gtk_timeout_add(100, timeout_func, NULL);
}

static void cleanup(void)
{
	if (timeout_tag)
		gtk_timeout_remove(timeout_tag);
	timeout_tag = 0;
	if (cmd_line)
		g_free(cmd_line);
	if (cmd_line_end)
		g_free(cmd_line_end);
	cmd_line = NULL;
	cmd_line_end = NULL;
	signal(SIGCHLD, SIG_DFL);
}

static void configure_ok_cb(gpointer data)
{
	gchar *cmd, *cmd_end;
	ConfigFile *cfgfile;

	cmd = gtk_entry_get_text(GTK_ENTRY(cmd_entry));
	cmd_end = gtk_entry_get_text(GTK_ENTRY(cmd_end_entry));

	cfgfile = xmms_cfg_open_default_file();
	xmms_cfg_write_string(cfgfile, "song_change", "cmd_line", cmd);
	xmms_cfg_write_string(cfgfile, "song_change", "cmd_line_end", cmd_end);
	xmms_cfg_write_default_file(cfgfile);
	xmms_cfg_free(cfgfile);

	if (timeout_tag)
	{
		g_free(cmd_line);
		cmd_line = g_strdup(cmd);
		g_free(cmd_line_end);
		cmd_line_end = g_strdup(cmd_end);
	}
	gtk_widget_destroy(configure_win);
}

static void configure(void)
{
	GtkWidget *cmd_hbox, *cmd_label, *cmd_end_hbox, *cmd_end_label;
	GtkWidget *song_desc, *end_desc;
	GtkWidget *configure_bbox, *configure_ok, *configure_cancel;
	GtkWidget *song_frame, *song_vbox, *end_frame, *end_vbox;
	gchar *temp;
	
	if (configure_win)
		return;

	read_config();

	configure_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
	gtk_window_set_title(GTK_WINDOW(configure_win), _("Song Change Configuration"));
/*  	gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE); */
	
	gtk_container_set_border_width(GTK_CONTAINER(configure_win), 10);

	configure_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(configure_win), configure_vbox);

	song_frame = gtk_frame_new(_("Song change"));
	gtk_box_pack_start(GTK_BOX(configure_vbox), song_frame, FALSE, FALSE, 0);
	song_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(song_vbox), 5);
	gtk_container_add(GTK_CONTAINER(song_frame), song_vbox);
	gtk_widget_show(song_frame);
	gtk_widget_show(song_vbox);
	
	end_frame = gtk_frame_new(_("Playlist end"));
	gtk_box_pack_start(GTK_BOX(configure_vbox), end_frame, FALSE, FALSE, 0);
	end_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(end_vbox), 5);
	gtk_container_add(GTK_CONTAINER(end_frame), end_vbox);
	gtk_widget_show(end_frame);
	gtk_widget_show(end_vbox);
	
	temp = g_strdup_printf(_(
		"Shell-command to run when xmms changes song.  "
		"It can optionally include the string %%s which will be "
		"replaced by the new song title."));
	song_desc = gtk_label_new(temp);
	g_free(temp);
	gtk_label_set_justify(GTK_LABEL(song_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(song_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(song_vbox), song_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(song_desc), TRUE);
	gtk_widget_show(song_desc);
					  
	cmd_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(song_vbox), cmd_hbox, FALSE, FALSE, 0);

	cmd_label = gtk_label_new(_("Command:"));
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_label, FALSE, FALSE, 0);
	gtk_widget_show(cmd_label);

	cmd_entry = gtk_entry_new();
	if (cmd_line)
		gtk_entry_set_text(GTK_ENTRY(cmd_entry), cmd_line);
	gtk_widget_set_usize(cmd_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_hbox), cmd_entry, TRUE, TRUE, 0);
	gtk_widget_show(cmd_entry);
	gtk_widget_show(cmd_hbox);

	end_desc = gtk_label_new(_(
		"Shell-command to run when xmms reaches the end "
		"of the playlist."));
	gtk_label_set_justify(GTK_LABEL(end_desc), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(end_desc), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(end_vbox), end_desc, FALSE, FALSE, 0);
	gtk_label_set_line_wrap(GTK_LABEL(end_desc), TRUE);
	gtk_widget_show(end_desc);
					  
	cmd_end_hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(end_vbox), cmd_end_hbox, FALSE, FALSE, 0);

	cmd_end_label = gtk_label_new(_("Command:"));
	gtk_box_pack_start(GTK_BOX(cmd_end_hbox), cmd_end_label, FALSE, FALSE, 0);
	gtk_widget_show(cmd_end_label);

	cmd_end_entry = gtk_entry_new();
	if (cmd_line_end)
		gtk_entry_set_text(GTK_ENTRY(cmd_end_entry), cmd_line_end);
	gtk_widget_set_usize(cmd_end_entry, 200, -1);
	gtk_box_pack_start(GTK_BOX(cmd_end_hbox), cmd_end_entry, TRUE, TRUE, 0);
	gtk_widget_show(cmd_end_entry);
	gtk_widget_show(cmd_end_hbox);

	configure_bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(configure_bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(configure_bbox), 5);
	gtk_box_pack_start(GTK_BOX(configure_vbox), configure_bbox, FALSE, FALSE, 0);

	configure_ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect(GTK_OBJECT(configure_ok), "clicked", GTK_SIGNAL_FUNC(configure_ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(configure_ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(configure_bbox), configure_ok, TRUE, TRUE, 0);
	gtk_widget_show(configure_ok);
	gtk_widget_grab_default(configure_ok);

	configure_cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(configure_cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
	GTK_WIDGET_SET_FLAGS(configure_cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(configure_bbox), configure_cancel, TRUE, TRUE, 0);
	gtk_widget_show(configure_cancel);
	gtk_widget_show(configure_bbox);
	gtk_widget_show(configure_vbox);
	gtk_widget_show(configure_win);
}

static void bury_child(int signal)
{
	waitpid(-1, NULL, WNOHANG);
}

static void execute_command(gchar *cmd)
{
	gchar *argv[4] = {"/bin/sh", "-c", NULL, NULL};
	gint i;
	argv[2] = cmd;
	signal(SIGCHLD, bury_child);
	if (fork() == 0)
	{
		/* We don't want this process to hog the audio device etc */
		for (i = 3; i < 255; i++)
			close(i);
		execv("/bin/sh", argv);
	}
}

/*
 * escape_shell_chars()
 *
 * Escapes characters that are special to the shell inside double quotes.
 */

static gchar * escape_shell_chars(gchar * string)
{
	const gchar *special = "$`\"\\"; /* Characters to escape */
	gchar *in = string, *out;
	gchar *escaped;
	gint num = 0;

	while (*in != '\0')
		if (strchr(special, *in++))
			num++;

	escaped = g_malloc(strlen(string) + num + 1);

	in = string;
	out = escaped;

	while (*in != '\0')
	{
		if (strchr(special, *in))
			*out++ = '\\';
		*out++ = *in++;
	}
	*out = '\0';

	return escaped;
}

static gint timeout_func(gpointer data)
{
	gint pos, length, rate, freq, nch;
	gchar *str, *shstring = NULL, *temp, numbuf[16];
	gboolean playing, run_end_cmd = FALSE;
	Formatter *formatter;

	GDK_THREADS_ENTER();

	playing = xmms_remote_is_playing(sc_gp.xmms_session);
	pos = xmms_remote_get_playlist_pos(sc_gp.xmms_session);
	length = xmms_remote_get_playlist_time(sc_gp.xmms_session, pos);
	
	/* Format codes:
	 *
	 *   F - frequency (in hertz)
	 *   c - number of channels
	 *   f - filename (full path)
	 *   l - length (in milliseconds)
	 *   n - name
	 *   r - rate (in bits per second)
	 *   s - name (since everyone's used to it)
	 *   t - playlist position (%02d)
	 */
	if (pos != previous_song || length != previous_length)
	{
		previous_song = pos;
		previous_length = length;
		if (cmd_line && strlen(cmd_line) > 0)
		{
			formatter = xmms_formatter_new();
			str = xmms_remote_get_playlist_title(sc_gp.xmms_session, pos);
			if (str)
			{
				temp = escape_shell_chars(str);
				xmms_formatter_associate(formatter, 's', temp);
				xmms_formatter_associate(formatter, 'n', temp);
				g_free(str);
				g_free(temp);
			}
			str = xmms_remote_get_playlist_file(sc_gp.xmms_session, pos);
			if (str)
			{
				temp = escape_shell_chars(str);
				xmms_formatter_associate(formatter, 'f', temp);
				g_free(str);
				g_free(temp);
			}
			sprintf(numbuf, "%02d", pos + 1);
			xmms_formatter_associate(formatter, 't', numbuf);
			sprintf(numbuf, "%d", length);
			xmms_formatter_associate(formatter, 'l', numbuf);
			xmms_remote_get_info(sc_gp.xmms_session, &rate, &freq, &nch);
			sprintf(numbuf, "%d", rate);
			xmms_formatter_associate(formatter, 'r', numbuf);
			sprintf(numbuf, "%d", freq);
			xmms_formatter_associate(formatter, 'F', numbuf);
			sprintf(numbuf, "%d", nch);
			xmms_formatter_associate(formatter, 'c', numbuf);
			shstring = xmms_formatter_format(formatter, cmd_line);
			xmms_formatter_destroy(formatter);
		}
	}

	if (playing)
	{
		gint playlist_length = xmms_remote_get_playlist_length(sc_gp.xmms_session);
		if (pos + 1 == playlist_length)
			possible_pl_end = TRUE;
		else
			possible_pl_end = FALSE;
	}
	else if (possible_pl_end)
	{
		if (pos == 0 && cmd_line_end && strlen(cmd_line_end) > 0)
			run_end_cmd = TRUE;
		possible_pl_end = FALSE;
	}

	if (shstring)
	{
		execute_command(shstring);
		g_free(shstring); /* FIXME: This can possibly be freed too early */
	}

	if (run_end_cmd)
		execute_command(cmd_line_end);

	GDK_THREADS_LEAVE();

	return TRUE;
}
