
#if defined(USE_DMALLOC)
# include <glib.h>

/* 
 * G_malloc() and friends are redefined by glib.h when using DMALLOC.
 */
# define ALLOC(type, count)	    (type *) malloc (sizeof (type) * (count))
# define CALLOC(type, count)	    (type *) calloc ((count), sizeof (type))
# define REALLOC(mem, type, count)  (type *) realloc ((mem), sizeof (type) * (count))
# define MALLOC(size)		    (gpointer) malloc (size)
# define FREE(mem)		    (void) free (mem)


/*
 * Wrapper macro which invokes the given function and allocates the
 * returned string through the dmalloc library.
 */
# define g_wrap_it(function, args...) 					\
({									\
	gchar *__s1 = function (args);					\
	gchar *__s2 = strcpy (malloc (strlen (__s1) + 1), __s1);	\
	g_free_orig (__s1);						\
	__s2;								\
})


/* 
 * The follwing functions return strings which must be freed with
 * g_free().  We must therefore wrap them so that the strings are
 * allocated via the dmalloc library.
 */
# define g_strdup(str)                 g_wrap_it (g_strdup, str)
# define g_strdup_printf(fmt, args...) g_wrap_it (g_strdup_printf, fmt ,##args)
# define g_strdup_vprintf(fmt, argv)   g_wrap_it (g_strdup_vprintf, fmt, argv)
# define g_strndup(str, n)             g_wrap_it (g_strndup, str, n)
# define g_strnfill(len, fill_char)    g_wrap_it (g_strnfill, len, fill_char)
# define g_strconcat(str1, args...)    g_wrap_it (g_strconcat, str1 ,##args)
# define g_strjoin(sep, args...)       g_wrap_it (g_strjoin, sep ,##args)
# define g_strescape(str)              g_wrap_it (g_strescape, str)
# define g_strjoinv(sep, str_array)    g_wrap_it (g_strjoinv, sep, str_array)


void g_free_func (gpointer mem);
void g_free_orig (gpointer mem);

#else
# define g_free_func g_free
#endif

#undef HAVE_SCHED_SETSCHEDULER
#undef HAVE_NANOSLEEP

@TOP@

#undef PACKAGE
#undef VERSION
