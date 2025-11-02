/*********************************************************************
 * 
 *    Copyright (C) 1999-2000, 2001,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_frame.c
 * Description:   Code for handling ID3 frames.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Fri Feb  5 23:47:08 1999
 * 
 * $Id: id3_frame.c,v 1.13 2001/12/27 22:37:30 havard Exp $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *                
 ********************************************************************/
#include "config.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "id3.h"
#include "id3_header.h"

static void* id3_frame_get_dataptr(id3_frame_t *frame);
static int id3_frame_get_size(id3_frame_t *frame);
static gboolean id3_frame_is_text(id3_frame_t *frame);


/*
 * Description of all valid ID3v2 frames.
 */
static id3_framedesc_t Framedesc[] = {
    { ID3_AENC, "AENC", "Audio encryption" },
    { ID3_APIC, "APIC", "Attached picture" },

    { ID3_COMM, "COMM", "Comments" },
    { ID3_COMR, "COMR", "Commercial frame" },

    { ID3_ENCR, "ENCR", "Encryption method registration" },
    { ID3_EQUA, "EQUA", "Equalization" },
    { ID3_ETCO, "ETCO", "Event timing codes" },

    { ID3_GEOB, "GEOB", "General encapsulated object" },
    { ID3_GRID, "GRID", "Group identification registration" },

    { ID3_IPLS, "IPLS", "Involved people list" },

    { ID3_LINK, "LINK", "Linked information" },

    { ID3_MCDI, "MCDI", "Music CD identifier" },
    { ID3_MLLT, "MLLT", "MPEG location lookup table" },

    { ID3_OWNE, "OWNE", "Ownership frame" },

    { ID3_PRIV, "PRIV", "Private frame" },
    { ID3_PCNT, "PCNT", "Play counter" },
    { ID3_POPM, "POPM", "Popularimeter" },
    { ID3_POSS, "POSS", "Position synchronisation frame" },

    { ID3_RBUF, "RBUF", "Recommended buffer size" },
    { ID3_RVAD, "RVAD", "Relative volume adjustment" },
    { ID3_RVRB, "RVRB", "Reverb" },

    { ID3_SYLT, "SYLT", "Synchronized lyric/text" },
    { ID3_SYTC, "SYTC", "Synchronized tempo codes" },

    { ID3_TALB, "TALB", "Album/Movie/Show title" },
    { ID3_TBPM, "TBPM", "BPM (beats per minute)" },
    { ID3_TCOM, "TCOM", "Composer" },
    { ID3_TCON, "TCON", "Content type" },
    { ID3_TCOP, "TCOP", "Copyright message" },
    { ID3_TDAT, "TDAT", "Date" },
    { ID3_TDLY, "TDLY", "Playlist delay" },
    { ID3_TENC, "TENC", "Encoded by" },
    { ID3_TEXT, "TEXT", "Lyricist/Text writer" },
    { ID3_TFLT, "TFLT", "File type" },
    { ID3_TIME, "TIME", "Time" },
    { ID3_TIT1, "TIT1", "Content group description" },
    { ID3_TIT2, "TIT2", "Title/songname/content description" },
    { ID3_TIT3, "TIT3", "Subtitle/Description refinement" },
    { ID3_TKEY, "TKEY", "Initial key" },
    { ID3_TLAN, "TLAN", "Language(s)" },
    { ID3_TLEN, "TLEN", "Length" },
    { ID3_TMED, "TMED", "Media type" },
    { ID3_TOAL, "TOAL", "Original album/movie/show title" },
    { ID3_TOFN, "TOFN", "Original filename" },
    { ID3_TOLY, "TOLY", "Original lyricist(s)/text writer(s)" },
    { ID3_TOPE, "TOPE", "Original artist(s)/performer(s)" },
    { ID3_TORY, "TORY", "Original release year" },
    { ID3_TOWN, "TOWN", "File owner/licensee" },
    { ID3_TPE1, "TPE1", "Lead performer(s)/Soloist(s)" },
    { ID3_TPE2, "TPE2", "Band/orchestra/accompaniment" },
    { ID3_TPE3, "TPE3", "Conductor/performer refinement" },
    { ID3_TPE4, "TPE4", "Interpreted, remixed, or otherwise modified by" },
    { ID3_TPOS, "TPOS", "Part of a set" },
    { ID3_TPUB, "TPUB", "Publisher" },
    { ID3_TRCK, "TRCK", "Track number/Position in set" },
    { ID3_TRDA, "TRDA", "Recording dates" },
    { ID3_TRSN, "TRSN", "Internet radio station name" },
    { ID3_TRSO, "TRSO", "Internet radio station owner" },
    { ID3_TSIZ, "TSIZ", "Size" },
    { ID3_TSRC, "TSRC", "ISRC (international standard recording code)" },
    { ID3_TSSE, "TSSE", "Software/Hardware and settings used for encoding" },
    { ID3_TYER, "TYER", "Year" },
    { ID3_TXXX, "TXXX", "User defined text information frame" },

    { ID3_UFID, "UFID", "Unique file identifier" },
    { ID3_USER, "USER", "Terms of use" },
    { ID3_USLT, "USLT", "Unsychronized lyric/text transcription" },

    { ID3_WCOM, "WCOM", "Commercial information" },
    { ID3_WCOP, "WCOP", "Copyright/Legal information" },
    { ID3_WOAF, "WOAF", "Official audio file webpage" },
    { ID3_WOAR, "WOAR", "Official artist/performer webpage" },
    { ID3_WOAS, "WOAS", "Official audio source webpage" },
    { ID3_WORS, "WORS", "Official internet radio station homepage" },
    { ID3_WPAY, "WPAY", "Payment" },
    { ID3_WPUB, "WPUB", "Publishers official webpage" },
    { ID3_WXXX, "WXXX", "User defined URL link frame" },
};


/*
 * Function id3_read_frame (id3)
 *
 *    Read next frame from the indicated ID3 tag.  Return 0 upon
 *    success, or -1 if an error occured.
 *
 */
int id3_read_frame(id3_t *id3)
{
    id3_framehdr_t *framehdr;
    id3_frame_t *frame;
    guint32 id;
    void *p;
    int i;

    /*
     * Read frame header.
     */
    framehdr = id3->id3_read( id3, NULL, sizeof(*framehdr) );
    if ( framehdr == NULL )
	return -1;

    /*
     * If we encounter an invalid frame id, we assume that there is
     * some padding in the header.  We just skip the rest of the ID3
     * tag.
     */
    i = *((guint8 *) &framehdr->fh_id);
    if ( !((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z')) ) {
	id3->id3_seek( id3, id3->id3_tagsize - id3->id3_pos );
	return 0;
    }
    id = g_ntohl(framehdr->fh_id);

    /*
     * Allocate frame.
     */
    frame = g_malloc0(sizeof(*frame));

    frame->fr_owner = id3;
    frame->fr_raw_size = g_ntohl(framehdr->fh_size);
    frame->fr_flags = g_ntohs(framehdr->fh_flags);

    /*
     * Determine the type of the frame.
     */
    for (i = 0; i < sizeof(Framedesc) / sizeof(id3_framedesc_t); i++)
    {
	if ( Framedesc[i].fd_id == id ) {
	    /*
	     * Initialize frame.
	     */
	    frame->fr_desc = Framedesc + i;

	    /*
	     * When allocating memory to hold a text frame, we
	     * allocate 2 extra byte.  This simplifies retrieval of
	     * text strings.
	     */
	    frame->fr_raw_data = g_malloc(frame->fr_raw_size +
					  (id3_frame_is_text(frame) ?
					   2 : 0));
	    p = id3->id3_read(id3, frame->fr_raw_data, frame->fr_raw_size);
	    if ( p == NULL ) {
		g_free(frame->fr_raw_data);
		g_free(frame);
		return -1;
	    }

	    /*
	     * Null-terminate text frames.
	     */
	    if (id3_frame_is_text(frame))
	    {
		((char *) frame->fr_raw_data)[frame->fr_raw_size] = 0;
		((char *) frame->fr_raw_data)[frame->fr_raw_size + 1] = 0;
	    }

	    /*
	     * Insert frame into linked list.
	     */
	    id3->id3_frame = g_list_append(id3->id3_frame, frame);

	    break;
	}
    }

    /*
     * Check if frame had a valid id.
     */
    if ( frame->fr_desc == NULL ) {
	/*
	 * No. Ignore the frame.
	 */
	if (id3->id3_seek(id3, frame->fr_raw_size) < 0)
	{
	    g_free(frame);
	    return -1;
	}
	return 0;
    }

    /*
     * Check if frame is compressed using zlib.
     */
    if (frame->fr_flags & ID3_FHFLAG_COMPRESS)
	    return 0;
    
    frame->fr_data = id3_frame_get_dataptr(frame);
    frame->fr_size = id3_frame_get_size(frame);

    return 0;
}


/*
 * Function id3_get_frame (id3, type, num)
 *
 *    Search in the list of frames for the ID3-tag, and return a frame
 *    of the indicated type.  If tag contains several frames of the
 *    indicated type, the third argument tells which of the frames to
 *    return.
 *
 */
id3_frame_t *id3_get_frame(id3_t *id3, guint32 type, int num)
{
	GList *node;

	for (node = id3->id3_frame; node != NULL; node = node->next)
	{
		id3_frame_t *fr = node->data;
		if (fr->fr_desc && fr->fr_desc->fd_id == type)
		{
			if (--num <= 0)
				return fr;
		}
	}
	return NULL;
}

/*
 * Function decompress_frame(frame)
 *
 *    Uncompress the indicated frame.  Return 0 upon success, or -1 if
 *    an error occured.
 *
 */
static int decompress_frame(id3_frame_t *frame)
{
#ifdef HAVE_LIBZ
    z_stream z;
    int r;

    /*
     * Fetch the size of the decompressed data.
     */
    frame->fr_size_z = g_ntohl( *((guint32 *) frame->fr_raw_data) );

    /*
     * Allocate memory to hold uncompressed frame.
     */
    frame->fr_data_z = g_malloc(frame->fr_size_z + 
				id3_frame_is_text(frame) ? 2 : 0);

    /*
     * Initialize zlib.
     */
    z.next_in   = id3_frame_get_dataptr(frame);
    z.avail_in  = id3_frame_get_size(frame);
    z.zalloc    = NULL;
    z.zfree     = NULL;
    z.opaque    = NULL;

    r = inflateInit( &z );
    switch ( r ) {
    case Z_OK:
	break;
    case Z_MEM_ERROR:
	id3_error( frame->fr_owner, "zlib - no memory" );
	goto Error_init;
    case Z_VERSION_ERROR:
	id3_error( frame->fr_owner, "zlib - invalid version" );
	goto Error_init;
    default:
	id3_error( frame->fr_owner, "zlib - unknown error" );
	goto Error_init;
    }

    /*
     * Decompress frame.
     */
    z.next_out = frame->fr_data_z;
    z.avail_out = frame->fr_size_z;
    r = inflate( &z, Z_SYNC_FLUSH );
    switch ( r ) {
    case Z_STREAM_END:
	break;
    case Z_OK:
	if ( z.avail_in == 0 )
	    /*
	     * This should not be possible with a correct stream.
	     * We will be nice however, and try to go on.
	     */
	    break;
	id3_error( frame->fr_owner, "zlib - buffer exhausted" );
	goto Error_inflate;
    default:
	id3_error( frame->fr_owner, "zlib - unknown error" );
	goto Error_inflate;
    }

    r = inflateEnd( &z );
    if ( r != Z_OK )
	id3_error( frame->fr_owner, "zlib - inflateEnd error" );

    /*
     * Null-terminate text frames.
     */
    if (id3_frame_is_text(frame))
    {
	((char *) frame->fr_data_z)[frame->fr_size_z] = 0;
	((char *) frame->fr_data_z)[frame->fr_size_z + 1] = 0;
    }
    frame->fr_data = frame->fr_data_z;
    frame->fr_size = frame->fr_size_z;

    return 0;

    /*
     * Cleanup code.
     */
 Error_inflate:
    r = inflateEnd( &z );
 Error_init:
    g_free(frame->fr_data_z);
    frame->fr_data_z = NULL;
#endif
    return -1;
}

/*
 * Function id3_decompress_frame(frame)
 *
 *    Check if frame is compressed, and uncompress if necessary.
 *    Return 0 upon success, or -1 if an error occured.
 *
 */
int id3_decompress_frame(id3_frame_t *frame)
{
	if (!(frame->fr_flags & ID3_FHFLAG_COMPRESS))
		/* Frame not compressed */
		return 0;
	if (frame->fr_data_z)
		/* Frame already decompressed */
		return 0;
	/* Do decompression */
	return decompress_frame(frame);
}


/*
 * Function id3_delete_frame (frame)
 *
 *    Remove frame from ID3 tag and release memory ocupied by it.
 *
 */
int id3_delete_frame(id3_frame_t *frame)
{
    GList *list = frame->fr_owner->id3_frame;
    int ret;

    /*
     * Search for frame in list.
     */

    if (g_list_find(list, frame) != NULL) {
	/*
	 * Frame does not exist in frame list.
	 */
	ret = -1;

    } else {
	/*
	 * Remove frame from frame list.
	 */
	list = g_list_remove(list, frame);
	frame->fr_owner->id3_altered = 1;
	ret = 0;
    }

    /*
     * Release memory occupied by frame.
     */
    if (frame->fr_raw_data)
	 g_free(frame->fr_raw_data);
    if ( frame->fr_data_z )
	 g_free( frame->fr_data_z );
    g_free( frame );

    return ret;
}


/*
 * Function id3_add_frame (id3, type)
 *
 *    Add a new frame to the ID3 tag.  Return a pointer to the new
 *    frame, or NULL if an error occured.
 *
 */
id3_frame_t *id3_add_frame(id3_t *id3, guint32 type)
{
    id3_frame_t *frame;
    int i;

    /*
     * Allocate frame.
     */
    frame = g_malloc0( sizeof(*frame) );

    /*
     * Initialize frame
     */
    frame->fr_owner = id3;

    /*
     * Try finding the correct frame descriptor.
     */
    for ( i = 0; i < sizeof(Framedesc) / sizeof(id3_framedesc_t); i++ ) {
	if ( Framedesc[i].fd_id == type ) {
	    frame->fr_desc = &Framedesc[i];
	    break;
	}
    }

    /*
     * Insert frame into linked list.
     */
    id3->id3_frame = g_list_append(id3->id3_frame, frame);
    id3->id3_altered = 1;

    return frame;
}


/*
 * Destroy all frames  in an id3 tag, and free all data
 */
void id3_destroy_frames(id3_t *id)
{
	GList *node;

	for (node = id->id3_frame; node != NULL; node = node->next)
	{
		id3_frame_t *frame = node->data;
		/*
		 * Release memory occupied by frame.
		 */
		if (frame->fr_raw_data)
			g_free(frame->fr_raw_data);
		if (frame->fr_data_z)
			g_free(frame->fr_data_z);
		g_free(frame);
	}
	g_list_free(id->id3_frame);
	id->id3_frame = NULL;
}

static gboolean id3_frame_is_text(id3_frame_t *frame)
{
	if (frame && frame->fr_desc &&
	    (frame->fr_desc->fd_idstr[0] == 'T' ||
	     frame->fr_desc->fd_idstr[0] == 'W' ))
		return TRUE;
	return FALSE;
}

static int id3_frame_extra_headers(id3_frame_t *frame)
{
	int retv = 0;
	/*
	 * If frame is encrypted, we have four extra bytes in the
	 * header.
	 */
	if (frame->fr_flags & ID3_FHFLAG_COMPRESS)
		retv += 4;
	/*
	 * If frame is encrypted, we have one extra byte in the
	 * header.
	 */
	if (frame->fr_flags & ID3_FHFLAG_ENCRYPT)
		retv += 1;
	
	/*
	 * If frame has grouping identity, we have one extra byte in
	 * the header.
	 */
	if (frame->fr_flags & ID3_FHFLAG_GROUP)
		retv += 1;

	return retv;
}

static void* id3_frame_get_dataptr(id3_frame_t *frame)
{
	char *ptr = frame->fr_raw_data;

	ptr += id3_frame_extra_headers(frame);
	
	return ptr;
}

static int id3_frame_get_size(id3_frame_t *frame)
{
	return frame->fr_raw_size - id3_frame_extra_headers(frame);
}

void id3_frame_clear_data(id3_frame_t *frame)
{
	if (frame->fr_raw_data)
		g_free(frame->fr_raw_data);
	if (frame->fr_data_z)
		g_free(frame->fr_data_z);
	frame->fr_raw_data = NULL;
	frame->fr_raw_size = 0;
	frame->fr_data = NULL;
	frame->fr_size = 0;
	frame->fr_data_z = NULL;
	frame->fr_size_z = 0;
}
