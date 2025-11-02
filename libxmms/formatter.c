#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include "formatter.h"

Formatter *xmms_formatter_new(void)
{
	Formatter *formatter = g_new0(Formatter, 1);

	xmms_formatter_associate(formatter, '%', "%");
	return formatter;
}

void xmms_formatter_destroy(Formatter *formatter)
{
	gint i;

	for(i = 0; i < 256; i++)
		if(formatter->values[i])
			g_free(formatter->values[i]);
	g_free(formatter);
}

void xmms_formatter_associate(Formatter *formatter, guchar id, gchar *value)
{
	xmms_formatter_dissociate(formatter, id);
	formatter->values[id] = g_strdup(value);
}

void xmms_formatter_dissociate(Formatter *formatter, guchar id)
{
	if(formatter->values[id])
		g_free(formatter->values[id]);
	formatter->values[id] = 0;
}

gchar *xmms_formatter_format(Formatter *formatter, gchar *format)
{
	gchar *p, *q, *buffer;
	gint len;

	for(p = format, len = 0; *p; p++)
		if(*p == '%') {
			if(formatter->values[(gint)*++p])
				len += strlen(formatter->values[(gint)*p]);
			else
				len += 2;
		} else
			len++;
	buffer = g_malloc(len + 1);
	for(p = format, q = buffer; *p; p++)
		if(*p == '%') {
			if(formatter->values[(gint)*++p]) {
				strcpy(q, formatter->values[(gint)*p]);
				q += strlen(q);
			} else {
				*q++ = '%';
				*q++ = *p;
			}
		} else
			*q++ = *p;
	*q = 0;
	return buffer;
}

