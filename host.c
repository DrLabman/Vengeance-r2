/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"
#include "NeuralNets.h"

/*
A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.
*/

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution

double		host_frametime;
double		host_time;
double		realtime;				// without any filtering or bounding
double		oldrealtime;			// last frame run
int			host_framecount;

int			host_hunklevel;
int			video_hunklevel;
int			server_hunklevel;

int			minimum_memory;

client_t	*host_client;			// current client

jmp_buf 	host_abortserver;

byte		*host_basepal;
byte		*host_colormap;

cvar_t	host_framerate = {"host_framerate","0"};	// set for slow motion
cvar_t	host_speeds = {"host_speeds","0"};			// set for running times

cvar_t	sys_ticrate = {"sys_ticrate","0.05"};
cvar_t	serverprofile = {"serverprofile","0"};

cvar_t	fraglimit = {"fraglimit","0",false,true};
cvar_t	timelimit = {"timelimit","0",false,true};
cvar_t	teamplay = {"teamplay","0",false,true};

cvar_t	samelevel = {"samelevel","0"};
cvar_t	noexit = {"noexit","0",false,true};

#ifdef QUAKE2
cvar_t	developer = {"developer","1"};	// should be 0 for release!
#else
cvar_t	developer = {"developer","0"};
#endif

cvar_t	skill = {"skill","1"};						// 0 - 3
cvar_t	deathmatch = {"deathmatch","0"};			// 0, 1, or 2
cvar_t	coop = {"coop","0"};			// 0 or 1

cvar_t	pausable = {"pausable","1"};

cvar_t	temp1 = {"temp1","0"};

//QMB
cvar_t	max_fps = {"max_fps","72",true};	// set the max fps
cvar_t	show_fps = {"show_fps","0",true};	// set for running times - muff
int			fps_count;
// CSL - epca@powerup.com.au
// Use bspextensions to determine changes in format to bsp files
int bspextensions = 0;
// CSL 

/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,message);
	vsprintf (string,message,argptr);
	va_end (argptr);
	Con_DPrintf ("Host_EndGame: %s\n",string);
	
	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit
	
	if (cls.demonum != -1)
		CL_NextDemo ();
	else
		CL_Disconnect ();

	longjmp (host_abortserver, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;
	
	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;
	
	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);
	
	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n",string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

	longjmp (host_abortserver, 1);
}

/*
================
Host_FindMaxClients
================
*/
void	Host_FindMaxClients (void)
{
	// Entar : maxclients default 16
	int		i;

	svs.maxclients = 1;
		
	i = COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (com_argc - 1))
		{
			svs.maxclients = Q_atoi (com_argv[i+1]);
		}
		else
			svs.maxclients = 16;
	}
	else
		cls.state = ca_disconnected;


	i = COM_CheckParm ("-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");
		if (i != (com_argc - 1))
			svs.maxclients = Q_atoi (com_argv[i+1]);
		else
			svs.maxclients = 16;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 16;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = Hunk_AllocName (svs.maxclientslimit*sizeof(client_t), "clients");

	if (svs.maxclients > 1)
		Cvar_SetValue ("deathmatch", 1.0);
	else
		Cvar_SetValue ("deathmatch", 0.0);
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal (void)
{
	Host_InitCommands ();
	
	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterVariable (&host_speeds);

	Cvar_RegisterVariable (&sys_ticrate);
	Cvar_RegisterVariable (&serverprofile);

	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&timelimit);
	Cvar_RegisterVariable (&teamplay);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&noexit);
	Cvar_RegisterVariable (&skill);
	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&coop);

	Cvar_RegisterVariable (&pausable);

	Cvar_RegisterVariable (&temp1);

	Host_FindMaxClients ();

	Cvar_RegisterVariable (&show_fps); // muff QMB
	Cvar_RegisterVariable (&max_fps);

	host_time = 1.0;		// so a think at time 0 won't get called
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (void)
{
	vfsfile_t	*f;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	if (host_initialized & !isDedicated)
	{
//		f = fopen (va("%s/config.cfg",com_gamedir), "w");
		f = FS_OpenVFS (va("config.cfg"), "wb", FS_GAMEONLY);
		if (!f)
		{
			Con_Printf ("Couldn't write config.cfg.\n");
			return;
		}
		
		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

//		fclose (f);
		VFS_CLOSE(f);
	}
}


/*
=================
SV_ClientPrintf

Sends text across to be displayed 
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);
	
	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			i;
	
	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);
	
	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
}

/*
=================
SV_SendSlowmoValue

Sends slowmo value to all active clients
=================
*/
void SV_SendSlowmoValue ()
{
	int			i;
	
	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
//			MSG_WriteByte (&svs.clients[i].message, svc_updateslowmo);
//			MSG_WriteFloat (&svs.clients[i].message, slowmo.value);
			MSG_WriteByte(&svs.clients[i].message, svc_stufftext);
			MSG_WriteString(&svs.clients[i].message, va("slowmo %f\n", slowmo.value));
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);
	
	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient (qboolean crash)
{
	int		saveSelf;
	int		i;
	client_t *client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}
	
		if (host_client->edict && host_client->spawned)
		{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf ("Client %s removed\n",host_client->name);
	}

// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int		i;
	int		count;
	sizebuf_t	buf;
	char		message[4];
	double	start;

	if (!sv.active)
		return;

	sv.active = false;

// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

// flush any pending messages - like the score!!!
	start = Sys_FloatTime();
	do
	{
		count = 0;
		for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_FloatTime() - start) > 3.0)
			break;
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.data = message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_DropClient(crash);

//
// clear structures
//
	memset (&sv, 0, sizeof(sv));
	memset (svs.clients, 0, svs.maxclientslimit*sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (void)
{
	Con_DPrintf ("Clearing memory\n");
	Mod_ClearAll ();
	if (video_hunklevel)
		Hunk_FreeToLowMark (video_hunklevel);

	if (server_hunklevel)
		Hunk_FreeToHighMark (server_hunklevel);

	cls.signon = 0;
	memset (&sv, 0, sizeof(sv));
	memset (&cl, 0, sizeof(cl));
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
float old_slowmo;
extern int SND_speed;

qboolean Host_FilterTime (float time)
{
	extern	void FreeSound (void);
	realtime += time;

	if (max_fps.value>0)
	{
		    // CAPTURE <anthony@planetquake.com>
		if (!cls.capturedemo) // only allow the following early return if not capturing:
			if (!cls.timedemo && realtime - oldrealtime < 1.0/max_fps.value)
				return false;		// framerate is too high
	}

	//host_frametime = realtime - oldrealtime;
	host_frametime = (realtime - oldrealtime) * slowmo.value;
	oldrealtime = realtime;

	if(slowmo.value != old_slowmo)
	{
			if(sv.active)
			{
				SV_SendSlowmoValue();	//(515): update slowmo for all clients
			}
//			if(slowmo.value > 2)
//				SND_speed = 88200;
//			else
//				SND_speed = 44100*slowmo.value;
			S_BlockSound();
			FreeSound();
			SNDDMA_Init();
			S_UnblockSound();
			old_slowmo = slowmo.value;
	}

	if (host_framerate.value > 0)
		host_frametime = (float)host_framerate.value;
	else
	{	// don't allow really long or short frames
		if (host_frametime > 0.1)
			host_frametime = 0.1;
		if (host_frametime < 0.001)
			host_frametime = 0.001;
	}
	
	return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}


/*
==================
Host_ServerFrame

==================
*/
#ifdef FPS_20

void _Host_ServerFrame (void)
{
// run the world state	
	pr_global_struct->frametime = host_frametime;

// read client messages
	SV_RunClients ();
	
// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();
}

void Host_ServerFrame (void)
{
	float	save_host_frametime;
	float	temp_host_frametime;

// run the world state	
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();
	
// check for new clients
	SV_CheckForNewClients ();

	temp_host_frametime = save_host_frametime = host_frametime;
	while(temp_host_frametime > (1.0/72.0))
	{
		if (temp_host_frametime > 0.05)
			host_frametime = 0.05;
		else
			host_frametime = temp_host_frametime;
		temp_host_frametime -= host_frametime;
		_Host_ServerFrame ();
	}
	host_frametime = save_host_frametime;

// send all messages to the clients
	SV_SendClientMessages ();
}

#else

void Host_ServerFrame (void)
{
// run the world state	
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();
	
// check for new clients
	SV_CheckForNewClients ();

// read client messages
	SV_RunClients ();
	
// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();

// send all messages to the clients
	SV_SendClientMessages ();
}

#endif


/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame (float time)
{
	static double		time1 = 0;
	static double		time2 = 0;
	static double		time3 = 0;
	int			pass1, pass2, pass3;

	if (setjmp (host_abortserver) )
		return;			// something bad happened, or the server disconnected

// keep the random time dependent
	rand ();
	
// decide the simulation time
	if (!Host_FilterTime (time))
		return;			// don't run too fast, or packets will flood out
		
// get new key events
	Sys_SendKeyEvents ();

// allow mice or other external controllers to add commands
	IN_Commands ();

// process console commands
	Cbuf_Execute ();

	NET_Poll();

// if running the server locally, make intentions now
	if (sv.active)
		CL_SendCmd ();
	
//-------------------
//
// server operations
//
//-------------------

// check for commands typed to the host
	Host_GetConsoleCommands ();
	
	if (sv.active)
		Host_ServerFrame ();

//-------------------
//
// client operations
//
//-------------------

// if running the server remotely, send intentions now after
// the incoming messages have been read
	if (!sv.active)
		CL_SendCmd ();

	host_time += host_frametime;

// fetch results from server
	if (cls.state == ca_connected)
	{
		CL_ReadFromServer ();
	}

// update video
	if (host_speeds.value)
		time1 = Sys_FloatTime ();
		
	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_FloatTime ();
		
// update audio
	if (cls.signon == SIGNONS)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);
	
	CDAudio_Update();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_FloatTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n",
					pass1+pass2+pass3, pass1, pass2, pass3);
	}
	
	host_framecount++;
}

void Host_Frame (float time)
{
	double	time1, time2;
	static double	timetotal;
	static int		timecount;
	int		i, c, m;

	if (!serverprofile.value)
	{
		_Host_Frame (time);
		return;
	}
	
	time1 = Sys_FloatTime ();
	_Host_Frame (time);
	time2 = Sys_FloatTime ();	
	
	timetotal += time2 - time1;
	timecount++;
	
	if (timecount < 1000)
		return;

	m = timetotal*1000/timecount;
	timecount = 0;
	timetotal = 0;
	c = 0;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		if (svs.clients[i].active)
			c++;
	}

//frame speed counter QMB
	fps_count++;//muff

	Con_Printf ("serverprofile: %2i clients %2i msec\n",  c,  m);
}

//============================================================================

extern char	skyname[32];
qboolean vid_initialized=false;
void Host_InitVideo(void)
{
	if (vid_initialized)
		return;
	vid_initialized = true;

	host_basepal = (byte *)COM_LoadHunkFile ("gfx/palette.lmp");
	if (!host_basepal)
		Sys_Error ("Couldn't load gfx/palette.lmp");
	host_colormap = (byte *)COM_LoadHunkFile ("gfx/colormap.lmp");
	if (!host_colormap)
		Sys_Error ("Couldn't load gfx/colormap.lmp");

	W_LoadWadFile ("gfx.wad");

#ifndef _WIN32 // on non win32, mouse comes before video for security reasons
	IN_Init ();
#endif
	VID_Init (host_basepal);

	Draw_Init ();
	SCR_Init ();
	R_Init ();
#ifndef	_WIN32
// on Win32, sound initialization has to come before video initialization, so we
// can put up a popup if the sound hardware is in use
	S_Init ();
#else

#ifdef	GLQUAKE
// FIXME: doesn't use the new one-window approach yet
	S_Init ();
#endif

#endif	// _WIN32
	CDAudio_Init ();
	Sbar_Init ();
	CL_InitTEnts ();
#ifdef _WIN32 // on non win32, mouse comes before video for security reasons
	IN_Init ();
#endif

	video_hunklevel = Hunk_LowMark ();

	if (skyname[0])
		R_LoadSky(skyname);
}

extern void SCR_Shutdown(void), Draw_Shutdown(void), Mod_Shutdown(void);

void Host_DeInitVideo(void)
{
	if (!vid_initialized)
		return;
	vid_initialized=false;
	cl.worldmodel = NULL;

	SCR_Shutdown ();
	IN_Shutdown  ();
//	CL_DeInitTEnts ();
//	Sbar_Shutdown ();
	CDAudio_Shutdown ();
	S_Shutdown ();
//	R_Shutdown ();
	Draw_Shutdown ();
	VID_Shutdown ();

	Mod_Shutdown();

	Cache_Flush();

	Hunk_FreeToLowMark (host_hunklevel);	//strip right back

	video_hunklevel = host_hunklevel;
}

void Host_VidRestart_f(void)
{
/*
	if (sv.active)
	{
		Con_Printf("vid_restart refused while server is active\nDisconnect first\n");
		return;
	}
*/
	Host_DeInitVideo();

	Host_InitVideo();

	if (sv.active)
	{
		int i;
		//we might be getting a little behind
		//so send out messages
//		SV_SendClientMessages();

		for (i=1 ; i<MAX_MODELS && sv.model_precache[i] ; i++)
		{
			sv.models[i] = Mod_ForName (sv.model_precache[i], false);
		}
		sv.worldmodel = sv.models[1];

		//model loading might have taken a while, so send again
		SV_SendClientMessages();
	}

	if (cls.state == ca_connected)
	{
		int i;
	//
	// now we try to load everything else until a cache allocation fails
	//
		for (i=1 ; i<MAX_MODELS && cl.model_precache_name[i][0]; i++)
		{
			cl.model_precache[i] = Mod_ForName (cl.model_precache_name[i], false);
			if (cl.model_precache[i] == NULL)
			{
				Con_Printf("Model %s not found\n", cl.model_precache_name[i]);
				return;
			}
		}

		S_BeginPrecaching ();
		for (i=1 ; i<MAX_SOUNDS && cl.sound_precache_name[i][0]; i++)
		{
			cl.sound_precache[i] = S_PrecacheSound (cl.sound_precache_name[i]);
		}
		S_EndPrecaching ();

	// local state
		cl_entities[0].model = cl.worldmodel = cl.model_precache[1];
		
		R_NewMap ();

		for (i=0; i < cl.num_statics; i++)
		{
			R_AddEfrags(&cl_static_entities[i]);
			cl_static_entities[i].model = cl.model_precache[cl_static_entities[i].baseline.modelindex];
		}

		Hunk_Check ();		// make sure nothing is hurt
	}
}

/*
====================
Host_Init
====================
*/
//qmb :globot
void Bot_Init (void);
void Host_Init (quakeparms_t *parms)
{
	extern	cvar_t	vid_width, vid_height, vid_bitsperpixel, vid_fullscreen;
	extern	void	IN_Init_Register(void), R_Init_Register(void), Draw_Init_Register(void), VID_Init_Register(void), SCR_Init_Register(void),
		Sbar_Init_Register(void), R_InitParticles_Register(void), S_Init_Register(void), CaptureHelper_Init_Register(void),	V_Init_Register(void);
	extern	int		findbpp;
	extern	long	time(long *);

	qboolean		findheight=false;
	// LordHavoc: quake never seeded the random number generator before... heh
	srand(time(0));	

	if (standard_quake)
		minimum_memory = MINIMUM_MEMORY;
	else
		minimum_memory = MINIMUM_MEMORY_LEVELPAK;

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = minimum_memory;

	host_parms = *parms;

	if (parms->memsize < minimum_memory)
		Sys_Error ("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

	com_argc = parms->argc;
	com_argv = parms->argv;

	Memory_Init (parms->membase, parms->memsize);
	Cbuf_Init ();
	Cmd_Init ();
	V_Init ();
	//Qmb :NeuralNets
	NN_Init();

	Chase_Init ();
	COM_Init (parms->basedir);
	Host_InitLocal ();

	Key_Init ();
	Con_Init ();	
	M_Init ();	
	PR_Init ();
	Mod_Init ();
	NET_Init ();
	SV_Init ();
	//Qmb :globot
	Bot_Init ();
	
	CL_Init();	//just registers some cvars/commands
	IN_Init_Register();
	R_Init_Register();
	Draw_Init_Register();
	VID_Init_Register();
	SCR_Init_Register();
	Sbar_Init_Register();
	R_InitParticles_Register();
	S_Init_Register();
	CaptureHelper_Init_Register();
	V_Init_Register();

	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	if (extra_info)
		Con_Printf ("%4.1f megabyte heap\n",parms->memsize/ (1024*1024.0));
	
//	R_InitTextures ();		// needed even for dedicated servers

	Cmd_AddCommand("vid_restart", Host_VidRestart_f);
	Cvar_RegisterVariable (&vid_width);
	Cvar_RegisterVariable (&vid_height);
	Cvar_RegisterVariable (&vid_bitsperpixel);
	Cvar_RegisterVariable (&vid_fullscreen);

	Hunk_AllocName (0, "-VID_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	server_hunklevel = Hunk_HighMark ();

	host_initialized = true;
	
	Sys_Printf ("========Quake Initialized=========\n");	

		// Entar : for DP_CON_STARTMAP
	Cbuf_InsertText ("alias startmap_sp \"map start\"\n");
	Cbuf_InsertText ("alias startmap_dm \"map start\"\n");

	Cbuf_InsertText ("exec quake.rc\n");

	if (!sv.active && (cls.state == ca_dedicated || COM_CheckParm("-listen")))
		Cbuf_InsertText ("startmap_dm\n");

	Cbuf_Execute();

	if (COM_CheckParm("-window"))
		Cvar_SetValueQuick(&vid_fullscreen, 0);

	if (COM_CheckParm("-width"))
	{
		Cvar_SetValueQuick(&vid_width, Q_atoi(com_argv[COM_CheckParm("-width")+1]));
		vid.realwidth = Q_atoi(com_argv[COM_CheckParm("-width")+1]);
		findheight = true;
	}

	if (COM_CheckParm("-bpp"))
	{
		Cvar_SetValueQuick(&vid_bitsperpixel, Q_atoi(com_argv[COM_CheckParm("-bpp")+1]));
		findbpp = 0;
	}
	else
	{
		Cvar_SetValueQuick(&vid_bitsperpixel, 32); //qmb :32bit colour
		findbpp = 1;

		//qmb :16 bit defualt on 3dfx cards
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			Cvar_SetValueQuick(&vid_bitsperpixel, 16); //qmb :32bit colour
	}

	if (COM_CheckParm("-height"))
	{
		Cvar_SetValueQuick(&vid_height, Q_atoi(com_argv[COM_CheckParm("-height")+1]));
		vid.realheight = Q_atoi(com_argv[COM_CheckParm("-height")+1]);
	}
	else if (findheight) // if we got width, we can't count on the config, so default to 4:3
	{
		Cvar_SetValueQuick(&vid_height, vid_width.value/4*3); // default to a 4:3 ratio
		vid.realheight = vid_width.value/4*3;
	}

	if (cls.state != ca_dedicated)
	{
		Host_InitVideo();
	}
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
extern qboolean con_debuglog;
void Con_CloseDebugLog(void);

void Host_Shutdown(void)
{
	static qboolean isdown = false;
	
	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration (); 

	NN_Deinit();

#if 1
	Host_DeInitVideo();
#else

	CDAudio_Shutdown ();
	NET_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();


	if (cls.state != ca_dedicated)
	{
		VID_Shutdown();
	}
#endif
	if (con_debuglog)
	{
		Con_CloseDebugLog ();
	}
}
