/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 2000-2001  Haavard Kvaalen <havardk@xmms.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licensse as published by
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
#include "xmms_logo.xpm"

/*
 * The different sections are kept in alphabetical order
 */

static const gchar *credit_text[] =
{N_("Main Programming:") ,
	N_("Peter Alm"),
	NULL,
 N_("Additional Programming:"),
/* for languages that can't display 'a ring' (å) replace it with "aa" */
	N_("Håvard Kvålen"),
	N_("Derrik Pates"),
	NULL,
 N_("With Additional Help:"),
	N_("Sean Atkinson"),
	N_("Jorn Baayen"),
	N_("James M. Cape"),
	N_("Anders Carlsson (effect plugins)"),
	N_("Chun-Chung Chen (xfont patch)"),
	N_("Tim Ferguson (joystick plugin)"),
	N_("Ben Gertzfield"),
	N_("Vesa Halttunen"),
	N_("Logan Hanks"),
	N_("Eric L. Hernes (FreeBSD patches)"),
	N_("Ville Herva"),
        N_("Michael Hipp and others (MPG123 engine)"),
/* for languages that can't display 'a diaeresis' (ä) replace it with "ae" */
	N_("Olle Hällnäs (compiling fixes)"),
        N_("David Jacoby"),
	N_("Osamu Kayasono (3DNow!)"),
	N_("Lyle B Kempler"),
	N_("J. Nick Koston (MikMod plugin)"),
	N_("Aaron Lehmann"),
	N_("Johan Levin (echo + stereo plugin)"),
	N_("Eric Lindvall"),
	N_("Colin Marquardt"),
	N_("Willem Monsuwe"),
	N_("John Riddoch (Solaris plugin)"),
	N_("Pablo Saratxaga (i18n)"),
	N_("Carl van Schaik (pro logic plugin)"),
/* for languages that can't display 'o diaeresis' (ö) replace it with "oe" */
	N_("Jörg Schuler"),
	N_("Charles Sielski (irman plugin)"),
	N_("Espen Skoglund"),
	N_("Kimura Takuhiro (3DNow!)"),
	N_("Zinx Verituse"),
	N_("Ryan Weaver (RPMs among other things)"),
	N_("Chris Wilson"),
	N_("Dave Yearke"),
	N_("Stephan K. Zitz"),
	NULL,
 N_("Homepage and Graphics:"),
	N_("Thomas Nilsson"),
	NULL,
 N_("Support and Docs:"),
/* for languages that can't display 'a diaeresis' (ä) replace it with "ae" */
	N_("Olle Hällnäs"),
	NULL, NULL};

static const char *translators[] =
{
	N_("Afrikaans:"),
	/* for languages that can't display 'e acute' (é) replace it with "e" */
	N_("Schalk W. Cronjé "), NULL,
	N_("Azerbaijani:"), N_("Vasif Ismailoglu"), NULL,
	N_("Basque:"), N_("Iñigo Salvador Azurmendi"), NULL,
	N_("Brazilian Portuguese:"), N_("Juan Carlos Castro y Castro"),	NULL,
	N_("Bulgarian:"), N_("Yovko D. Lambrev"), NULL,
	N_("Catalan:"), N_("Quico Llach"), NULL,
	N_("Chinese:"), N_("Chun-Chung Chen"), N_("Jouston Huang"),
	N_("Andrew Lee"), N_("Chih-Wei Huang"), N_("Danny Zeng"), NULL,
	N_("Croatian:"), N_("Vlatko Kosturjak"), N_("Vladimir Vuksan"), NULL,
	N_("Czech:"),
	/* for languages that can't display 'i acute' (í) replace it with "i" */
	N_("Vladimír Marek"),
	N_("Radek Vybiral"), NULL,
	N_("Danish:"), N_("Nikolaj Berg Amondsen"), N_("Troels Liebe Bentsen"),
	N_("Kenneth Christiansen"), N_("Keld Simonsen"), NULL,
	N_("Dutch:"), N_("Jorn Baayen"), N_("Wilmer van der Gaast"),
	N_("Tom Laermans"), NULL,
	N_("Esperanto:"), N_("D. Dale Gulledge"), NULL,
	N_("German:"), N_("Colin Marquardt"), N_("Stefan Siegel"), NULL,
	N_("Greek:"), N_("Kyritsis Athanasios"), NULL,
	N_("French:"), N_("Arnaud Boissinot"), N_("Eric Fernandez-Bellot"), NULL,
	N_("Galician:"),
	/* for languages that can't display 'i acute' (í) replace it with "i" */
	N_("Alberto García"),
	/* for languages that can't display 'a acute' (á) replace it with "a" */
	N_("David Fernández Vaamonde"), NULL,
	N_("Hungarian:"), N_("Arpad Biro"), NULL,
	N_("Indonesian:"), N_("Budi Rachmanto"), NULL,
	N_("Irish:"), N_("Alastair McKinstry"), NULL,
	N_("Italian:"), N_("Paolo Lorenzin"), NULL,
	N_("Japanese:"), N_("Hiroshi Takekawa"), NULL,
	N_("Korean:"), N_("Jaegeum Choe"), N_("Sang-Jin Hwang"),
	N_("Byeong-Chan Kim"), N_("Man-Yong Lee"), NULL,
	N_("Lithuanian:"), N_("Gediminas Paulauskas"), NULL,
	N_("Latvian:"),
	/* I18N: The "n" should be replaced with 'n cedilla' (ò) if it
           can be displayed */
	/* I18N: The last "s" should be replaced with 's caron' (ð) if
           it can be displayed */
	N_("Juris Kudins"),
	N_("Vitauts Stochka"), NULL,
	N_("Norwegian:"),
	/* for languages that can't display 'o slash' (ø) replace it with "oe" */
	N_("Andreas Bergstrøm"),
	N_("Terje Bjerkelia"), N_("Håvard Kvålen"), N_("Roy-Magne Mo"),
	N_("Espen Skoglund"), NULL,
	N_("Polish:"), N_("Grzegorz Kowal"), NULL,
	N_("Portuguese:"), N_("Jorge Costa"), NULL,
	N_("Romanian:"), N_("Florin Grad"),
	/* I18N: The "s" should be replaced with 's cedilla' (º) if it
           can be displayed */
	N_("Misu Moldovan"), NULL,
	N_("Russian:"), N_("Valek Filippov"), N_("Alexandr P. Kovalenko"),
	N_("Maxim Koshelev"), N_("Aleksey Smirnov"), NULL,
	N_("Serbian:"), N_("Tomislav Jankovic"), NULL,
	N_("Slovak:"), N_("Pavol Cvengros"),
	N_("Tomas Hornocek"), N_("Jan Matis"), NULL,
	N_("Spanish:"), N_("Fabian Mandelbaum"), N_("Jordi Mallach"),
	/* for languages that can't display 'i acute' (í) replace it with "i" */
	N_("Juan Manuel García Molina"), NULL,
	N_("Swedish:"), N_("David Hedbor"), N_("Olle Hällnäs"),
	N_("Thomas Nilsson"),	N_("Christian Rose"), N_("Fuad Sabanovic"), NULL,
	N_("Tajik:"), N_("Trinh Minh Thanh"), NULL,
	N_("Thai:"), N_("Supphachoke Suntiwichaya"), NULL, 
	N_("Turkish:"), N_("Nazmi Savga"),
	/* I18N: The "i" should be replaced with 'dotless i' (ý) if it
           can be displayed */
	/* for languages that can't display 'O diaeresis' (Ö) replace it with "O" */
	N_("Ömer Fadil Usta"), NULL,
	N_("Ukrainian:"), N_("Dmytro Koval'ov"), NULL,
	N_("Vietnamese:"), N_("Roger Kovacs"), N_("Dilshod Marupov"), NULL,
	N_("Walloon:"), N_("Lucyin Mahin"), N_("Pablo Saratxaga"), NULL,
	NULL
};

static GtkWidget* generate_credit_list(const char *text[], gboolean sec_space)
{
	GtkWidget *clist, *scrollwin;
	int i = 0;

	clist = gtk_clist_new(2);

	while (text[i])
	{
		gchar *temp[2];
		guint row;
		
		temp[0] = gettext(text[i++]);
		temp[1] = gettext(text[i++]);
		row = gtk_clist_append(GTK_CLIST(clist), temp);
		gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
		temp[0] = "";
		while (text[i])
		{
			temp[1] = gettext(text[i++]);
			row = gtk_clist_append(GTK_CLIST(clist), temp);
			gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
		}
		i++;
		if (text[i] && sec_space)
		{
			temp[1] = "";
			row = gtk_clist_append(GTK_CLIST(clist), temp);
			gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
		}
	}
	gtk_clist_columns_autosize(GTK_CLIST(clist));
	gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_RIGHT);
	
	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
				       GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(scrollwin), clist);
	gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 10);
	gtk_widget_set_usize(scrollwin, -1, 120);

	return scrollwin;
}


void show_about_window(void)
{
	static GtkWidget *about_window = NULL;
	static GdkPixmap *xmms_logo_pmap = NULL, *xmms_logo_mask = NULL;

	GtkWidget *about_vbox, *about_notebook;
	GtkWidget *about_credits_logo_box, *about_credits_logo_frame;
	GtkWidget *about_credits_logo;
	GtkWidget *bbox, *close_btn;
	GtkWidget *label, *list;
	gchar *text;

	if (about_window)
		return;
	
	about_window = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_window_set_title(GTK_WINDOW(about_window), _("About XMMS"));
	gtk_window_set_policy(GTK_WINDOW(about_window), FALSE, TRUE, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);
	gtk_signal_connect(GTK_OBJECT(about_window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about_window);
	gtk_widget_realize(about_window);
	
	about_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(about_window), about_vbox);
	
	if (!xmms_logo_pmap)
		xmms_logo_pmap =
			gdk_pixmap_create_from_xpm_d(about_window->window,
						     &xmms_logo_mask, NULL,
						     xmms_logo);
	
	about_credits_logo_box = gtk_hbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(about_vbox), about_credits_logo_box,
			   FALSE, FALSE, 0);
	
	about_credits_logo_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
				  GTK_SHADOW_OUT);
	gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
			   about_credits_logo_frame, FALSE, FALSE, 0);
	
	about_credits_logo = gtk_pixmap_new(xmms_logo_pmap, xmms_logo_mask);
	gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
			  about_credits_logo);
	
	text = g_strdup_printf(_("XMMS %s - Cross platform multimedia player"),
			       VERSION);
	label = gtk_label_new(text);
	g_free(text);
	
	gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);
	
	label = gtk_label_new(_("Copyright (C) 1997-2001 4front Technologies"));
	gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);
	
	about_notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(about_vbox), about_notebook, TRUE, TRUE, 0);
	
	list = generate_credit_list(credit_text, TRUE);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
				 gtk_label_new(_("Credits")));

	list = generate_credit_list(translators, FALSE);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
				 gtk_label_new(_("Translators")));
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(about_vbox), bbox, FALSE, FALSE, 0);

	close_btn = gtk_button_new_with_label(_("Close"));
	gtk_signal_connect_object(GTK_OBJECT(close_btn), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(about_window));
	GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), close_btn, TRUE, TRUE, 0);
	gtk_widget_grab_default(close_btn);

	gtk_widget_show_all(about_window);
}
