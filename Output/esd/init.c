/*      xmms - esound output plugin
 *    Copyright (C) 1999      Galex Yen
 *      
 *      this program is free software
 *      
 *      Description:
 *              This program is an output plugin for xmms v0.9 or greater.
 *              The program uses the esound daemon to output audio in order
 *              to allow more than one program to play audio on the same
 *              device at the same time.
 *
 *              Contains code Copyright (C) 1998-1999 Mikael Alm, Olle Hallnas,
 *              Thomas Nillson and 4Front Technologies
 *
 */

#include "esdout.h"
#include "libxmms/configfile.h"

ESDConfig esd_cfg;
esd_info_t *all_info;
esd_player_info_t *player_info;

void esdout_init(void)
{
	ConfigFile *cfgfile;
	char *env;

	memset(&esd_cfg, 0, sizeof (ESDConfig));
	esd_cfg.port = ESD_DEFAULT_PORT;
	esd_cfg.buffer_size = 3000;
	esd_cfg.prebuffer = 25;

	cfgfile = xmms_cfg_open_default_file();

	if ((env = getenv("ESPEAKER")) != NULL)
	{
		char *temp;
		esd_cfg.use_remote = TRUE;
		esd_cfg.server = g_strdup(env);
		temp = strchr(esd_cfg.server,':');
		if (temp != NULL)
		{
			*temp = '\0';
			esd_cfg.port = atoi(temp+1);
			if (esd_cfg.port == 0)
				esd_cfg.port = ESD_DEFAULT_PORT;
		}
	}
	else
	{
		xmms_cfg_read_boolean(cfgfile, "ESD", "use_remote",
				      &esd_cfg.use_remote);
		xmms_cfg_read_string(cfgfile, "ESD", "remote_host",
				     &esd_cfg.server);
		xmms_cfg_read_int(cfgfile, "ESD", "remote_port", &esd_cfg.port);
	}
	xmms_cfg_read_boolean(cfgfile, "ESD", "use_oss_mixer",
			      &esd_cfg.use_oss_mixer);
	xmms_cfg_read_int(cfgfile, "ESD", "buffer_size", &esd_cfg.buffer_size);
	xmms_cfg_read_int(cfgfile, "ESD", "prebuffer", &esd_cfg.prebuffer);
	xmms_cfg_free(cfgfile);

	if (!esd_cfg.server)
		esd_cfg.server = g_strdup("localhost");
}
