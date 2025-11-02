/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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

#include "xmms.h"

#ifdef WITH_SM
#include <X11/SM/SMlib.h>

void mainwin_quit_cb(void);

static SmcConn smc_conn = NULL;
static IceConn ice_conn;
static char *session_id;

static void sm_save_yourself(SmcConn c, SmPointer p, int save_type, Bool shutdown,
			     int interact_style, Bool fast)
{
	GDK_THREADS_ENTER();
	save_config();
	SmcSaveYourselfDone(c, TRUE);
	GDK_THREADS_LEAVE();
}

static void sm_shutdown_cancelled(SmcConn c, SmPointer a)
{
}

void sm_save_complete(SmcConn c, SmPointer a)
{
}

static void sm_die(SmcConn c, SmPointer arg)
{
	GDK_THREADS_ENTER();
	mainwin_quit_cb();
	GDK_THREADS_LEAVE();
}

static void ice_handler(gpointer data, gint source, GdkInputCondition condition)
{
	IceProcessMessages(data, NULL, NULL);
}

void sm_init(int argc, char **argv)
{
	SmPropValue program_val, restart_val;
	SmPropValue userid_val  = { 0, NULL };
	SmProp program_prop, userid_prop, restart_prop, clone_prop, *props[4];
	SmcCallbacks smcall;
	char errstr[256];

	program_val.length = strlen(PACKAGE);
	program_val.value = PACKAGE;
	restart_val.length = strlen(argv[0]);
	restart_val.value = argv[0];
	program_prop.name = SmProgram;
	program_prop.type = SmARRAY8;
	program_prop.num_vals = 1;
	program_prop.vals = &program_val;
	userid_prop.name = SmProgram;
	userid_prop.type = SmARRAY8;
	userid_prop.num_vals = 1;
	userid_prop.vals = &userid_val;
	restart_prop.name = SmRestartCommand;
	restart_prop.type = SmLISTofARRAY8;
	restart_prop.num_vals = 1;
	restart_prop.vals = &restart_val;
	clone_prop.name = SmCloneCommand;
	clone_prop.type = SmLISTofARRAY8;
	clone_prop.num_vals = 1;
	clone_prop.vals = &restart_val;

	props[0] = &program_prop;
	props[1] = &userid_prop;
	props[2] = &restart_prop;
	props[3] = &clone_prop;


	if (!getenv("SESSION_MANAGER"))
		return;

	memset(&smcall, 0, sizeof(smcall));
	userid_val.value = g_strdup_printf("%d", geteuid());
	userid_val.length = strlen(userid_val.value);
	smcall.save_yourself.callback = sm_save_yourself;
	smcall.die.callback = sm_die;
	smcall.save_complete.callback = sm_save_complete;
	smcall.shutdown_cancelled.callback = sm_shutdown_cancelled;

	smc_conn = SmcOpenConnection(NULL, NULL, SmProtoMajor, SmProtoMinor,
				     SmcSaveYourselfProcMask |
				     SmcSaveCompleteProcMask |
				     SmcShutdownCancelledProcMask |
				     SmcDieProcMask,
				     &smcall, NULL, &session_id,
				     sizeof(errstr), errstr);
	if (!smc_conn)
		return;
	SmcSetProperties(smc_conn, sizeof(props) / sizeof(props[0]),
			 (SmProp **)&props);
	ice_conn = SmcGetIceConnection(smc_conn);
	gdk_input_add(IceConnectionNumber(ice_conn), GDK_INPUT_READ,
		      ice_handler, ice_conn);
}

void sm_cleanup(void)
{
	if (smc_conn)
		SmcCloseConnection(smc_conn, 0, NULL);
}

#else

void sm_init(int argc, char **argv)
{
}

void sm_cleanup(void)
{
}


#endif

