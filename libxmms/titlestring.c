/*
 * Copyright (C) 2001,  Espen Skoglund <esk@ira.uka.de>
 * Copyright (C) 2001,  Haavard Kvaalen <havardk@xmms.org>
 *                
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *                
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "titlestring.h"
#include "../xmms/i18n.h"

#define CHECK(input, field) \
	(((char *) &input->field - (char *) input) < input->__size)

#define VS(input, field) (CHECK(input, field) ? input->field : NULL)
#define VI(input, field) (CHECK(input, field) ? input->field : 0)


gchar *xmms_get_titlestring(gchar *fmt, TitleInput *input)
{
	gchar c, *p, outbuf[256], convert[16], *string;
	gint numdigits, numpr, val, size, i;
	gint f_left, f_space, f_zero, someflag, width, precision;
	gboolean did_output = FALSE;
	char digits[] = "0123456789";

#define PUTCH(ch)			\
	do {				\
		if ( size-- <= 0 )	\
			goto Done;	\
		*p++ = (ch);		\
	} while (0)

#define LEFTPAD(num)						\
	do {							\
		int cnt = (num);				\
		if ( ! f_left && cnt > 0 )			\
			while ( cnt-- > 0 )			\
				PUTCH(f_zero ? '0' : ' ');	\
	} while (0)

#define RIGHTPAD(num)						\
	do {							\
		int cnt = (num);				\
		if ( f_left && cnt > 0 )			\
			while ( cnt-- > 0 )			\
				PUTCH( ' ' );			\
	} while (0)

	size = sizeof (outbuf) - 1;
	p = outbuf;

	if ( fmt == NULL )
		return NULL;

	while (size > 0)
	{
		/* Copy characters until we encounter '%'. */
		while ((c = *fmt++) != '%')
		{
			if (size-- <= 0 || c == '\0')
				goto Done;
			*p++ = c;
		}

		f_left = f_space = f_zero = 0;
		someflag = 1;


		/* Parse flags. */
		while (someflag)
		{
			switch (*fmt)
			{
				case '-':
					f_left = 1;
					fmt++;
					break;
				case ' ':
					f_space = 1;
					fmt++;
					break;
				case '0':
					f_zero = 1;
					fmt++;
					break;
				default:
					someflag = 0;
					break;
			}
		}


		/* Parse field width. */
		if ((c = *fmt) >= '0' && c <= '9')
		{
			width = 0;
			while ((c = *fmt++) >= '0' && c <= '9')
			{
				width *= 10;
				width += c - '0';
			}
			fmt--;
		}
		else
			width = -1;


		/* Parse precision. */
		if (*fmt == '.')
		{
			if ((c = *++fmt) >= '0' && c <= '9')
			{
				precision = 0;
				while ((c = *fmt++) >= '0' && c <= '9')
				{
					precision *= 10;
					precision += c - '0';
				}
				fmt--;
			}
			else
				precision = -1;
		}
		else
			precision = -1;


		/* Parse format conversion. */
		switch (c = *fmt++)
		{
		case 'a':
			string = VS(input, album_name);
			goto Print_string;
		case 'c':
			string = VS(input, comment);
			goto Print_string;
		case 'd':
			string = VS(input, date);
			goto Print_string;
		case 'e':
			string = VS(input, file_ext);
			goto Print_string;
		case 'f':
			string = VS(input, file_name);
			goto Print_string;
		case 'F':
			string = VS(input, file_path);
			goto Print_string;
		case 'g':
			string = VS(input, genre);
			goto Print_string;
		case 'n':
			val = VI(input, track_number);
			goto Print_number;
		case 'p':
			string = VS(input, performer);
			goto Print_string;
		case 't':
			string = VS(input, track_name);

		Print_string:
			if ( string == NULL )
				break;
			did_output = TRUE;

			numpr = 0;
			if (width > 0)
			{
				/* Calculate printed size. */
				numpr = strlen(string);
				if (precision >= 0 && precision < numpr)
					numpr = precision;

				LEFTPAD(width - numpr);
			}

			/* Insert string. */
			if (precision >= 0)
			{
				while (precision-- > 0 &&
				       (c = *string++) != '\0')
					PUTCH(c);
			}
			else
			{
				while ((c = *string++) != '\0')
					PUTCH(c);
			}

			RIGHTPAD(width - numpr);
			break;

		case 'y':
			val = VI(input, year);

		Print_number:
			if ( val == 0 )
				break;
			if ( c != 'N' )
				did_output = TRUE;

			/* Create reversed number string. */
			numdigits = 0;
			do
			{
				convert[numdigits++] = digits[val % 10];
				val /= 10;
			}
			while (val > 0);

			numpr = numdigits > precision ? numdigits : precision;

			/* Insert left padding. */
			if (!f_left && width > numpr)
			{
				if (f_zero)
					numpr = width;
				else
					for (i = width - numpr; i-- > 0;)
						PUTCH(' ');
			}

			/* Insert zero padding. */
			for (i = numpr - numdigits; i-- > 0;)
				PUTCH('0');

			/* Insert number. */
			while (numdigits > 0)
				PUTCH(convert[--numdigits]);

			RIGHTPAD(width - numpr);
			break;

		case '%':
			PUTCH('%');
			break;

		default:
			PUTCH('%');
			PUTCH(c);
			break;
		}
	}
	
      Done:
	*p = '\0';

	return did_output ? g_strdup(outbuf) : NULL;
}

static struct tagdescr {
	char tag;
	char *description;
} descriptions[] = {
	{'p', N_("Performer/Artist")},
	{'a', N_("Album")},
	{'g', N_("Genre")},
	{'f', N_("File name")},
	{'F', N_("File path")},
	{'e', N_("File extension")},
	{'t', N_("Track name")},
	{'n', N_("Track number")},
	{'d', N_("Date")},
	{'y', N_("Year")},
	{'c', N_("Comment")},
};

GtkWidget* xmms_titlestring_descriptions(char* tags, int columns)
{
	GtkWidget *table, *label;
	gchar tagstr[5];
	gint num = strlen(tags);
	int r, c, i;

	g_return_val_if_fail(tags != NULL, NULL);
	g_return_val_if_fail(columns <= num, NULL);

	table = gtk_table_new((num + columns - 1) / columns, columns * 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);
	
	for (c = 0; c < columns; c++)
	{
		for (r = 0; r < (num + columns - 1 - c) / columns; r++)
		{
			sprintf(tagstr, "%%%c:", *tags);
			label = gtk_label_new(tagstr);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_table_attach(GTK_TABLE(table), label, 2 * c, 2 * c + 1, r, r + 1,
					 GTK_FILL, GTK_FILL, 0, 0);
			gtk_widget_show(label);
				
			for (i = 0; i <= sizeof(descriptions) / sizeof(struct tagdescr); i++)
			{
				if (i == sizeof(descriptions) / sizeof(struct tagdescr))
					g_warning("xmms_titlestring_descriptions(): Invalid tag: %c", *tags);
				else if (*tags == descriptions[i].tag)
				{
					label = gtk_label_new(gettext(descriptions[i].description));
					gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
					gtk_table_attach(GTK_TABLE(table), label, 2 * c + 1, 2 * c + 2, r, r + 1,
							 GTK_EXPAND | GTK_FILL,  GTK_EXPAND | GTK_FILL, 0 ,0);
					gtk_widget_show(label);
					break;
				}
			}
			tags++;
		}
			
	}

	return table;
}

