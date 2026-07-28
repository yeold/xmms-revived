#ifndef SPOTIFYD_MPRIS_H
#define SPOTIFYD_MPRIS_H

#include <glib.h>

gboolean mpris_init(void); /* main thread only */
void mpris_cleanup(void);

gboolean mpris_open_uri(const char *xmms_url); /* spotify://track/ID */
void mpris_play(void);
void mpris_pause(void);
void mpris_stop(void);
void mpris_seek_abs(gint64 usec);

gint64 mpris_position(void); /* microseconds, -1 on failure */

/* filled by PropertiesChanged; guarded by an internal mutex */
char *mpris_get_title(void); /* g_free() the result */
gint mpris_get_length(void); /* milliseconds, -1 unknown */

#endif