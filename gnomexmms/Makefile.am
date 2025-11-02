if HAVE_GNOME

bin_PROGRAMS = gnomexmms

gnomexmms_LDADD = @GTK_LIBS@ @GNOME_LIBDIR@ @GNOMEGNORBA_LIBS@ \
-lpanel_applet @ORBIT_LIBS@ $(top_builddir)/libxmms/libxmms.la @INTLLIBS@
INCLUDES = @GTK_CFLAGS@ @ORBIT_CFLAGS@ @XMMS_DEFINES@ $(GNOME_INCLUDEDIR) -I$(top_builddir)/intl

gnomexmms_SOURCES = gnomexmms.c

sysdir = $(GNOME_DATADIR)/applets/Multimedia
sys_DATA = gnomexmms.desktop

gnorbadir = $(GNOME_SYSCONFDIR)/CORBA/servers
gnorba_DATA = gnomexmms.gnorba

else

gnomexmms_SOURCES = 

endif

EXTRA_DIST = gnomexmms.c gnomexmms.desktop gnomexmms.gnorba gnomexmms.xpm xmms-dock-master.xpm

