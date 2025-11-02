/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen <havardk@sol.no>
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

#include "cdaudio.h"
#include <pthread.h>
#include "xmms/i18n.h"
#include "libxmms/util.h"
#include "libxmms/titlestring.h"

#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#elif defined HAVE_SYS_CDIO_H
#include <sys/cdio.h>
#endif

#ifdef CDROMSTOP
# define XMMS_STOP CDROMSTOP
#elif defined CDIOCSTOP
# define XMMS_STOP CDIOCSTOP
#else
# error "No stop ioctl"
#endif

#ifdef CDIOCPAUSE
# define XMMS_PAUSE CDIOCPAUSE
#elif defined CDROMPAUSE
# define XMMS_PAUSE CDROMPAUSE
#else
# error "No pause ioctl"
#endif

#ifdef CDIOCRESUME
# define XMMS_RESUME CDIOCRESUME
#elif defined CDROMRESUME
# define XMMS_RESUME CDROMRESUME
#else
# error "No resume ioctl"
#endif

#ifndef CDDA_DEVICE
# ifdef HAVE_SYS_CDIO_H
#  ifdef __FreeBSD__
#   define CDDA_DEVICE "/dev/acd0c"
#  elif defined __OpenBSD__
#   define CDDA_DEVICE "/dev/cd0c"
#  else
#   define CDDA_DEVICE "/vol/dev/aliases/cdrom0"
#  endif
# else
#   define CDDA_DEVICE "/dev/cdrom"
# endif
#endif

#ifndef CDDA_DIRECTORY
# ifdef HAVE_SYS_CDIO_H
#  ifdef __FreeBSD__
#   define CDDA_DIRECTORY "/cdrom"
#  elif defined __OpenBSD__
#   define CDDA_DIRECTORY "/cdrom"
#  else
#   define CDDA_DIRECTORY "/cdrom/cdrom0"
#  endif
# else
#   define CDDA_DIRECTORY "/mnt/cdrom"
# endif
#endif

static char * cdda_get_title(cdda_disc_toc_t *toc, gint track);
static gboolean stop_timeout(gpointer data);

static void cdda_init(void);
static int is_our_file(char *filename);
static GList *scan_dir(char *dir);
static void play_file(char *filename);
static void stop(void);
static void cdda_pause(short p);
static void seek(int time);
static int get_time(void);
static void get_song_info(char *filename, char **title, int *length);
static void get_volume(int *l, int *r);
static void set_volume(int l, int r);
static void cleanup(void);
void cdda_fileinfo(gchar *filename);

InputPlugin cdda_ip =
{
	NULL,
	NULL,
	NULL, /* Description */
	cdda_init,
	NULL,				/* about */
	cdda_configure,
	is_our_file,
	scan_dir,
	play_file,
	stop,
	cdda_pause,
	seek,
	NULL,				/* set_eq */
	get_time,
	get_volume,
	set_volume,
	cleanup,
	NULL,				/* obsolete */
	NULL,				/* add_vis_pcm */
	NULL,				/* set_info, filled in by xmms */
	NULL,				/* set_info_text, filled in by xmms */
	get_song_info,
	NULL, /*  cdda_fileinfo, */	/* file_info_box */
	NULL				/* output plugin handle */
};

CDDAConfig cdda_cfg;

static gint cdda_fd = -1;
static gint track;
static gboolean is_paused;
static int pause_time;
static cdda_disc_toc_t cd_toc;
static int stop_timeout_id;

/* Time to delay stop command in 1/10 second */
#define STOP_DELAY 20

#if !defined(CDROMVOLREAD) && !(defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__)))
static gint volume_left = 100, volume_right = 100;
#endif

InputPlugin *get_iplugin_info(void)
{
	cdda_ip.description = g_strdup_printf(_("CD Audio Player %s"), VERSION);
	return &cdda_ip;
}

static void cdda_init(void)
{
	ConfigFile *cfgfile;

	memset(&cdda_cfg, 0, sizeof(CDDAConfig));
	       
#if defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H)
	cdda_cfg.use_oss_mixer = TRUE;
#endif

	cfgfile = xmms_cfg_open_default_file();

	xmms_cfg_read_string(cfgfile, "CDDA", "device", &cdda_cfg.device);
	xmms_cfg_read_string(cfgfile, "CDDA", "directory", &cdda_cfg.directory);
	xmms_cfg_read_boolean(cfgfile, "CDDA", "use_oss_mixer", &cdda_cfg.use_oss_mixer);
	xmms_cfg_read_boolean(cfgfile, "CDDA", "title_override", &cdda_cfg.title_override);
	xmms_cfg_read_string(cfgfile, "CDDA", "name_format", &cdda_cfg.name_format);
	xmms_cfg_read_boolean(cfgfile, "CDDA", "use_cddb", &cdda_cfg.use_cddb);
	xmms_cfg_read_string(cfgfile, "CDDA", "cddb_server", &cdda_cfg.cddb_server);
#ifdef WITH_CDINDEX
	xmms_cfg_read_boolean(cfgfile, "CDDA", "use_cdin", &cdda_cfg.use_cdin);
#else
	cdda_cfg.use_cdin = FALSE;
#endif
	xmms_cfg_read_string(cfgfile, "CDDA", "cdin_server", &cdda_cfg.cdin_server);
	xmms_cfg_free(cfgfile);

	if (!cdda_cfg.device)
		cdda_cfg.device = g_strdup(CDDA_DEVICE);
	if (!cdda_cfg.directory)
		cdda_cfg.directory = g_strdup(CDDA_DIRECTORY);
	if (!cdda_cfg.cdin_server)
		cdda_cfg.cdin_server = g_strdup("www.cdindex.org");
	if (!cdda_cfg.cddb_server)
		cdda_cfg.cddb_server = g_strdup(CDDB_DEFAULT_SERVER);
	if (!cdda_cfg.name_format)
		cdda_cfg.name_format = g_strdup("%p - %t");
}

static void cleanup(void)
{
	if (stop_timeout_id)
	{
		gtk_timeout_remove(stop_timeout_id);
		stop_timeout(NULL);
	}
	cddb_quit();
}

static int is_our_file(char *filename)
{
	gchar *ext;

	ext = strrchr(filename, '.');
	if (ext && !strcasecmp(ext, ".cda"))
		return TRUE;

	return FALSE;
}

static gboolean is_mounted(gchar * device_name)
{
#if defined(HAVE_MNTENT_H) || defined(HAVE_GETMNTINFO)
  	char devname[256];
  	struct stat st;
#if defined(HAVE_MNTENT_H)
 	FILE *mounts;
 	struct mntent *mnt;
#elif defined(HAVE_GETMNTINFO)
 	struct statfs *fsp;
 	int entries;
#endif

	if (lstat(device_name, &st) < 0)
		return -1;

	if (S_ISLNK(st.st_mode))
		readlink(device_name, devname, 256);
	else
		strncpy(devname, device_name, 256);

#if defined(HAVE_MNTENT_H)		
	if ((mounts = setmntent(MOUNTED, "r")) == NULL)
		return TRUE;

	while ((mnt = getmntent(mounts)) != NULL)
	{
		if (strcmp(mnt->mnt_fsname, devname) == 0)
		{
			endmntent(mounts);
			return TRUE;
		}
	}
	endmntent(mounts);
#elif defined(HAVE_GETMNTINFO)
 	entries = getmntinfo(&fsp, MNT_NOWAIT);
 	if (entries < 0)
 		return NULL;
	
	while (entries-- > 0)
	{
		if (!strcmp(fsp->f_mntfromname, devname))
			return TRUE;
		fsp++;
	}
#endif                            
#endif
	return FALSE;
}

static GList *scan_dir(char *dir)
{
	GList *list = NULL;
	gint i;
	cdda_disc_toc_t toc;

	if (strncmp(cdda_cfg.directory, dir, strlen(cdda_cfg.directory)))
		return NULL;

	if (!cdda_get_toc(&toc))
		return NULL;

	for (i = toc.last_track; i >= toc.first_track; i--)
		if (!toc.track[i].flags.data_track)
			list = g_list_prepend(list, g_strdup_printf("Track %02d.cda", i));

	return list;
}

guint cdda_calculate_track_length(cdda_disc_toc_t *toc, gint track)
{
	if (track == toc->last_track)
		return (LBA(toc->leadout) - LBA(toc->track[track]));
	else
		return (LBA(toc->track[track + 1]) - LBA(toc->track[track]));
}

static void play_file(char *filename)
{
	gchar *tmp;

	if (is_mounted(cdda_cfg.device))
		return;

	tmp = strrchr(filename, '/');
	if (tmp)
		tmp++;
	else
		tmp = filename;

	if (!sscanf(tmp, "Track %d.cda", &track))
		return;

	if ((cdda_fd = open(cdda_cfg.device, CDOPENFLAGS)) == -1)
		return;

	if (!cdda_get_toc(&cd_toc) || cd_toc.track[track].flags.data_track ||
	    track < cd_toc.first_track || track > cd_toc.last_track)
	{
		close(cdda_fd);
		cdda_fd = -1;
		return;
	}
	cdda_ip.set_info(cdda_get_title(&cd_toc, track),
			 (cdda_calculate_track_length(&cd_toc, track) * 1000) / 75,
			 44100 * 2 * 2 * 8, 44100, 2);

	is_paused = FALSE;
	if (stop_timeout_id)
	{
		gtk_timeout_remove(stop_timeout_id);
		stop_timeout_id = 0;
	}
	seek(0);
}

static char * cdda_get_title(cdda_disc_toc_t *toc, gint track)
{
	static guint32 cached_id;
	static cdinfo_t cdinfo;
	static pthread_mutex_t title_mutex = PTHREAD_MUTEX_INITIALIZER;
	TitleInput *input;
	guint32 disc_id;
	gchar *title;

	disc_id = cdda_cddb_compute_discid(toc);
	
	/*
	 * We want to avoid looking up a album from two threads simultaneously.
	 * This can happen since we are called both from the main-thread and
	 * from the playlist-thread.
	 */
	
	pthread_mutex_lock(&title_mutex);
	if (!(disc_id == cached_id && cdinfo.is_valid))
	{
		/*
		 * We try to look up the disc again if the info is not
		 * valid.  The user might have configured a new server
		 * in the meantime.
		 */
		cdda_cdinfo_flush(&cdinfo);
		cached_id = disc_id;

		if (!cdda_cdinfo_read_file(disc_id, &cdinfo))
		{
			if (cdda_cfg.use_cdin)
				cdda_cdindex_get_idx(toc, &cdinfo);
			if (cdda_cfg.use_cddb && !cdinfo.is_valid)
				cdda_cddb_get_info(toc, &cdinfo);
			if (cdinfo.is_valid)
				cdda_cdinfo_write_file(disc_id, &cdinfo);
		}
	}
	pthread_mutex_unlock(&title_mutex);
	
	XMMS_NEW_TITLEINPUT(input);
	cdda_cdinfo_get(&cdinfo, track, &input->performer, &input->album_name,
			&input->track_name);
	input->track_number = track;
	input->file_name = input->file_path = 
	    g_strdup_printf(_("CD Audio Track %02u"), track);
	input->file_ext = "cda";
	title =  xmms_get_titlestring(cdda_cfg.title_override ?
				      cdda_cfg.name_format :
				      xmms_get_gentitle_format(), input);
	g_free(input->file_name);
	g_free(input);

	if (!title)
		title = g_strdup_printf(_("CD Audio Track %02u"), track);
	return title;
}

static gboolean stop_timeout(gpointer data)
{
	int fd;

	fd = open(cdda_cfg.device, CDOPENFLAGS);
	if (fd != -1)
	{
		ioctl(fd, XMMS_STOP, 0);
		close(fd);
	}
	stop_timeout_id = 0;
	return FALSE;
}

static void stop(void)
{
	if (cdda_fd < 0)
		return;

	ioctl(cdda_fd, XMMS_PAUSE);
	close(cdda_fd);
	cdda_fd = -1;
	stop_timeout_id = gtk_timeout_add(STOP_DELAY * 100, stop_timeout, NULL);
}

static void cdda_pause(short p)
{
	if (p)
	{
		pause_time = get_time();
		ioctl(cdda_fd, XMMS_PAUSE);
	}
	else
	{
		ioctl(cdda_fd, XMMS_RESUME);
		pause_time = -1;
	}
	is_paused = p;
}

static void seek(int time)
{
	struct cdda_msf *end, start;

#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
	struct ioc_play_msf msf;
#else
	struct cdrom_msf msf;
#endif

	start.minute = (cd_toc.track[track].minute * 60 +
			cd_toc.track[track].second + time) / 60;
	start.second = (cd_toc.track[track].second + time) % 60;
	start.frame = cd_toc.track[track].frame;
	if (track == cd_toc.last_track)
		end = &cd_toc.leadout;
	else
		end = &cd_toc.track[track + 1];

#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
	msf.start_m = start.minute;
	msf.start_s = start.second;
	msf.start_f = start.frame;
	msf.end_m = end->minute;
	msf.end_s = end->second;
	msf.end_f = end->frame;
	ioctl(cdda_fd, CDIOCPLAYMSF, &msf);
#else
	msf.cdmsf_min0 = start.minute;
	msf.cdmsf_sec0 = start.second;
	msf.cdmsf_frame0 = start.frame;
	msf.cdmsf_min1 = end->minute;
	msf.cdmsf_sec1 = end->second;
	msf.cdmsf_frame1 = end->frame;
	ioctl(cdda_fd, CDROMPLAYMSF, &msf);
#endif
	if (is_paused)
	{
		cdda_pause(TRUE);
		pause_time = time * 1000;
	}
}

static int get_current_frame(void)
{
#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
	struct ioc_read_subchannel subchnl;
	struct cd_sub_channel_info subinfo;
	subchnl.address_format = CD_MSF_FORMAT;
	subchnl.data_format = CD_CURRENT_POSITION;
	subchnl.data_len = sizeof(subinfo);
	subchnl.data = &subinfo;
	ioctl(cdda_fd, CDIOCREADSUBCHANNEL, &subchnl);

	return(LBA(subchnl.data->what.position.absaddr.msf));
#else
	struct cdrom_subchnl subchnl;

	subchnl.cdsc_format = CDROM_MSF;
	ioctl(cdda_fd, CDROMSUBCHNL, &subchnl);

	switch (subchnl.cdsc_audiostatus)
	{
		case CDROM_AUDIO_COMPLETED:
		case CDROM_AUDIO_ERROR:
			return -1;
	}

	return(LBA(subchnl.cdsc_absaddr.msf));
#endif
}


static int get_time(void)
{
	int frame, start_frame, length;

	if (cdda_fd == -1)
		return -1;

	if (is_paused && pause_time != -1)
		return pause_time;

	frame = get_current_frame();

	if (frame == -1)
		return -1;
	
	start_frame = LBA(cd_toc.track[track]);
	length = cdda_calculate_track_length(&cd_toc, track);

	if (frame - start_frame >= length - 20)	/* 20 seems to work better */
		return -1;

	return ((frame - start_frame) * 1000) / 75;
}

static void get_song_info(char *filename, char **title, int *len)
{
	cdda_disc_toc_t toc;
	gint t;
	gchar *tmp;

	*title = NULL;
	*len = -1;

	tmp = strrchr(filename, '/');
	if (tmp)
		tmp++;
	else
		tmp = filename;
	
	if (!sscanf(tmp, "Track %d.cda", &t))
		return;
	if (!cdda_get_toc(&toc))
		return;
	if (t < toc.first_track || t > toc.last_track || toc.track[t].flags.data_track)
		return;
	
	*len = (cdda_calculate_track_length(&toc, t) * 1000) / 75;
	*title = cdda_get_title(&toc, t);
}

static void get_volume(int *l, int *r)
{
#if defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H)
	int fd, devs, cmd, v;

	if (cdda_cfg.use_oss_mixer)
	{
		fd = open(DEV_MIXER, O_RDONLY);
		if (fd != -1)
		{
			ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
			if (devs & SOUND_MASK_CD)
				cmd = SOUND_MIXER_READ_CD;
			else if (devs & SOUND_MASK_VOLUME)
				cmd = SOUND_MIXER_READ_VOLUME;
			else
			{
				close(fd);
				return;
			}
			ioctl(fd, cmd, &v);
			*r = (v & 0xFF00) >> 8;
			*l = (v & 0x00FF);
			close(fd);
		}
	}
	else
#endif
	if (!cdda_cfg.use_oss_mixer)
	{

#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
		struct ioc_vol vol;

		if (cdda_fd != -1)
		{
			ioctl(cdda_fd, CDIOCGETVOL, &vol);
			*l = (100 * vol.vol[0]) / 255;
			*r = (100 * vol.vol[1]) / 255;
		}
#elif defined(CDROMVOLREAD)
		struct cdrom_volctrl vol;

		if (cdda_fd != -1)
		{
			ioctl(cdda_fd, CDROMVOLREAD, &vol);
			*l = (100 * vol.channel0) / 255;
			*r = (100 * vol.channel1) / 255;
		}
#else
		*l = volume_left;
		*r = volume_right;
#endif

	}
}

static void set_volume(int l, int r)
{
#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
	struct ioc_vol vol;
#else
	struct cdrom_volctrl vol;
#endif
	int fd, devs, cmd, v;

#if defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H)
	if (cdda_cfg.use_oss_mixer)
	{
		fd = open(DEV_MIXER, O_RDONLY);
		if (fd != -1)
		{
			ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
			if (devs & SOUND_MASK_CD)
				cmd = SOUND_MIXER_WRITE_CD;
			else if (devs & SOUND_MASK_VOLUME)
				cmd = SOUND_MIXER_WRITE_VOLUME;
			else
			{
				close(fd);
				return;
			}
			v = (r << 8) | l;
			ioctl(fd, cmd, &v);
			close(fd);
		}
	}
	else
#endif
	{
		if (cdda_fd != -1)
		{
#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
			vol.vol[0] = vol.vol[2] = (l * 255) / 100;
			vol.vol[1] = vol.vol[3] = (r * 255) / 100;
			ioctl(cdda_fd, CDIOCSETVOL, &vol);
#else
			vol.channel0 = vol.channel2 = (l * 255) / 100;
			vol.channel1 = vol.channel3 = (r * 255) / 100;
			ioctl(cdda_fd, CDROMVOLCTRL, &vol);
#endif
		}
#if !defined(CDROMVOLREAD) && !(defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__)))
		volume_left = l;
		volume_right = r;
#endif
	}
}

gboolean cdda_get_toc(cdda_disc_toc_t *info)
{
	gboolean retv = FALSE;

#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
	struct ioc_toc_header tochdr;
#if defined(__OpenBSD__)
	struct ioc_read_toc_entry tocentry;
#else
	struct ioc_read_toc_single_entry tocentry;
#endif
#else	
	struct cdrom_tochdr tochdr;
	struct cdrom_tocentry tocentry;
#endif
	gint i;
	gint fd;

	if (is_mounted(cdda_cfg.device))
		return FALSE;

	if ((fd = open(cdda_cfg.device, CDOPENFLAGS)) == -1)
		return FALSE;

	memset(info, 0, sizeof(cdda_disc_toc_t));

#if defined(HAVE_SYS_CDIO_H) && (defined(__FreeBSD__) || defined(__OpenBSD__))
	if ( ioctl(fd, CDIOREADTOCHEADER, &tochdr) )
	    goto done;

	for (i = tochdr.starting_track; i <= tochdr.ending_track; i++)
	{		
		tocentry.address_format = CD_MSF_FORMAT;

#if defined(__OpenBSD__)
		tocentry.starting_track = i;
		if (ioctl(fd, CDIOREADTOCENTRYS, &tocentry))
			goto done;
		info->track[i].minute =
		    tocentry.data->addr.msf.minute;
		info->track[i].second =
		    tocentry.data->addr.msf.second;
		info->track[i].frame =
		    tocentry.data->addr.msf.frame;
		info->track[i].flags.data_track =
		    (tocentry.data->control & 4) == 4;
#else
		tocentry.track = i;
		if (ioctl(fd, CDIOREADTOCENTRY, &tocentry))
			goto done;
		info->track[i].minute =
		    tocentry.entry.addr.msf.minute;
		info->track[i].second =
		    tocentry.entry.addr.msf.second;
		info->track[i].frame =
		    tocentry.entry.addr.msf.frame;
		info->track[i].flags.data_track =
		    (tocentry.entry.control & 4) == 4;
#endif
	}

	/* Get the leadout track */
	tocentry.address_format = CD_MSF_FORMAT;

#if defined(__OpenBSD__)
	tocentry.starting_track = 0xAA;
	if (ioctl(fd, CDIOREADTOCENTRYS, &tocentry))
		goto done;
	info->leadout.minute = tocentry.data->addr.msf.minute;
	info->leadout.second = tocentry.data->addr.msf.second;
	info->leadout.frame = tocentry.data->addr.msf.frame;
#else
	tocentry.track = 0xAA;
	if (ioctl(fd, CDIOREADTOCENTRY, &tocentry))
		goto done;
	info->leadout.minute = tocentry.entry.addr.msf.minute;
	info->leadout.second = tocentry.entry.addr.msf.second;
	info->leadout.frame = tocentry.entry.addr.msf.frame;
#endif
	
	info->first_track = tochdr.starting_track;
	info->last_track = tochdr.ending_track;
	retv = TRUE;

#else
	if (ioctl(fd, CDROMREADTOCHDR, &tochdr))
		goto done;

	for (i = tochdr.cdth_trk0; i <= tochdr.cdth_trk1; i++)
	{		
		tocentry.cdte_format = CDROM_MSF;
		tocentry.cdte_track = i;
		if (ioctl(fd, CDROMREADTOCENTRY, &tocentry))
			goto done;
		info->track[i].minute = tocentry.cdte_addr.msf.minute;
		info->track[i].second = tocentry.cdte_addr.msf.second;
		info->track[i].frame = tocentry.cdte_addr.msf.frame;
		info->track[i].flags.data_track = tocentry.cdte_ctrl == CDROM_DATA_TRACK;
	}

	/* Get the leadout track */
	tocentry.cdte_track = CDROM_LEADOUT;
	tocentry.cdte_format = CDROM_MSF;
	if (ioctl(fd, CDROMREADTOCENTRY, &tocentry))
		goto done;
	info->leadout.minute = tocentry.cdte_addr.msf.minute;
	info->leadout.second = tocentry.cdte_addr.msf.second;
	info->leadout.frame = tocentry.cdte_addr.msf.frame;
	
	info->first_track = tochdr.cdth_trk0;
	info->last_track = tochdr.cdth_trk1;
	retv = TRUE;
#endif

 done:
	close(fd);

	return retv;
}
