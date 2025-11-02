/*
 * Copyright (C) 1999-2001  Håvard Kvålen <havardk@xmms.org>
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

#include <config.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#ifdef HAVE_ICONV_OPEN
#include <iconv.h>
#endif

#include <errno.h>

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif



static char* get_current_charset(void)
{
	char *charset = getenv("CHARSET");

#ifdef HAVE_CODESET
	if (!charset)
		charset = nl_langinfo(CODESET);
#endif
	/* Maybe we should default to ISO-8859-1 instead? */
	if (!charset)
		charset = "US-ASCII";

	return charset;
}


#ifdef HAVE_ICONV_OPEN
static char* convert_string(const char *string, char *from, char *to)
{
	size_t outleft, outsize, length;
	iconv_t cd;
	char *out, *outptr;
	const char *input = string;

	if (!string)
		return NULL;

	length = strlen(string);
	
	/*  g_message("converting %s from %s to %s", string, from, to); */
	if ((cd = iconv_open(to, from)) == (iconv_t)-1)
	{
		g_warning("convert_string(): Conversion not supported. Charsets: %s -> %s", from, to);
		return g_strdup(string);
	}

	/* Due to a GLIBC bug, round outbuf_size up to a multiple of 4 */
	/* + 1 for nul in case len == 1 */
	outsize = ((length + 3) & ~3) + 1;
	out = g_malloc(outsize);
	outleft = outsize - 1;
	outptr = out;

 retry:
	if (iconv(cd, &input, &length, &outptr, &outleft) == -1)
	{
		int used;
		switch (errno)
		{
			case E2BIG:
				used = outptr - out;
				outsize = (outsize - 1) * 2 + 1;
				out = g_realloc(out, outsize);
				outptr = out + used;
				outleft = outsize - 1 - used;
				goto retry;
			case EINVAL:
				break;
			case EILSEQ:
				/* Invalid sequence, try to get the
                                   rest of the string */
				input++;
				length = strlen(input);
				goto retry;
			default:
				g_warning("convert_string(): Conversion failed. Inputstring: %s; Error: %s", string, strerror(errno));
				break;
		}
	}
	*outptr = '\0';

	iconv_close(cd);
	return out;
}

#else

static char* convert_string(const char *string, char *from, char *to)
{
	return g_strdup(string);
}

#endif

char* convert_to_utf8(const char *string)
{
	char *charset = get_current_charset();

	return convert_string(string, charset, "UTF-8");
}


char* convert_from_utf8(const char *string)
{
	char *charset = get_current_charset();

	return convert_string(string, "UTF-8", charset);
}


