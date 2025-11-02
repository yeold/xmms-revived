/*
 *  cdindex.c --- CDindex suport for XMMS.
 *
 *  Copyright (C) 1999 Espen Skoglund <esk@ira.uka.de>
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

#include "config.h"

#ifdef WITH_CDINDEX
#include <parser.h>

#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
# define childs children
# define ATTR_VALUE children
#else
# define ATTR_VALUE val
# ifndef HAVE_XMLDOCGETROOTELEMENT
#  define xmlDocGetRootElement(doc) (doc)->root
# endif
#endif



#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glib.h>

#include "libxmms/util.h"

#include "cdaudio.h"
#include "http.h"
#include "cdinfo.h"
#include "sha.h"
#include "xmms/i18n.h"


/*
 * Function base64_binary (src, srclen, dstlen)
 *
 *    Convert binary content in `src' to base64 and return it.  The
 *    `srcl' is the length of the binary data.  The `dstl' is the
 *    length of the outputted data.  The outputted data should be
 *    subsequently freed using g_free().
 *
 *    The function is almost RFC822 compatible; '-', '.', and '-' are
 *    substituted with '/', '+', and '=' because the resulting string
 *    is used as part of an URL.
 *
 */
static guchar *base64_binary(void *src, gulong srclen, gint *dstlen)
{
	guchar *s, *ret, *d;
	gchar *digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789._";
	gulong i, len;

	len = ((srclen + 2) / 3) * 4;	/* 3 bytes becomes 4 */
	len += 2 * ((len / 60) + 1);	/* Take linebreaks into account */

	d = ret = (guchar *) g_malloc(len + 1);

	if (d == NULL)
		return NULL;

	/*
	 * Process bindata three bytes at a time.
	 */
	for (i = 0, s = src; srclen > 0; s += 3)
	{
		/* Byte 1 is 6msb of source byte 1 */
		*d++ = digits[s[0] >> 2];

		/* Byte 2 is 2lsb of source byte 1 and 4msb of source byte 2 */
		*d++ = digits[((s[0] << 4) + (--srclen ? (s[1] >> 4) : 0)) & 0x3f];

		/* Byte 3 is 4lsb of source byte 2 and 2msb of source byte 3 */
		*d++ = srclen ? 
			digits[((s[1] << 2) + (--srclen ? (s[2]>>6) : 0)) & 0x3f] :
			'-';

		/* Byte 4 is 6lsb of source byte 3 */
		*d++ = srclen ? digits[s[2] & 0x3f] : '-';
		if (srclen)
			srclen--;

		/* Start on new line if 60 characters have been produced */
		if (++i == 15)
		{
			i = 0;
			*d++ = '\015';
			*d++ = '\012';
		}
	}

	/*
	 * Null terminate string
	 */
	*d = '\0';

	*dstlen = len;
	return ret;
}


/*
 * Function cdindex_test_sha ()
 *
 *    Test if the program is able to generete correct SHA-1.  Return 0
 *    upon success, or -1 if an error occured.
 *
 */
static gint cdindex_test_sha(void)
{
	SHA_INFO	sha;
	unsigned char	digest[20], *base64;
	gint		size;

	sha_init(&sha);
	sha_update(&sha, (unsigned char *)"0123456789", 10);
	sha_final(digest, &sha);

	base64 = base64_binary(digest, 20, &size);

	if (strncmp((char*) base64, "h6zsF82dzSCnFsws9nQXtxyKcBY-", size))
	{
		g_free(base64);
		xmms_show_message(
			_("Warning"),
			_("The SHA-1 hash function failed to properly\n"
			  "generate a test key.  As such, Xmms will\n"
			  "not be able to contact a CD Index server.\n"),
			_("Ok"), FALSE, NULL, NULL);
		return -1;
	}
	g_free(base64);

	return 0;
}


/*
 * Function cdindex_get_text (node)
 *
 *    Get text field within node and return it.  Perform a bit of
 *    sanity checking too.  The returned string should be freed later
 *    on using g_free().
 *
 */
static gchar *cdindex_get_text(xmlDoc *doc, xmlNode *node)
{
	gchar *ret;

	ret = xmlNodeListGetString(doc, node, 1);

	if (ret == NULL)
		ret = g_strdup(_("(unknown)"));

	return ret;
}


/*
 * Function cdindex_get_tracks (tinfo)
 *
 *    Read track information and store it.  If this was a single
 *    artist disc, return the name of the artist.  Return NULL
 *    otherwise.
 *
 */
static gchar *cdindex_get_tracks(xmlDoc *doc, xmlNode *tinfo, cdinfo_t *cdinfo)
{
	xmlNode *ch, *ti;
	gchar *cdartist, *tartist, *tname;
	gint prevnum = 0;

	cdartist = NULL;

	ch = tinfo->childs;
	while ( ch != NULL ) {
		if ( !strcasecmp(ch->name, "Artist") ) {
			cdartist = cdindex_get_text(doc, ch->childs);

		} else if ( !strcasecmp(ch->name, "Track") ) {
			xmlAttr *prop;
			gchar *numstr;
			gint num;

			/*
			 * Get track number.
			 */
			prop = ch->properties;
			while ( prop && strcasecmp(prop->name, "Num") )
				prop = prop->next;

			if ( prop == NULL ) {
				/*
				 * Invalid CDinfo response.  Try to be
				 * smart about it.
				 */
				num = prevnum + 1;
			} else {
				numstr = cdindex_get_text(doc, prop->ATTR_VALUE);
				num = atoi(numstr);
			}
			prevnum = num;

			tartist = NULL;
			tname = NULL;

			/*
			 * Get track name.
			 */
			ti = ch->childs;
			while ( ti != NULL ) {
				if ( !strcasecmp(ti->name, "Name") )
					tname = cdindex_get_text(doc, ti->childs);
				if ( !strcasecmp(ti->name, "Artist") )
					tartist = cdindex_get_text(doc, ti->childs);
				ti = ti->next;
			}

			cdda_cdinfo_track_set(cdinfo, num, tartist, tname);
		}

		ch = ch->next;
	}

	/*
	 * Return artist for CD, or NULL if this was a multiartist disc.
	 */
	return cdartist;
}

/*
 * Function cdindex_get_info (doc)
 *
 *    Get information from the specified XML document, and store it.
 *    Return 0 upon success, or -1 if an error occured.
 *
 */
static int cdindex_get_info(xmlDoc *doc, cdinfo_t *cdinfo)
{
	xmlNode *cdinfoX, *node;
	gchar *cdname = NULL, *cdartist = NULL;

	/*
	 * Check for CDinfo.
	 */
	cdinfoX = xmlDocGetRootElement(doc);
	while ( cdinfoX != NULL ) {
		if ( !strcasecmp(cdinfoX->name, "CDinfo") )
			break;
		cdinfoX = cdinfoX->next;
	}

	if ( cdinfoX == NULL )
		/*
		 * No CDinfo found in document.
		 */
		return -1;

	node = cdinfoX->childs;
	while ( node != NULL ) {
		if ( !strcasecmp(node->name, "Title") ) {
			/* Get CD title */
			cdname = cdindex_get_text(doc, node->childs);

		} else if ( !strcasecmp(node->name, "SingleArtistCD") ) {
			/* Get track names */
			cdartist = cdindex_get_tracks(doc, node, cdinfo);

		} else if ( !strcasecmp(node->name, "MultipleArtistCD") ) {
			/* Get track names */
			cdartist = cdindex_get_tracks(doc, node, cdinfo);

		} else if ( !strcasecmp(node->name, "IdInfo") ) {
			; /* Skip */

		} else if ( !strcasecmp(node->name, "NumTracks") ) {
			; /* Skip */
		}

		node = node->next;
	}

	/*
	 * Set name and artist of CD.
	 */
	cdda_cdinfo_cd_set(cdinfo, cdname, cdartist);

	return 0;
}


/*
 * Function cdindex_calc_id (toc)
 *
 *    Calculate SHA for the CD and return it as a base64 string.  The
 *    `toc' should be the result of a cdda_get_toc() invokation.
 *
 */
static gchar *cdindex_calc_id(cdda_disc_toc_t * toc)
{
	static gint	sha_test = 1;
	SHA_INFO	sha;
	guchar		digest[20];
	gchar		tbuf[9];
	gint		i;

	if ( sha_test == 1 )
		sha_test = cdindex_test_sha();

	if ( sha_test == -1 )
		return NULL;

	/*
	 * Sanity check
	 */
	if (toc->last_track == 0)
		return NULL;

	/*
	 * Calculate SHA using:
	 */
	sha_init(&sha);

	/* (1) the first track number, */
	sprintf(tbuf, "%02X", toc->first_track);
	sha_update(&sha, (unsigned char *) tbuf, 2);

	/* (2) the last track number, and */
	sprintf(tbuf, "%02X", toc->last_track);
	sha_update(&sha, (unsigned char *) tbuf, 2);

	/* (3) the offset of each track, with leadout as track 0. */
	sprintf(tbuf, "%08X", LBA(toc->leadout));
	sha_update(&sha, (unsigned char *) tbuf, 8);
	
	for(i = 1; i < 100; i++)
	{
		sprintf(tbuf, "%08X", LBA(toc->track[i]));
		sha_update(&sha, (unsigned char *) tbuf, 8);
	}
	sha_final(digest, &sha);

	return base64_binary(digest, 20, &i);
}


/*
 * Function cdindex_query_server (cdid)
 *
 *    Query cdindex server about the indicated `cdid'.  Return the XML
 *    document returned from the server.
 *
 */
static xmlDoc *cdindex_query_server(gchar * cdid, cdda_disc_toc_t * toc)
{
	xmlDoc *doc;
	gchar *url, *result, *tmp;

	url = g_strdup_printf("http://%s/cgi-bin/cdi/xget.pl?id=%s",
			      cdda_cfg.cdin_server, cdid);

	result = http_get(url);
	g_free(url);

	if ( !result )
		return NULL;

	if ( strncmp(result, "<?xml", 5) ) {
		g_free(result);
		return NULL;
	}

	/*
	 * Somewhere between libxml 1.8.6 and libxml 1.8.9 the size argument
	 * to xmlParseMemory changed from including the trailing '\0' to not
	 * including it.  We add a space at the end to make it work on all
	 * versions.
	 */

	tmp = g_strconcat(result, " ", NULL);
	g_free(result);
	result = tmp;

#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
	xmlKeepBlanksDefault(0);
#endif
	doc = xmlParseMemory(result, strlen(result));
	g_free(result);

	return doc;
}


/*
 * Function cdindex_get_idx ()
 *
 *    Get id from the current CD, contact cdindex server, and parse
 *    the query result.
 *
 */
void cdda_cdindex_get_idx(cdda_disc_toc_t *toc, cdinfo_t *cdinfo)
{
	static gchar *prev_cdid = NULL;
	gchar *cdid;
	xmlDoc *cdinfoX;

	/*
	 * Calculate id from disc.
	 */
	cdid = cdindex_calc_id(toc);
	if ( cdid == NULL ) {
		if ( prev_cdid )
			g_free(prev_cdid);
		/*  cdda_cdinfo_flush(&cdinfo); */
		prev_cdid = NULL;
		return;
	}

	/*
	 * Check if media really has changed (i.e. if we really have
	 * to contact the server).
	 */
	if ( prev_cdid && !strcmp(cdid, prev_cdid) ) {
		g_free(cdid);
		return;
	}
	if ( prev_cdid )
		g_free(prev_cdid);
	prev_cdid = cdid;
	/*  cdda_cdinfo_flush(&cdinfo); */

	/*
	 * Query server about CD.
	 */
	cdinfoX = cdindex_query_server(cdid, toc);
	if ( cdinfoX == NULL )
		return;

	/*
	 * Parse the received XML document.
	 */
	cdindex_get_info(cdinfoX, cdinfo);
	xmlFreeDoc(cdinfoX);
}


#else /* !WITH_CDINDEX */
/*
 * This is the only entry point into this file.  If CD Index
 * support is not compiled in, we just return some more or less sane
 * values.
 */

#include <glib.h>
#include "cdaudio.h"

void cdda_cdindex_get_idx(cdda_disc_toc_t *toc, cdinfo_t *cdinfo)
{
	cdinfo->is_valid = FALSE;
}
#endif
