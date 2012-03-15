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
#include "quakedef.h"

#ifdef _WIN32
#include "winquake.h"
#endif

extern	cvar_t	hud, crosshair_static;

void (*vid_menudrawfn)(void);

enum {m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup, m_net, m_options, m_video, m_keys, m_help, m_quit, m_serialconfig, m_modemconfig, m_lanconfig, m_gameoptions, m_search, m_slist, m_vid_options, m_gfx_options, m_credits, m_serveroptions} m_state;

extern POINT current_pos, last_pos;
extern int omousex, omousey, old_mouse_x, old_mouse_y;

int	m_main_cursor;

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
		void M_Menu_Help_f (void);
		void M_Menu_Credits_f (void); //JHL:ADD
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_Net_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
	void M_Menu_Video_f (void);	//JHL:ADD
			void M_Menu_VideoModes_f (void);
	void M_Menu_Quit_f (void);
void M_Menu_SerialConfig_f (void);
	void M_Menu_ModemConfig_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
		void M_Help_Draw (void);
		void M_Credits_Draw (void); //JHL:ADD
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
		void M_Net_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		void M_Server_Draw (void);
	void M_Video_Draw (void);	//JHL:ADD; advanced video options
		void M_VideoModes_Draw (void);
	void M_Quit_Draw (void);
void M_SerialConfig_Draw (void);
	void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
		void M_Help_Key (int key);
		void M_Credits_Key (int key); //JHL:ADD
	void M_MultiPlayer_Key (int key);
		void M_Setup_Key (int key);
		void M_Net_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
	void M_Video_Key (int key);
		void M_VideoModes_Key (int key);
	void M_Quit_Key (int key);
void M_SerialConfig_Key (int key);
	void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int			m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define	IPXConfig		(m_net_cursor == 2)
#define	TCPIPConfig		(m_net_cursor == 3)

void M_ConfigureNetSubsystem(void);

//JHL: My personal defines for making menu coding easier...
void Draw_AlphaFill (int x, int y, int w, int h, vec3_t c, float alpha);
void Draw_AlphaFillFade (int x, int y, int width, int height, vec3_t colour, float alpha[2]);

extern cvar_t	crosshair;

#define BUTTON_HEIGHT	10
#define BUTTON_START	50
#define	BUTTON_MENU_X	100
#define LAYOUT_RED		116

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character ( cx + ((vid.width - 320)>>1), line + ((vid.height/2)-120), num);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

//JHL:ADD; center print
void M_Centerprint (int cy, char *str)
{
	int cx;
	cx = vid.width/2 - (strlen(str)*4);

	while (*str)
	{
		Draw_Character ( cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}
//JHL:ADD; center print
void M_CenterprintWhite (int cy, char *str)
{
	int cx;
	cx = vid.width/2 - (strlen(str)*4);

	while (*str)
	{
		Draw_Character ( cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_TransPic (x + ((vid.width - 320)>>1), y+(vid.height/2-120), pic);
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_AlphaPic (x + ((vid.width - 320)>>1), y+(vid.height/2-120), pic, 1);
}

//=======================================================
/*
================
M_Main_Layout

JHL:ADD; Draws the main menu in desired manner
================
*/
void PrintRed (int cx, int cy, char *str)
{
	while (*str)
	{
		Draw_Character (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		Draw_Character (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_Main_ButtonList (char *buttons[], int cursor_location, int in_main)
{
	int	x, x2, y,
		x_mod,
		x_length,
		i, num;

	x_length = 0;

	for ( i = 0, num=0; buttons[i] != 0; i++ )
	{
		x_length = x_length + (strlen(buttons[i])*8);
		num++;
	}

	x_mod = (vid.width - x_length) / (i+1);
	y = vid.height / 14;
	x = x2 = 0;

	// mouse stuff
	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && in_main && (current_pos.y - omousey) < vid.height / 10)
	{
		for (i=num; i > 0; i--)
		{
			x2 = x2 + x_mod;
			if (current_pos.x - omousex > x2)
			{
				if (current_pos.x - omousex < (x2 + (x_length / num)))
				{
					m_main_cursor = num - i;
					break;
				}
			}
			x2 = x2 + (x_length / num);
		}
	}

	for ( i = 0; buttons[i] != 0; i++ )
	{	// center on point origin
		x = x + x_mod;
		if (cursor_location == i)
		{
			PrintWhite (x, y, buttons[i]);
			if (in_main == true)
				Draw_Character (x-10, y, 12+((int)(realtime*4)&1));
		}
		else
			PrintRed (x, y, buttons[i]);
		x = x + (strlen(buttons[i])*8);
	}
}

void M_Main_Layout (int f_cursor, int f_inmenu)
{
	char	*names[] =
	{
		"Single",
		"Multiplayer",
		"Options",
		"Video",
		"Quit",
		0
	};

	if (gamemode == GAME_CHRONICLE)
		strcpy(names[0], "Credits");

// the layout
	// top
	Draw_Fill (0,0,vid.width, vid.height/8, 0);
	Draw_Fill (0,vid.height/16,vid.width, 1, LAYOUT_RED);
	
	// bottom
	Draw_Fill (0,vid.height-24,vid.width, vid.height, 0);
	// bottom fade

	M_Main_ButtonList (names, f_cursor, f_inmenu);

	//JHL:HACK; In submenu, so do the background
	if (f_inmenu == false)
		Draw_FadeScreen ();
}

//======================================================

byte identityTable[256];
byte translationTable[256];

void M_BuildTranslationTable(int top, int bottom)
{
	int		j;
	byte	*dest, *source;

	for (j = 0; j < 256; j++)
		identityTable[j] = j;
	dest = translationTable;
	source = identityTable;
	memcpy (dest, source, 256);

	if (top < 128)	// the artists made some backwards ranges.  sigh.
		memcpy (dest + TOP_RANGE, source + top, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[TOP_RANGE+j] = source[top+15-j];

	if (bottom < 128)
		memcpy (dest + BOTTOM_RANGE, source + bottom, 16);
	else
		for (j=0 ; j<16 ; j++)
			dest[BOTTOM_RANGE+j] = source[bottom+15-j];
}


void M_DrawTransPicTranslate (int x, int y, qpic_t *pic)
{
	Draw_TransPicTranslate (x + ((vid.width - 320)>>1), y + ((vid.height/2)-120), pic, translationTable);
}


void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);
	
	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

//=============================================================================

int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f (void)
{
	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f ();
			return;
		}
		key_dest = key_game;
		m_state = m_none;
		return;
	}
	if (key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		M_Menu_Main_f ();
	}
}


//=============================================================================
/* MAIN MENU */

#define	MAIN_ITEMS	5

#define		M_M_SINGLE	0
#define		M_M_MULTI	1
#define		M_M_OPTION	2
#define		M_M_VIDEO	3
#define		M_M_QUIT	4

void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;
}

void M_Main_Draw (void)
{
	M_Main_Layout (m_main_cursor, true);
}

void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		current_pos.x = window_center_x;
		current_pos.y = window_center_y;
		old_mouse_x = 0;
		old_mouse_y = 0;
		IN_ClearStates();
		SetCursorPos(window_center_x, window_center_y);

		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
		break;

	case K_RIGHTARROW:
	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case M_M_SINGLE:
			if (gamemode != GAME_CHRONICLE)
				M_Menu_SinglePlayer_f ();
			else
				M_Menu_Credits_f ();
			break;

		case M_M_MULTI:
			M_Menu_MultiPlayer_f ();
			break;

		case M_M_OPTION:
			M_Menu_Options_f ();
			break;

		case M_M_VIDEO:
			M_Menu_Video_f (); // video options
			break;

		case M_M_QUIT:
			M_Menu_Quit_f ();
			break;
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

#define	SINGLEPLAYER_ITEMS	5

#define	GAME_NEW		0
#define	GAME_LOAD		1
#define	GAME_SAVE		2
#define	GAME_HELP		3
#define	GAME_CREDITS	4

int game_cursor_table[] = {BUTTON_START,
	   					   BUTTON_START + BUTTON_HEIGHT,
						   BUTTON_START + BUTTON_HEIGHT*2,
						   BUTTON_START + BUTTON_HEIGHT*4,
						   BUTTON_START + BUTTON_HEIGHT*5};
int	m_singleplayer_cursor;

int	dim_load, dim_save;	//JHL:check if available

void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;
}

void M_SinglePlayer_Draw (void)
{
	qpic_t	*p;
	char	*names[] =
	{
		"New game",
		"Load",
		"Save",
		"Help",
		"Credits",
		0
	};
	int i;

	M_Main_Layout (M_M_SINGLE, false);

	p = Draw_CachePic ("gfx/ttl_sgl.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	dim_load = false;
	dim_save = false;

	//JHL:HACK; dim "save"/"load" if we can't choose 'em!
	if (svs.maxclients != 1 || deathmatch.value || coop.value)
	{
		dim_load = true;
		dim_save = true;
	}
	
	if (!sv.active || cl.intermission)
		dim_save = true;

	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex) >= BUTTON_MENU_X)
	{
		for (i=4; i >= 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > game_cursor_table[GAME_NEW+i-1])
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < game_cursor_table[GAME_NEW+i])
				{
					m_singleplayer_cursor = i;
					break;
				}
			}
		}
	}

	M_PrintWhite (BUTTON_MENU_X, game_cursor_table[GAME_NEW], names[0]);
	
	if (dim_load)
		M_Print (BUTTON_MENU_X, game_cursor_table[GAME_LOAD], names[1]);
	else
		M_PrintWhite (BUTTON_MENU_X, game_cursor_table[GAME_LOAD], names[1]);

	if (dim_save)
		M_Print (BUTTON_MENU_X, game_cursor_table[GAME_SAVE], names[2]);
	else
		M_PrintWhite (BUTTON_MENU_X, game_cursor_table[GAME_SAVE], names[2]);

	M_PrintWhite (BUTTON_MENU_X, game_cursor_table[GAME_HELP], names[3]);
	M_PrintWhite (BUTTON_MENU_X, game_cursor_table[GAME_CREDITS], names[4]);

	if ((m_singleplayer_cursor == 1 && dim_load == true) || (m_singleplayer_cursor == 2 && dim_save == true))
		m_singleplayer_cursor = 0;

// cursor
	M_DrawCharacter (BUTTON_MENU_X - 10, game_cursor_table[m_singleplayer_cursor], 12+((int)(realtime*4)&1));
}

void M_SinglePlayer_Key (int key)
{
again:
	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case GAME_NEW:
			current_pos.x = window_center_x;
			current_pos.y = window_center_y;
			old_mouse_x = 0;
			old_mouse_y = 0;
			IN_ClearStates();
			SetCursorPos(window_center_x, window_center_y);

			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
					break;
			key_dest = key_game;
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("deathmatch 0\n");
			Cbuf_AddText ("coop 0\n");
//			Cbuf_AddText ("map start\n");
			Cbuf_AddText ("startmap_sp\n");
			break;

		case GAME_LOAD:
			// JHL:BUG-FIX; ever tried to load SP game while playing DM?
			if (dim_load == true)
				m_entersound = false;
			M_Menu_Load_f ();
			break;

		case GAME_SAVE:
			// JHL:BUG-FIX; won't play selecting sound at wrong time
			if (dim_save == true)
				m_entersound = false;
			M_Menu_Save_f ();
			break;

		case GAME_HELP:
			M_Menu_Help_f ();
			break;

		case GAME_CREDITS:
			M_Menu_Credits_f ();
			break;
		}
	}
	if (m_singleplayer_cursor == GAME_LOAD && dim_load == true)
		goto again;
	if (m_singleplayer_cursor == GAME_SAVE && dim_save == true)
		goto again;
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[MAX_OSPATH], temp[MAX_OSPATH], *c;
	vfsfile_t	*f;
	int		version;

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "s%i.sav", i);
//		f = fopen (name, "r");
		f = FS_OpenVFS(name, "rb", FS_GAMEONLY);
		if (!f)
			continue;
//		fscanf (f, "%i\n", &version);
//		fscanf (f, "%79s\n", name);
		VFS_GETS(f, temp, sizeof(temp)); //temp is used as a temporary buffer
		for (c = temp+strlen(temp)-1; c>=temp && (*c == '\n' || *c == '\r'); *c--='\0');
		version = atoi(temp);
		VFS_GETS(f, name, sizeof(name));
		strncpy (m_filenames[i], name, sizeof(m_filenames[i])-1);

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
//		fclose (f);
		VFS_CLOSE(f);
	}
}

void M_Menu_Load_f (void)
{
	dim_load = false;
	if (svs.maxclients != 1 || deathmatch.value || coop.value)
		dim_load = true;
	
	if (dim_load == true)
		return;

	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	dim_save = false;
	if (svs.maxclients != 1 || deathmatch.value || coop.value)
		dim_save = true;
	
	if (!sv.active || cl.intermission)
		dim_save = true;

	if (dim_save == true)
		return;

	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	qpic_t	*p;

	M_Main_Layout (M_M_SINGLE, false);

	p = Draw_CachePic ("gfx/p_load.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 32 + BUTTON_HEIGHT*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*BUTTON_HEIGHT, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;
	qpic_t	*p;

	M_Main_Layout (M_M_SINGLE, false);

	p = Draw_CachePic ("gfx/p_save.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 32 + BUTTON_HEIGHT*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*BUTTON_HEIGHT, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		key_dest = key_game;
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

#define	SLIDER_RANGE	10
void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_DrawCheckbox (int x, int y, int on)
{
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

//=============================================================================
/* SERVER OPTIONS MENU */

#define	SERVER_OPTIONS_ITEMS	5

#define ITEM_SERVER_SPEED		0
#define ITEM_SERVER_FRICTION	1
#define ITEM_SERVER_GRAVITY		2
#define ITEM_SERVER_AUTOAIM		3
#define ITEM_SERVER_CHEATS		4


int server_options_cursor_table[] = {BUTTON_START,
								  BUTTON_START+BUTTON_HEIGHT,
								  BUTTON_START+BUTTON_HEIGHT*3,
								  BUTTON_START+BUTTON_HEIGHT*4,
								  BUTTON_START+BUTTON_HEIGHT*5,
								  BUTTON_START+BUTTON_HEIGHT*6};
								  
int		server_options_cursor;

void M_Menu_Server_f (void)
{
	key_dest = key_menu;
	m_state = m_serveroptions;
	m_entersound = true;
}

void M_AdjustServerSliders (int dir)
{
	extern cvar_t slowmo, sv_friction, sv_gravity, sv_aim, sv_cheats;

	S_LocalSound ("misc/menu3.wav");

	switch (server_options_cursor)
	{
	case ITEM_SERVER_SPEED:	// server speed
		Cvar_SetValue ("slowmo", (slowmo.value + (float)(dir * 0.2f)));
		if (slowmo.value < 0.05)
			Cvar_SetValue ("slowmo", 0.05f);
		if (slowmo.value > 5)
			Cvar_SetValue ("slowmo", 5);
		break;
	case ITEM_SERVER_FRICTION:	// friction
		sv_friction.value += dir * 0.5;
		Cvar_SetValue ("sv_friction", sv_friction.value);
		break;
	case ITEM_SERVER_GRAVITY:	// gravity
		sv_gravity.value += dir * 100;
		Cvar_SetValue ("sv_gravity", sv_gravity.value);
		break;
	case ITEM_SERVER_AUTOAIM:	// autoaim
		if (sv_aim.value == 0)
			sv_aim.value = 2;
		else if (sv_aim.value == 2)
			sv_aim.value = 0;
		else
			sv_aim.value += dir * 0.1;
		Cvar_SetValue ("sv_aim", sv_aim.value);
		break;
	case ITEM_SERVER_CHEATS:	// server cheats
		sv_cheats.value = !sv_cheats.value;
		Cvar_SetValue ("sv_cheats", sv_cheats.value);
		break;
	}
}

void M_Server_Draw (void)
{
	extern cvar_t slowmo, sv_friction, sv_gravity, sv_aim, sv_cheats;
	float	r;
	int		y, pos, i;

	pos = 16 + ((vid.height/2)-120);
	M_CenterprintWhite(pos, "Server Options Menu");

	y = server_options_cursor_table[ITEM_SERVER_SPEED];
	M_Print (16, y, "           Game speed");
	r = slowmo.value / 5;
	M_DrawSlider (220, y, r);

	y = server_options_cursor_table[ITEM_SERVER_FRICTION];
	M_Print (16, y, "      Player Friction");
	r = sv_friction.value / 12;
	M_DrawSlider (220, y, r);

	y = server_options_cursor_table[ITEM_SERVER_GRAVITY];
	M_Print (16, y, "              Gravity");
	r = sv_gravity.value / 1500;
	M_DrawSlider (220, y, r);

	y = server_options_cursor_table[ITEM_SERVER_AUTOAIM];
	M_Print (16, y, "             Auto Aim");
	r = sv_aim.value / 2;
	M_DrawSlider (220, y, r);

	y = server_options_cursor_table[ITEM_SERVER_CHEATS];
	M_Print (16, y, "               Cheats");
	M_DrawCheckbox (220, y, sv_cheats.value);
	y += BUTTON_HEIGHT*4;
	M_Print (16, y, "    Changes to the cheats option");
	y += BUTTON_HEIGHT;
	M_Print (16, y, "    will not take effect until");
	y += BUTTON_HEIGHT;
	M_Print (16, y, "    next map, or a map restart.");

	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex) >= BUTTON_MENU_X)
	{
		for (i=5; i >= 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > server_options_cursor_table[i-1])
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < server_options_cursor_table[i])
				{
					server_options_cursor = i-1;
					break;
				}
			}
		}
	}
// cursor
	M_DrawCharacter (200, server_options_cursor_table[server_options_cursor], 12+((int)(realtime*4)&1));
}


void M_Server_Key (int k)
{
	//JHL:HACK; two diffirent 'customize' menus
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		switch (server_options_cursor)
		{
		default:
			M_AdjustServerSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		server_options_cursor--;
		if (server_options_cursor < 0)
			server_options_cursor = SERVER_OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		server_options_cursor++;
		if (server_options_cursor >= SERVER_OPTIONS_ITEMS)
			server_options_cursor = 0;
		break;

	case K_MWHEELDOWN:
	case K_LEFTARROW:
		M_AdjustServerSliders (-1);
		break;

	case K_MWHEELUP:
	case K_RIGHTARROW:
		M_AdjustServerSliders (1);
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	4


void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
}


void M_MultiPlayer_Draw (void)
{
	qpic_t	*p;
	char	*names[] =
	{
		"Join game",
		"Create game",
		"Player setup",
		"Server options",
		0
	};
	int i;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_DrawTransPic ( (320-p->width)/2, 4, p); //JHL:HACK; transparent menu banners

	M_PrintWhite (BUTTON_MENU_X, BUTTON_START, names[0]);
	M_PrintWhite (BUTTON_MENU_X, BUTTON_START+BUTTON_HEIGHT, names[1]);
	M_PrintWhite (BUTTON_MENU_X, BUTTON_START+(BUTTON_HEIGHT*2), names[2]);
	M_PrintWhite (BUTTON_MENU_X, BUTTON_START+(BUTTON_HEIGHT*3), names[3]);

	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex) >= BUTTON_MENU_X)
	{
		for (i=3; i >= 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > BUTTON_START+(10*(i-1)))
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < BUTTON_START+(10*i))
				{
					m_multiplayer_cursor = i;
					break;
				}
			}
		}
	}

//cursor
	M_DrawCharacter (BUTTON_MENU_X - 10, BUTTON_START+(m_multiplayer_cursor*BUTTON_HEIGHT), 12+((int)(realtime*4)&1));

	if (serialAvailable || ipxAvailable || tcpipAvailable)
		return;

	//JHL:NOTE; WTF is this?
	M_PrintWhite ((320/2) - ((27*8)/2), 148, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Menu_Net_f ();
			break;

		case 1:
			if (serialAvailable || ipxAvailable || tcpipAvailable)
				M_Menu_Net_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;

		case 3:
			M_Menu_Server_f ();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

int		setup_cursor = 4;
int		setup_cursor_table[] = {40, 56, 80, 104, 140};

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	5

void M_Menu_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	Q_strcpy(setup_myname, cl_name.string);
	Q_strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
	setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
}


void M_Setup_Draw (void)
{
	qpic_t	*p;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_PrintWhite (168, 40, setup_hostname);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_PrintWhite (168, 56, setup_myname);

	M_Print (64, 80, "Shirt color");
	M_Print (64, 104, "Pants color");

	M_DrawTextBox (64, 140-8, 14, 1);
	M_Print (72, 140, "Accept Changes");

	M_DrawTextBox (160, 64, 7, 7);
	p = Draw_CachePic ("gfx/menuplyr.lmp");
	M_BuildTranslationTable(setup_top*16, setup_bottom*16);
	M_DrawTransPicTranslate (172, 72, p);

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 0)
		M_DrawCharacter (168 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}

void M_Setup_Key (int k)
{
	int			l;

	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top - 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom - 1;
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top + 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom + 1;
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3)
			goto forward;

		// setup_cursor == 4 (OK)
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (Q_strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
			Cbuf_AddText( va ("color %i %i\n", setup_top, setup_bottom) );
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;

	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 1)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	if (setup_top > 13)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 13;
	if (setup_bottom > 13)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 13;
}

//=============================================================================
/* NET MENU */
/* NET MENU */

int	m_net_cursor;
int m_net_items;
int m_net_saveHeight;

char *net_helpMessage [] =
{
  "",
  "Two computers connected through two modems.",

  "",
  "Two computers connected by a null-modem cable.",

  "Novell network LANs or Win 95/98/ME DOS-box.",
  "(LAN=Local Area Network)",

  "Commonly used to play over the Internet, but",
  "also used on a Local Area Network."
};

void M_Menu_Net_f (void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	qpic_t	*p;
	char	*names[] =
	{
		"Modem",
		"Direct connect",
		"IPX",
		"TCP/IP",
		0
	};
	int i;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	//JHL:BUG-FIX; removed "multiprotocol" test from here
	if (serialAvailable)
		M_PrintWhite (BUTTON_MENU_X, BUTTON_START, names[0]);
	else
		M_Print (BUTTON_MENU_X, BUTTON_START, names[0]);

	if (serialAvailable)
		M_PrintWhite (BUTTON_MENU_X, BUTTON_START+BUTTON_HEIGHT, names[1]);
	else
		M_Print (BUTTON_MENU_X, BUTTON_START+BUTTON_HEIGHT, names[1]);

	if (ipxAvailable)
		M_PrintWhite (BUTTON_MENU_X, BUTTON_START+(BUTTON_HEIGHT*2), names[2]);
	else
		M_Print (BUTTON_MENU_X, BUTTON_START+(BUTTON_HEIGHT*2), names[2]);

	if (tcpipAvailable)
		M_PrintWhite (BUTTON_MENU_X, BUTTON_START+(BUTTON_HEIGHT*3), names[3]);
	else
		M_Print (BUTTON_MENU_X, BUTTON_START+(BUTTON_HEIGHT*3), names[3]);

	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex) >= BUTTON_MENU_X)
	{
		for (i=3; i >= 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > BUTTON_START+(10*(i-1)))
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < BUTTON_START+(10*i))
				{
					m_net_cursor = i;
					break;
				}
			}
		}
	}

// cursor
	M_DrawCharacter (BUTTON_MENU_X - 10, BUTTON_START+(m_net_cursor*BUTTON_HEIGHT), 12+((int)(realtime*4)&1));

// help message
	M_CenterprintWhite (vid.height-16, net_helpMessage[m_net_cursor*2]);
	M_CenterprintWhite (vid.height-8, net_helpMessage[m_net_cursor*2+1]);
}

void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 0:
			M_Menu_SerialConfig_f ();
			break;

		case 1:
			M_Menu_SerialConfig_f ();
			break;

		case 2:
			M_Menu_LanConfig_f ();
			break;

		case 3:
			M_Menu_LanConfig_f ();
			break;

		case 4:
// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0 && !serialAvailable)
		goto again;
	if (m_net_cursor == 1 && !serialAvailable)
		goto again;
	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;
}

//=============================================================================
/* OPTIONS MENU */

#ifdef _WIN32
#define	OPTIONS_ITEMS	13
#else
#define	OPTIONS_ITEMS	12
#endif

#define	ITEM_KEYS		0
#define	ITEM_CONSOLE	1
#define	ITEM_RESET		2
#define	ITEM_SPEED		3
#define	ITEM_CD_VOLUME	4
#define	ITEM_VOLUME		5
#define ITEM_CROSSHAIR	6
#define ITEM_DYNAMIC	7
#define	ITEM_AUTORUN	8
#define	ITEM_REVERSE	9
#define	ITEM_SPRING		10 
#define	ITEM_STRAFE		11
#define	ITEM_WINMOUSE	12

int options_cursor_table[] =    {BUTTON_START,
								 BUTTON_START+BUTTON_HEIGHT,
								 BUTTON_START+BUTTON_HEIGHT*2,
								 BUTTON_START+BUTTON_HEIGHT*4,
							     BUTTON_START+BUTTON_HEIGHT*5,
							     BUTTON_START+BUTTON_HEIGHT*6,
								 BUTTON_START+BUTTON_HEIGHT*7,
								 BUTTON_START+BUTTON_HEIGHT*8,
								 BUTTON_START+BUTTON_HEIGHT*9,
								 BUTTON_START+BUTTON_HEIGHT*10,
								 BUTTON_START+BUTTON_HEIGHT*11,
								 BUTTON_START+BUTTON_HEIGHT*12,
								 BUTTON_START+BUTTON_HEIGHT*13,
								 BUTTON_START+BUTTON_HEIGHT*14};

int		options_cursor;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;

#ifdef _WIN32
	if ((options_cursor == ITEM_WINMOUSE) && (modestate != MS_WINDOWED))
	{
		options_cursor = 0;
	}
#endif
}

void M_AdjustSliders (int dir)
{
	S_LocalSound ("misc/menu3.wav");

	switch (options_cursor)
	{
	case ITEM_SPEED:	// mouse speed
		sensitivity.value += dir * 0.5;
		if (sensitivity.value < 1)
			sensitivity.value = 1;
		if (sensitivity.value > 11)
			sensitivity.value = 11;
		Cvar_SetValue ("sensitivity", sensitivity.value);
		break;
	case ITEM_CD_VOLUME:// music volume
#ifdef _WIN32
		bgmvolume.value += dir * 1.0;
#else
		bgmvolume.value += dir * 0.1;
#endif
		if (bgmvolume.value < 0)
			bgmvolume.value = 0;
		if (bgmvolume.value > 1)
			bgmvolume.value = 1;
		Cvar_SetValue ("bgmvolume", bgmvolume.value);
		break;
	case ITEM_VOLUME:	// sfx volume
		volume.value += dir * 0.1;
		if (volume.value < 0)
			volume.value = 0;
		if (volume.value > 1)
			volume.value = 1;
		Cvar_SetValue ("volume", volume.value);
		break;

	case ITEM_CROSSHAIR:// crosshair
		Cvar_SetValue ("crosshair", !crosshair.value);
		break;

	case ITEM_DYNAMIC:// dynamic crosshair
		Cvar_SetValue ("crosshair_static", !crosshair_static.value);
		break;

	case ITEM_AUTORUN:	// allways run
		if (cl_forwardspeed.value > 200)
		{
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
		}
		break;

	case ITEM_REVERSE:	// invert mouse
		Cvar_SetValue ("m_pitch", -m_pitch.value);
		break;

	case ITEM_SPRING:	// lookspring
		Cvar_SetValue ("lookspring", !lookspring.value);
		break;

	case ITEM_STRAFE:	// lookstrafe
		Cvar_SetValue ("lookstrafe", !lookstrafe.value);
		break;

#ifdef _WIN32
	case ITEM_WINMOUSE:	// _windowed_mouse
		Cvar_SetValue ("_windowed_mouse", !_windowed_mouse.value);
		break;
#endif
	}
}

void M_Options_Draw (void)
{
	float	r;
	int		y, i;
	qpic_t	*p;

	M_Main_Layout (M_M_OPTION, false);

	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	y = options_cursor_table[ITEM_KEYS];
	M_PrintWhite (16, y, "    Customize controls");
	y = options_cursor_table[ITEM_CONSOLE];
	M_PrintWhite (16, y, "         Go to console");
	y = options_cursor_table[ITEM_RESET];
	M_PrintWhite (16, y, "     Reset to defaults");

	y = options_cursor_table[ITEM_SPEED];
	M_Print (16, y, "           Mouse Speed");
	r = (sensitivity.value - 1)/10;
	M_DrawSlider (220, y, r);

	y = options_cursor_table[ITEM_CD_VOLUME];
	M_Print (16, y, "       CD Music Volume");
	r = bgmvolume.value;
	M_DrawSlider (220, y, r);

	y = options_cursor_table[ITEM_VOLUME];
	M_Print (16, y, "          Sound Volume");
	r = volume.value;
	M_DrawSlider (220, y, r);

	y = options_cursor_table[ITEM_CROSSHAIR];
	M_Print (16, y,  "             Crosshair");
	M_DrawCheckbox (220, y, crosshair.value);

	y = options_cursor_table[ITEM_DYNAMIC];
	M_Print (16, y,  "     Dynamic Crosshair");
	M_DrawCheckbox (220, y, !crosshair_static.value);

	y = options_cursor_table[ITEM_AUTORUN];
	M_Print (16, y,  "            Always Run");
	M_DrawCheckbox (220, y, cl_forwardspeed.value > 200);

	y = options_cursor_table[ITEM_REVERSE];
	M_Print (16, y, "          Invert Mouse");
	M_DrawCheckbox (220, y, m_pitch.value < 0);

	y = options_cursor_table[ITEM_SPRING];
	M_Print (16, y, "            Lookspring");
	M_DrawCheckbox (220, y, lookspring.value);

	y = options_cursor_table[ITEM_STRAFE];
	M_Print (16, y, "            Lookstrafe");
	M_DrawCheckbox (220, y, lookstrafe.value);

#ifdef _WIN32
	if (modestate == MS_WINDOWED)
	{
		y = options_cursor_table[ITEM_WINMOUSE];
		M_Print (16, y, "             Use Mouse");
		M_DrawCheckbox (220, y, _windowed_mouse.value);
	}
#endif

	// Entar: a little odd implementation, but it works
	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex) >= BUTTON_MENU_X)
	{
		if (modestate == MS_WINDOWED)
			i = 13;
		else
			i = 12;

		for (; i >= 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > options_cursor_table[i-1])
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < options_cursor_table[i])
				{
					options_cursor = i-1;
					break;
				}
			}
		}
	}

// cursor
	M_DrawCharacter (200, options_cursor_table[options_cursor], 12+((int)(realtime*4)&1));
}

void M_Options_Key (int k)
{
	//JHL:HACK; two diffirent 'customize' menus
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case ITEM_KEYS:
					M_Menu_Keys_f ();
			break;
		case ITEM_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case ITEM_RESET:
			//JHL:ADD; separate defaults for video and input
			Cbuf_AddText ("exec default_input.cfg\n");
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
		{
#ifdef _WIN32
			if (modestate == MS_WINDOWED)
				options_cursor = OPTIONS_ITEMS-1;
			else
				options_cursor = OPTIONS_ITEMS-2;
#else
			options_cursor = OPTIONS_ITEMS-1;
#endif
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;
		break;

	case K_MWHEELDOWN:
	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;
	
	case K_MWHEELUP:
	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}

#ifdef _WIN32
	if ((options_cursor == ITEM_WINMOUSE) && (modestate != MS_WINDOWED))
	{
		if (k == K_UPARROW)
			options_cursor = OPTIONS_ITEMS - 1;
		else
			options_cursor = 0;
	}
#endif
}

//=============================================================================
/* GRAPHICS OPTIONS MENU */

#define GFX_OPTIONS_ITEMS	11

#define ITEM_RTL_WORLD		0
#define ITEM_RTL_DLIGHT		1
#define ITEM_RTL_DRAW_WORLD	2
#define ITEM_RTL_DRAW_MODEL	3
#define ITEM_REFLECTIVE		4
#define ITEM_BLOOMS			5
#define ITEM_BLOOM_INT		6
#define ITEM_BLOOM_SEN		7
#define ITEM_DIRLIGHTING	8
#define ITEM_WATERALPHA		9
#define ITEM_CORONAS		10

int gfx_options_cursor_table[] = {BUTTON_START,
								  BUTTON_START+BUTTON_HEIGHT,
								  BUTTON_START+BUTTON_HEIGHT*2,
								  BUTTON_START+BUTTON_HEIGHT*3,
							      BUTTON_START+BUTTON_HEIGHT*4,
							      BUTTON_START+BUTTON_HEIGHT*5,
								  BUTTON_START+BUTTON_HEIGHT*6,
								  BUTTON_START+BUTTON_HEIGHT*7,
								  BUTTON_START+BUTTON_HEIGHT*8,
								  BUTTON_START+BUTTON_HEIGHT*9,
								  BUTTON_START+BUTTON_HEIGHT*10};

int		gfx_options_cursor;

void M_Menu_Graphics_f (void)
{
	key_dest = key_menu;
	m_state = m_gfx_options;
	m_entersound = true;
}

void M_AdjustGraphicsSliders (int dir)
{
	extern	cvar_t r_bloom, r_bloom_intensity, r_bloom_darken, r_coronas;

	S_LocalSound ("misc/menu3.wav");
	
	switch (gfx_options_cursor)
	{
	case ITEM_RTL_WORLD:
		Cvar_SetValue ("r_shadow_realtime_world", !r_shadow_realtime_world.value);
		break;
	case ITEM_RTL_DLIGHT:
		Cvar_SetValue ("r_shadow_realtime_dlight", !r_shadow_realtime_dlight.value);
		break;
	case ITEM_RTL_DRAW_WORLD:
		Cvar_SetValue ("r_shadow_realtime_draw_world", !r_shadow_realtime_draw_world.value);
		break;
	case ITEM_RTL_DRAW_MODEL:
		Cvar_SetValue ("r_shadow_realtime_draw_models", !r_shadow_realtime_draw_models.value);
		break;
	case ITEM_REFLECTIVE:	// reflective water
		Cvar_SetValue ("r_waterrefl", !r_waterrefl.value);
		break;
	case ITEM_BLOOMS:	// light blooms
		if (r_bloom.value == 0)
			r_bloom.value = 1;
		else if (r_bloom.value == 1)
			r_bloom.value = 0;
		else
			r_bloom.value = 0;
		Cvar_SetValue ("r_bloom", r_bloom.value);
		break;
	case ITEM_BLOOM_INT:	// light bloom intensity
		r_bloom_intensity.value += dir * 0.5;
		if (r_bloom_intensity.value < 0.1)
			r_bloom_intensity.value = 0.1f;
		if (r_bloom_intensity.value > 5)
			r_bloom_intensity.value = 5;
		Cvar_SetValue ("r_bloom_intensity", r_bloom_intensity.value);
		break;
	case ITEM_BLOOM_SEN:	// light bloom sensitivity
		r_bloom_darken.value -= dir*2; // it's backwards
		if (r_bloom_darken.value < 0.5)
			r_bloom_darken.value = 0.5f;
		if (r_bloom_darken.value > 40)
			r_bloom_darken.value = 40;
		Cvar_SetValue ("r_bloom_darken", r_bloom_darken.value);
		break;
	case ITEM_DIRLIGHTING:	// directional polygon lighting
		Cvar_SetValue ("r_dirlighting", !r_dirlighting.value);
		break;
	case ITEM_WATERALPHA:
		r_wateralpha.value += dir * 0.1;
		if (r_wateralpha.value > 1)
			r_wateralpha.value = 1;
		else if (r_wateralpha.value < 0)
			r_wateralpha.value = 0;
		Cvar_SetValueQuick (&r_wateralpha, r_wateralpha.value);
		break;
	case ITEM_CORONAS:
		r_coronas.value += dir * 0.2f;
		if (r_coronas.value > 5)
			r_coronas.value = 5;
		else if (r_coronas.value < 0)
			r_coronas.value = 0;
		Cvar_SetValueQuick (&r_coronas, r_coronas.value);
		break;
	}
}

void M_Graphics_Draw (void)
{
	extern	cvar_t	r_bloom, r_bloom_intensity, r_bloom_darken, r_coronas;
	float r;
	int y, i;
	char	value[8];
	
	M_Main_Layout (M_M_VIDEO, false);

	y = gfx_options_cursor_table[ITEM_RTL_WORLD];
	M_Print (16, y, "         RT World Lights");
	M_DrawCheckbox (236, y, r_shadow_realtime_world.value);

	y = gfx_options_cursor_table[ITEM_RTL_DLIGHT];
	M_Print (16, y, "              RT DLights");
	M_DrawCheckbox (236, y, r_shadow_realtime_dlight.value);

	y = gfx_options_cursor_table[ITEM_RTL_DRAW_WORLD];
	M_Print (16, y, "           RT World Draw");
	M_DrawCheckbox (236, y, r_shadow_realtime_draw_world.value);

	y = gfx_options_cursor_table[ITEM_RTL_DRAW_MODEL];
	M_Print (16, y, "          RT Models Draw");
	M_DrawCheckbox (236, y, r_shadow_realtime_draw_models.value);

	y = gfx_options_cursor_table[ITEM_REFLECTIVE];
	M_Print (16, y, "        Reflective water");
	M_DrawCheckbox (236, y, r_waterrefl.value);

	y = gfx_options_cursor_table[ITEM_BLOOMS];
	M_Print (16, y, "            Light Blooms");
	M_DrawCheckbox (236, y, r_bloom.value);

	y = gfx_options_cursor_table[ITEM_BLOOM_INT];
	M_Print (16, y, "  Light Blooms Intensity");
	r = (r_bloom_intensity.value / 5);
	M_DrawSlider (236, y, r);
	sprintf (value, "%.2f", r_bloom_intensity.value);
	M_Print (328, y, value);

	y = gfx_options_cursor_table[ITEM_BLOOM_SEN];
	M_Print (16, y, "Light Blooms Sensitivity");
	r = (40 - r_bloom_darken.value)/40;
	M_DrawSlider (236, y, r);

	y = gfx_options_cursor_table[ITEM_DIRLIGHTING];
	M_Print (16, y, "    Directional Lighting");
	M_DrawCheckbox (236, y, r_dirlighting.value);

	y = gfx_options_cursor_table[ITEM_WATERALPHA];
	M_Print (16, y, "      Water transparency");
	r = (r_wateralpha.value)/1;
	M_DrawSlider (236, y, r);

	y = gfx_options_cursor_table[ITEM_CORONAS];
	M_Print (16, y, "        Corona Intensity");
	r = (r_coronas.value)/5;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.2f", r_coronas.value);
	M_Print (328, y, value);

	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex - ((vid.width - 320)>>1)) >= 16)
	{
		for (i=GFX_OPTIONS_ITEMS; i > 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > gfx_options_cursor_table[i-1])
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < gfx_options_cursor_table[i])
				{
					gfx_options_cursor = i-1;
					break;
				}
			}
		}
	}

// cursor
	M_DrawCharacter (216, gfx_options_cursor_table[gfx_options_cursor], 12+((int)(realtime*4)&1));
}

void M_Graphics_Key (int k)
{
	//JHL:HACK; two different 'customize' menus
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Video_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		M_AdjustGraphicsSliders (1);
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		gfx_options_cursor--;
		if (gfx_options_cursor < 0)
			gfx_options_cursor = GFX_OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		gfx_options_cursor++;
		if (gfx_options_cursor >= GFX_OPTIONS_ITEMS)
			gfx_options_cursor = 0;
		break;

	case K_MWHEELDOWN:
	case K_LEFTARROW:
		M_AdjustGraphicsSliders (-1);
		break;

	case K_MWHEELUP:
	case K_RIGHTARROW:
		M_AdjustGraphicsSliders (1);
		break;
	}
}

//=============================================================================
/* VIDEO OPTIONS MENU */

#define	VID_OPTIONS_ITEMS	17

#define ITEM_VIDEO_MODES	0
#define ITEM_VIDEO_RESET	1
#define ITEM_GRAPHICS_MENU	2
#define ITEM_SCR_SIZE		3
#define ITEM_GAMMA			4
#define ITEM_CONTRAST		5
#define ITEM_SHADOWS		6
#define ITEM_SSHOT_JPEG		7
#define ITEM_SSHOT_GAMMA	8
#define ITEM_CAUSTICS		9
#define ITEM_DETAILTEX		10
#define ITEM_GUNMOVE		11
#define ITEM_HUDSTYLE		12
#define ITEM_SHOWFPS		13
#define ITEM_HUDR			14
#define ITEM_HUDG			15
#define ITEM_HUDB			16

int vid_options_cursor_table[] = {BUTTON_START,
								  BUTTON_START+BUTTON_HEIGHT,
								  BUTTON_START+BUTTON_HEIGHT*2,
								  BUTTON_START+BUTTON_HEIGHT*4,
								  BUTTON_START+BUTTON_HEIGHT*5,
							      BUTTON_START+BUTTON_HEIGHT*6,
							      BUTTON_START+BUTTON_HEIGHT*7,
								  BUTTON_START+BUTTON_HEIGHT*9,
								  BUTTON_START+BUTTON_HEIGHT*10,
								  BUTTON_START+BUTTON_HEIGHT*12,
								  BUTTON_START+BUTTON_HEIGHT*13,
								  BUTTON_START+BUTTON_HEIGHT*14,
								  BUTTON_START+BUTTON_HEIGHT*15,
								  BUTTON_START+BUTTON_HEIGHT*16,
								  BUTTON_START+BUTTON_HEIGHT*17,
								  BUTTON_START+BUTTON_HEIGHT*18,
								  BUTTON_START+BUTTON_HEIGHT*19};
								  
int		vid_options_cursor;

void M_Menu_Video_f (void)
{
	key_dest = key_menu;
	m_state = m_vid_options;
	m_entersound = true;
}

extern cvar_t show_fps, hud_r, hud_g, hud_b, hud_a, scr_screenshot_jpeg, scr_screenshot_gammaboost;

void M_AdjustVideoSliders (int dir)
{
	S_LocalSound ("misc/menu3.wav");

	switch (vid_options_cursor)
	{
	case ITEM_SCR_SIZE:	// screen size
		scr_viewsize.value += dir * 10;
		if (scr_viewsize.value < 30)
			scr_viewsize.value = 30;
		if (scr_viewsize.value > 120)
			scr_viewsize.value = 120;
		Cvar_SetValue ("viewsize", scr_viewsize.value);
		break;
	case ITEM_GAMMA:	// gamma
		if (dir == 1 && v_gamma.value < 3)
			v_gamma.value += 0.05f;
		else if (dir == -1 && v_gamma.value > 0)
			v_gamma.value -= 0.05f;
		Cvar_SetValue ("v_gamma", v_gamma.value);
		break;
	case ITEM_CONTRAST:	// contrast
		if (dir == 1 && v_contrast.value < 2)
			v_contrast.value += 0.05f;
		else if (dir == -1 && v_contrast.value > 0)
			v_contrast.value -= 0.05f;

		Cvar_SetValue ("v_contrast", v_contrast.value);
		break;
	case ITEM_SHADOWS:	// character shadows
		r_shadows.value += dir * 0.1;
		if (r_shadows.value > 1)
			r_shadows.value = 1;
		else if (r_shadows.value < 0)
			r_shadows.value = 0;
		Cvar_SetValue ("r_shadows", r_shadows.value);
		break;
	case ITEM_SSHOT_JPEG: // jpeg screenshots on/off
		Cvar_SetValue("scr_screenshot_jpeg", !scr_screenshot_jpeg.value);
		break;
	case ITEM_SSHOT_GAMMA: // screenshot gamma boost
		scr_screenshot_gammaboost.value += dir * 0.1;
		if (scr_screenshot_gammaboost.value > 3)
			scr_screenshot_gammaboost.value = 3;
		else if (scr_screenshot_gammaboost.value < 0)
			scr_screenshot_gammaboost.value = 0;
		Cvar_SetValue ("scr_screenshot_gammaboost", scr_screenshot_gammaboost.value);
		break;
	case ITEM_CAUSTICS:	// underwater caustics
		Cvar_SetValue ("gl_caustics", !gl_caustics.value);
		break;
	case ITEM_DETAILTEX:// detail textures
		Cvar_SetValue ("gl_detail", !gl_detail.value);
		break;
	case ITEM_GUNMOVE:// idle gun movement
		if (v_gunmove.value)
			v_gunmove.value = 0;
		else if (!v_gunmove.value)
			v_gunmove.value = 2;
		Cvar_SetValue ("v_gunmove", v_gunmove.value);
		break;
	case ITEM_HUDSTYLE:	// hudstyles
		if (hud.value != 3)
			Cvar_SetValue ("hud", 3);
		else
			Cvar_SetValue ("hud", 4);
		break;
	case ITEM_SHOWFPS:	// show fps
		Cvar_SetValue ("show_fps", !show_fps.value);
		break;
	case ITEM_HUDR:
		Cvar_SetValue("hud_red", hud_r.value + (dir * 15));
		if (hud_r.value > 255)
			Cvar_SetValue("hud_red", 255);
		else if (hud_r.value < 0)
			Cvar_SetValue("hud_red", 0);
		break;
	case ITEM_HUDG:
		Cvar_SetValue("hud_green", hud_g.value + (dir * 15));
		if (hud_g.value > 255)
			Cvar_SetValue("hud_green", 255);
		else if (hud_g.value < 0)
			Cvar_SetValue("hud_green", 0);
		break;
	case ITEM_HUDB:
		Cvar_SetValue("hud_blue", hud_b.value + (dir * 15));
		if (hud_b.value > 255)
			Cvar_SetValue("hud_blue", 255);
		else if (hud_b.value < 0)
			Cvar_SetValue("hud_blue", 0);
		break;
	}
	//Entar : clamp gamma values (shouldn't have to do this)
	if (v_gamma.value > 3)
		Cvar_SetValue("v_gamma", 3);
	else if (v_gamma.value < 0)
		Cvar_SetValue("v_gamma", 0);
}


void M_Video_Draw (void)
{
	float	r;
	int		y, i;
	qpic_t	*p;
	vfsfile_t *f;
	char	value[8];

	M_Main_Layout (M_M_VIDEO, false);

	f = FS_OpenVFS("gfx/qmb_menu_video.lmp", "rb", FS_GAME);
	if (f)
	{
		VFS_CLOSE(f);
		p = Draw_CachePic ("gfx/qmb_menu_video.lmp");
	}
	else
		p = Draw_CachePic("gfx/vidmodes.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	//check if available
	if (vid_menudrawfn)
	{
		y = vid_options_cursor_table[ITEM_VIDEO_MODES];
		M_PrintWhite (16, y, "             Video Modes");
	}

	y = vid_options_cursor_table[ITEM_VIDEO_RESET];
	M_PrintWhite (16, y, "       Reset to defaults");

	y = vid_options_cursor_table[ITEM_GRAPHICS_MENU];
	M_PrintWhite (16, y, "           Graphics menu");
	
	y = vid_options_cursor_table[ITEM_SCR_SIZE];
	M_Print (16, y, "             Screen size");
	r = (scr_viewsize.value - 30) / (120 - 30);
	M_DrawSlider (236, y, r);
	sprintf (value, "%.0f", scr_viewsize.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_GAMMA];
	M_Print (16, y, "              Brightness");
	r = v_gamma.value / 3;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.2f", v_gamma.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_CONTRAST];
	M_Print (16, y, "                Contrast");
	r = v_contrast.value / 2;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.2f", v_contrast.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_SHADOWS];
	M_Print (16, y, "                 Shadows");
	r = r_shadows.value/1;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.2f", r_shadows.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_SSHOT_JPEG];
	M_Print (16, y, "        JPEG Screenshots");
	M_DrawCheckbox (236, y, scr_screenshot_jpeg.value);

	y = vid_options_cursor_table[ITEM_SSHOT_GAMMA];
	M_Print (16, y, "        Screenshot Gamma");
	r = (scr_screenshot_gammaboost.value / 3);
	M_DrawSlider (236, y, r);
	sprintf (value, "%.2f", scr_screenshot_gammaboost.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_CAUSTICS];
	M_Print (16, y, "     Underwater caustics");
	M_DrawCheckbox (236, y, gl_caustics.value);

	y = vid_options_cursor_table[ITEM_DETAILTEX];
	M_Print (16, y, "        Detail texturing");
	M_DrawCheckbox (236, y, gl_detail.value);

	y = vid_options_cursor_table[ITEM_GUNMOVE];
	M_Print (16, y, "       Idle gun movement");
	M_DrawCheckbox (236, y, v_gunmove.value);

	y = vid_options_cursor_table[ITEM_HUDSTYLE];
	M_Print (16, y, "     Alternate hud style");
	if (hud.value < 4)
		M_Print (236, y, "off");
	else
		M_Print (236, y, "on");

	y = vid_options_cursor_table[ITEM_SHOWFPS];
	M_Print (16, y, "                Show fps");
	M_DrawCheckbox (236, y, show_fps.value);

	y = vid_options_cursor_table[ITEM_HUDR];
	M_Print (16, y, "                 HUD red");
	r = (hud_r.value)/255;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.0f", hud_r.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_HUDG];
	M_Print (16, y, "               HUD green");
	r = (hud_g.value)/255;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.0f", hud_g.value);
	M_Print (328, y, value);

	y = vid_options_cursor_table[ITEM_HUDB];
	M_Print (16, y, "                HUD blue");
	r = (hud_b.value)/255;
	M_DrawSlider (236, y, r);
	sprintf (value, "%.0f", hud_b.value);
	M_Print (328, y, value);

	if ((current_pos.x != last_pos.x || current_pos.y != last_pos.y) && (current_pos.x - omousex - ((vid.width - 320)>>1)) >= 18)
	{
		for (i=20; i >= 0; i--)
		{
			if (current_pos.y - (omousey+((vid.height/2)-120)) > vid_options_cursor_table[i-1])
			{
				if (current_pos.y - (omousey+((vid.height/2)-120)) < vid_options_cursor_table[i])
				{
					vid_options_cursor = i-1;
					break;
				}
			}
		}
	}

// cursor
	M_DrawCharacter (216, vid_options_cursor_table[vid_options_cursor], 12+((int)(realtime*4)&1));
}


void M_Video_Key (int k)
{
	//JHL:HACK; two different 'customize' menus
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_MOUSE1:
	case K_ENTER:
		m_entersound = true;
		switch (vid_options_cursor)
		{
		case ITEM_VIDEO_RESET:
			//JHL:ADD; separate defaults for video and input
			Cbuf_AddText ("exec default_video.cfg\n");
			break;
		case ITEM_VIDEO_MODES:
			M_Menu_VideoModes_f();
			return;
		case ITEM_GRAPHICS_MENU:
			M_Menu_Graphics_f();
			return;
		default:
			M_AdjustVideoSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		vid_options_cursor--;
		if (vid_options_cursor < 0)
			vid_options_cursor = VID_OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		vid_options_cursor++;
		if (vid_options_cursor >= VID_OPTIONS_ITEMS)
			vid_options_cursor = 0;
		break;

	case K_MWHEELDOWN:
	case K_LEFTARROW:
		M_AdjustVideoSliders (-1);
		break;

	case K_MWHEELUP:
	case K_RIGHTARROW:
		M_AdjustVideoSliders (1);
		break;
	}

	if (vid_options_cursor == ITEM_VIDEO_MODES && vid_menudrawfn == NULL)
	{
		if (k == K_UPARROW)
			vid_options_cursor = ITEM_VIDEO_MODES - 1;
		else
			vid_options_cursor = 0;
	}
}

//=============================================================================
/* VIDEO MODES MENU */

void M_Menu_VideoModes_f (void)
{
	key_dest = key_menu;
	m_state = m_video;
	m_entersound = true;
}

void M_VideoModes_Draw (void)
{
	M_Main_Layout (M_M_VIDEO, false);
	(*vid_menudrawfn) ();
}


void M_VideoModes_Key (int key)
{
	VID_MenuKey (key);
}

//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 10.0", 		"next weapon"},
{"impulse 12.0", 		"prev weapon"},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
//{"+mlook", 			"mouse look"}, // command removed
{"+klook", 			"keyboard look"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

int		keys_cursor;
int		bind_grab;

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	char	*name;
	int		x, y;
	qpic_t	*p;
	
	M_Main_Layout (M_M_OPTION, false);

	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	if (bind_grab)
		M_CenterprintWhite (vid.height-8, "Press a key or button for this action");
	else
		M_CenterprintWhite (vid.height-8, "Enter to change, backspace to clear");

// search for known bindings
	for (i=0 ; i<NUMCOMMANDS ; i++)
	{
		y = 48 + 8*i;

		M_Print (16, y, bindnames[i][1]);

		l = strlen (bindnames[i][0]);

		M_FindKeysForCommand (bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 48 + keys_cursor*8, '=');
	else
		M_DrawCharacter (130, 48 + keys_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];

	if (bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("misc/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	5

#define HELP_MOVEMENT	0
#define HELP_SWIMMING	1
#define HELP_HAZARDS	2
#define HELP_BUTTONS	3
#define HELP_SECRETS	4

void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}

void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/help%i.lmp", help_page)) );
}

void M_Help_Key (int key)
{
	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* CREDITS MENU */

int		credits_page;
#define	NUM_CREDITS_PAGES	2

#define CREDITS_QMB	0
#define CREDITS_ID	1

void M_Menu_Credits_f (void)
{
	key_dest = key_menu;
	m_state = m_credits;
	m_entersound = true;
	credits_page = 0;
}

void M_Credits_Draw (void)
{
	char	onscreen_help[64];
	int		y;

	M_Main_Layout (M_M_SINGLE, false);
	M_DrawTextBox (-4, -4, 39, 31); // originally 29 lines

	//do the help message
	sprintf (onscreen_help, "Page %i/%i", credits_page+1, NUM_CREDITS_PAGES);
	M_CenterprintWhite (vid.height-16, onscreen_help);
	M_CenterprintWhite (vid.height-8, "Use the arrow keys to scroll the pages.");

	if (credits_page == CREDITS_QMB)
	{
		// Chronicle credits
		if (gamemode == GAME_CHRONICLE)
		{
			y = 16 + ((vid.height/2)-120);
			M_CenterprintWhite	(y, "Chronicle");
			y += 8;
			M_CenterprintWhite	(y, "A Deathmatch game");
			y = (int)(host_time * 10)%6;
			//M_DrawTransPic (160, 26, Draw_CachePic( va("gfx/menudot%i.lmp", y+1 ) ) );
			M_DrawTransPic (160, 32, Draw_CachePic( va("gfx/menudot%i.lmp", y+1 ) ) );

			y = 56;
			M_PrintWhite (16, y,  "Programming and Project Lead"); y += 8;
			M_Print (16, y,	      " Entar"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "QuakeC Programming"); y += 8;
			M_Print (16, y,	      " MauveBib"); y += 8;
			M_Print (16, y,	      " DarkSnow"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Textures and models by QRP"); y += 8;
			M_PrintWhite (16, y,  "http://facelift.quakedev.com"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Vr2 engine based on QMB by"); y += 8;
			M_Print (16, y,       " DrLabman"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Special thanks to"); y += 8;
			M_Print (16, y,       " All the people at QSG who"); y += 8;
			M_Print (16, y,       " helped with coding support"); y += 16;
			M_PrintWhite (16, y,       "And those who contributed code"); y += 8;
			M_PrintWhite (16, y,       "(or whom I nicked code from):"); y += 16;
			M_Print (16, y,       "Reckless, KrimZon, frag.machine,"); y += 8;
			M_Print (16, y,       "mh, DrLabman, Tomaz, LordHavoc"); y += 16;
		}
		else if (gamemode == GAME_RO)
		{
			y = 16 + ((vid.height/2)-120);
			M_CenterprintWhite	(y, "Hunter Pack");
			y += 8;
			M_CenterprintWhite	(y, "A Standalone game");
			y = (int)(host_time * 10)%6;
			//M_DrawTransPic (160, 26, Draw_CachePic( va("gfx/menudot%i.lmp", y+1 ) ) );
			M_DrawTransPic (160, 32, Draw_CachePic( va("gfx/menudot%i.lmp", y+1 ) ) );

			y = 56;
			M_PrintWhite (16, y,  "Programming and Project Lead"); y += 8;
			M_Print (16, y,	      " Aionita Cosmin"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "QuakeC Programming"); y += 8;
			M_Print (16, y,	      " Aionita Cosmin"); y += 8;
			M_Print (16, y,	      " Other dude"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Textures and models by dudes"); y += 8;
			M_PrintWhite (16, y,  "	Another dude!"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Based on Vr2 by"); y += 8;
			M_Print (16, y,       " Entar"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Other Team Members"); y += 8;
			M_Print (16, y,       " "); y += 8;
			M_Print (16, y,       " helped with coding support"); y += 16;
			M_PrintWhite (16, y,       "And those who contributed code"); y += 8;
			M_PrintWhite (16, y,       "(or whom I nicked code from):"); y += 16;
			M_Print (16, y,       "Reckless, KrimZon, frag.machine,"); y += 8;
			M_Print (16, y,       "mh, DrLabman, Tomaz, LordHavoc"); y += 16;
		}
		else // Vengeance r2 credits
		{
			y = 16 + ((vid.height/2)-120);
			M_CenterprintWhite	(y, "Vengeance r2");
			y += 8;
			M_CenterprintWhite	(y, "A Quake Engine Project");
			y = (int)(host_time * 10)%6;
			//M_DrawTransPic (160, 26, Draw_CachePic( va("gfx/menudot%i.lmp", y+1 ) ) );
			M_DrawTransPic (160, 32, Draw_CachePic( va("gfx/menudot%i.lmp", y+1 ) ) );

			y = 56;
			M_PrintWhite (16, y,  "Programming and Project Lead"); y += 8;
			M_Print (16, y,	      " Entar"); y += 8;
			y += 8;
			M_PrintWhite (16, y,  "Based on QMB by"); y += 8;
			M_Print (16, y,       " DrLabman"); y += 8;
			y += 16;
			M_PrintWhite (16, y,  "Special thanks to"); y += 16;
			M_Print (16, y,       " All the people at QSG who"); y += 8;
			M_Print (16, y,       " helped with coding support"); y += 16;
			M_PrintWhite (16, y,       "And those who contributed code"); y += 8;
			M_PrintWhite (16, y,       "(or whom I nicked code from):"); y += 16;
			M_Print (16, y,       "Reckless, KrimZon, frag.machine,"); y += 8;
			M_Print (16, y,       "mh, DrLabman, Tomaz, LordHavoc, Spike"); y += 16;
		}
	}
	else if (credits_page == CREDITS_ID)
	{
		y = 16 + ((vid.height/2)-120);
		M_CenterprintWhite	(y, "QUAKE by id Software");

		y = 46;
		M_Print (16, y,       " Based on Quake1,"); y += 8;
		M_Print (16, y,       " so let's thank id software as well,"); y += 16;

		M_PrintWhite (16, y,  "Programming        Art"); y += 8;
		M_Print (16, y,	      " John Carmack       Adrian Carmack"); y += 8;
		M_Print (16, y,		  " Michael Abrash     Kevin Cloud"); y += 8;
		M_Print (16, y,       " John Cash          Paul Steed"); y += 8;
		M_Print (16, y,       " Dave 'Zoid' Kirsch"); y += 8;
		y += 8;
		M_PrintWhite (16, y,  "Design             Biz"); y += 8;
		M_Print (16, y,       " John Romero        Jay Wilbur"); y += 8;
		M_Print (16, y,       " Sandy Petersen     Mike Wilson"); y += 8;
		M_Print (16, y,       " American McGee     Donna Jackson"); y += 8;
		M_Print (16, y,       " Tim Willits        Todd Hollenshead"); y += 8;
		y += 8;
		M_PrintWhite (16, y,  "Support            Projects\n"); y += 8;
		M_Print (16, y,       " Barrett Alexander  Shawn Green\n"); y += 8;
		y += 8;
		M_PrintWhite (16, y,  "Sound Effects"); y += 8;
		M_Print (16, y,       " Trent Reznor and Nine Inch Nails"); y += 8;
		y += 56;
		M_PrintWhite (16, y, "Quake is a trademark of Id Software,"); y += 8;
		M_PrintWhite (16, y, "inc., (c)1996 Id Software, inc. All"); y += 8;
		M_PrintWhite (16, y, "rights reserved. NIN logo is a"); y += 8;
		M_PrintWhite (16, y, "registered trademark licensed to"); y += 8;
		M_PrintWhite (16, y, "Nothing Interactive, Inc. All rights"); y += 8;
		M_PrintWhite (16, y, "reserved.");
	}
}

void M_Credits_Key (int key)
{
	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		if (gamemode == GAME_CHRONICLE)
			M_Menu_Main_f ();
		else
			M_Menu_SinglePlayer_f ();
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++credits_page >= NUM_CREDITS_PAGES)
			credits_page = 0;
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--credits_page < 0)
			credits_page = NUM_CREDITS_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
int		m_quit_prevstate;
qboolean	wasInMenus;

char *quitMessage [] = 
{
/* .........1.........2.... *
  "  Are you gonna quit    ",
  "  this game just like   ",
  "   everything else?     ",
  "                        ",
 
  " Milord, methinks that  ",
  "   thou art a lowly     ",
  " quitter. Is this true? ",
  "                        ",

  " Do I need to bust your ",
  "  face open for trying  ",
  "        to quit?        ",
  "                        ",

  " Man, I oughta smack you",
  "   for trying to quit!  ",
  "     Press Y to get     ",
  "      smacked out.      ",
 
  " Press Y to quit like a ",
  "   big loser in life.   ",
  "  Press N to stay proud ",
  "    and successful!     ",
 
  "   If you press Y to    ",
  "  quit, I will summon   ",
  "  Satan all over your   ",
  "      hard drive!       ",
 
  "  Um, Asmodeus dislikes ",
  " his children trying to ",
  " quit. Press Y to return",
  "   to your Tinkertoys.  ",
 
  "  If you quit now, I'll ",
  "  throw a blanket-party ",
  "   for you next time!   ",
  "                        "*/

// new messages - Entar
  "      Vengeance r2      ",
  "        by Entar        ",
  "      based on QMB      ",
  "       by DrLabman      ",
 
  " Milord, methinks that  ",
  "   thou art a lowly     ",
  " quitter. Is this true? ",
  "                        ",

  " Do I need to bust your ",
  "  face open for trying  ",
  "        to quit?        ",
  "                        ",

  "      Don't leave,      ",
  "      Keep Playing      ",
  "   It's good for you.   ",
  "     Really, it is.     ",
 
  " Press Y to quit like a ",
  "   big loser in life.   ",
  "  Press N to stay proud ",
  "    and successful!     ",
 
  "      Vengeance r2      ",
  "        by Entar        ",
  "      based on QMB      ",
  "       by DrLabman      ",
 
  "  Um, Asmodeus dislikes ",
  " his children trying to ",
  " quit. Press Y to return",
  "   to your Tinkertoys.  ",
 
  "      Vengeance r2      ",
  "        by Entar        ",
  "      based on QMB      ",
  "       by DrLabman      ",
};

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;
}

void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		key_dest = key_console;
		Host_Quit_f ();
		break;

	default:
		break;
	}
}

void M_Quit_Draw (void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	M_DrawTextBox (56, 76, 24, 4);
	M_Print (64, 84,  quitMessage[msgNumber*4+0]);
	M_Print (64, 92,  quitMessage[msgNumber*4+1]);
	M_Print (64, 100, quitMessage[msgNumber*4+2]);
	M_Print (64, 108, quitMessage[msgNumber*4+3]);
}

//=============================================================================

/* SERIAL CONFIG MENU */

int		serialConfig_cursor;
int		serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define	NUM_SERIALCONFIG_CMDS	6

static int ISA_uarts[]	= {0x3f8,0x2f8,0x3e8,0x2e8};
static int ISA_IRQs[]	= {4,3,4,3};
int serialConfig_baudrate[] = {9600,14400,19200,28800,38400,57600};

int		serialConfig_comport;
int		serialConfig_irq ;
int		serialConfig_baud;
char	serialConfig_phone[16];

void M_Menu_SerialConfig_f (void)
{
	int		n;
	int		port;
	int		baudrate;
	qboolean	useModem;

	key_dest = key_menu;
	m_state = m_serialconfig;
	m_entersound = true;
	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;

	(*GetComPortConfig) (0, &port, &serialConfig_irq, &baudrate, &useModem);

	// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;
	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}
	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;
	if (n == 6)
		n = 5;
	serialConfig_baud = n;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_SerialConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*directModem;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

if (StartingGame)
		startJoin = "Create game";
	else
		startJoin = "Join game";
	if (SerialConfig)
		directModem = "Modem";
	else
		directModem = "Direct connect";
	M_Print (basex, 32, va ("%s - %s", startJoin, directModem));
	basex += 8;

	M_Print (basex, serialConfig_cursor_table[0], "Port");
	M_DrawTextBox (160, 40, 4, 1);
	M_PrintWhite (168, serialConfig_cursor_table[0], va("COM%u", serialConfig_comport));

	M_Print (basex, serialConfig_cursor_table[1], "IRQ");
	M_DrawTextBox (160, serialConfig_cursor_table[1]-8, 1, 1);
	M_PrintWhite (168, serialConfig_cursor_table[1], va("%u", serialConfig_irq));

	M_Print (basex, serialConfig_cursor_table[2], "Baud");
	M_DrawTextBox (160, serialConfig_cursor_table[2]-8, 5, 1);
	M_PrintWhite (168, serialConfig_cursor_table[2], va("%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print (basex, serialConfig_cursor_table[3], "Modem Setup...");
		if (JoiningGame)
		{
			M_Print (basex, serialConfig_cursor_table[4], "Phone number");
			M_DrawTextBox (160, serialConfig_cursor_table[4]-8, 16, 1);
			M_PrintWhite (168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 7, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "Connect");
	}
	else
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 2, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "OK");
	}

	M_DrawCharacter (basex-8, serialConfig_cursor_table [serialConfig_cursor], 12+((int)(realtime*4)&1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter (168 + 8*strlen(serialConfig_phone), serialConfig_cursor_table [serialConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_SerialConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor--;
		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor++;
		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;
		break;

	case K_LEFTARROW:
		if (serialConfig_cursor > 2)
			break;
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;
			if (serialConfig_comport == 0)
				serialConfig_comport = 4;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;
			if (serialConfig_irq == 6)
				serialConfig_irq = 5;
			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;
			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:
		if (serialConfig_cursor > 2)
			break;
forward:
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;
			if (serialConfig_comport > 4)
				serialConfig_comport = 1;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;
			if (serialConfig_irq == 6)
				serialConfig_irq = 7;
			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;
			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_ENTER:
		if (serialConfig_cursor < 3)
			goto forward;

		m_entersound = true;

		if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_Menu_ModemConfig_f ();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

		// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem ();

		if (StartingGame)
		{
			M_Menu_GameOptions_f ();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;
		key_dest = key_game;
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText (va ("connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText ("connect\n");
		break;

	case K_BACKSPACE:
		if (serialConfig_cursor == 4)
		{
			if (strlen(serialConfig_phone))
				serialConfig_phone[strlen(serialConfig_phone)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;
		if (serialConfig_cursor == 4)
		{
			l = strlen(serialConfig_phone);
			if (l < 15)
			{
				serialConfig_phone[l+1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;

	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
}

//=============================================================================
/* MODEM CONFIG MENU */

int		modemConfig_cursor;
int		modemConfig_cursor_table [] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS	5

char	modemConfig_dialing;
char	modemConfig_clear [16];
char	modemConfig_init [32];
char	modemConfig_hangup [16];

void M_Menu_ModemConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_modemconfig;
	m_entersound = true;
	(*GetModemConfig) (0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}


void M_ModemConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print (basex, modemConfig_cursor_table[0], "Pulse Dialing");
	else
		M_Print (basex, modemConfig_cursor_table[0], "Touch Tone Dialing");

	M_Print (basex, modemConfig_cursor_table[1], "Clear");
	M_DrawTextBox (basex, modemConfig_cursor_table[1]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[1]+12, modemConfig_clear);
	if (modemConfig_cursor == 1)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_clear), modemConfig_cursor_table[1]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[2], "Init");
	M_DrawTextBox (basex, modemConfig_cursor_table[2]+4, 30, 1);
	M_PrintWhite (basex+8, modemConfig_cursor_table[2]+12, modemConfig_init);
	if (modemConfig_cursor == 2)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_init), modemConfig_cursor_table[2]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[3], "Hangup");
	M_DrawTextBox (basex, modemConfig_cursor_table[3]+4, 16, 1);
	M_PrintWhite (basex+8, modemConfig_cursor_table[3]+12, modemConfig_hangup);
	if (modemConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_hangup), modemConfig_cursor_table[3]+12, 10+((int)(realtime*4)&1));

	M_DrawTextBox (basex, modemConfig_cursor_table[4]-8, 2, 1);
	M_Print (basex+8, modemConfig_cursor_table[4], "OK");

	M_DrawCharacter (basex-8, modemConfig_cursor_table [modemConfig_cursor], 12+((int)(realtime*4)&1));
}


void M_ModemConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_SerialConfig_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor--;
		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor++;
		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			S_LocalSound ("misc/menu1.wav");
		}
		break;

	case K_ENTER:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			m_entersound = true;
		}

		if (modemConfig_cursor == 4)
		{
			(*SetModemConfig) (0, va ("%c", modemConfig_dialing), modemConfig_clear, modemConfig_init, modemConfig_hangup);
			m_entersound = true;
			M_Menu_SerialConfig_f ();
		}
		break;

	case K_BACKSPACE:
		if (modemConfig_cursor == 1)
		{
			if (strlen(modemConfig_clear))
				modemConfig_clear[strlen(modemConfig_clear)-1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen(modemConfig_init))
				modemConfig_init[strlen(modemConfig_init)-1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen(modemConfig_hangup))
				modemConfig_hangup[strlen(modemConfig_hangup)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen(modemConfig_clear);
			if (l < 15)
			{
				modemConfig_clear[l+1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen(modemConfig_init);
			if (l < 29)
			{
				modemConfig_init[l+1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen(modemConfig_hangup);
			if (l < 15)
			{
				modemConfig_hangup[l+1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {72, 92, 124};
#define NUM_LANCONFIG_CMDS	3

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor == 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	char	*startJoin;
	char	*protocol;

	M_Main_Layout (M_M_MULTI, false);
	
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

	if (StartingGame)
		startJoin = "Create game";
	else
		startJoin = "Join game";
	if (IPXConfig)
		protocol = "IPX";
	else
		protocol = "TCP/IP";
	M_Print (basex, 32, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 52, "Address:");
	if (IPXConfig)
		M_PrintWhite (basex+9*8, 52, my_ipx_address);
	else
		M_PrintWhite (basex+9*8, 52, my_tcpip_address);

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_PrintWhite (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Search for local games...");
		M_Print (basex, 108, "Join game at:");
		M_DrawTextBox (basex+8, lanConfig_cursor_table[2]-8, 22, 1);
		M_PrintWhite (basex+16, lanConfig_cursor_table[2], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 2)
		M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [2], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem ();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 2)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			key_dest = key_game;
			m_state = m_none;
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 2)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 2)
		{
			l = strlen(lanConfig_joinname);
			if (l < 21)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;

	l =  Q_atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
   {"start", "Command HQ"},  // 0

   {"hip1m1", "The Pumping Station"},          // 1
   {"hip1m2", "Storage Facility"},
   {"hip1m3", "The Lost Mine"},
   {"hip1m4", "Research Facility"},
   {"hip1m5", "Military Complex"},

   {"hip2m1", "Ancient Realms"},          // 6
   {"hip2m2", "The Black Cathedral"},
   {"hip2m3", "The Catacombs"},
   {"hip2m4", "The Crypt"},
   {"hip2m5", "Mortum's Keep"},
   {"hip2m6", "The Gremlin's Domain"},

   {"hip3m1", "Tur Torment"},       // 12
   {"hip3m2", "Pandemonium"},
   {"hip3m3", "Limbo"},
   {"hip3m4", "The Gauntlet"},

   {"hipend", "Armagon's Lair"},       // 16

   {"hipdm1", "The Edge of Oblivion"}           // 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

episode_t   hipnoticepisodes[] =
{
   {"Scourge of Armagon", 0, 1},
   {"Fortress of the Dead", 1, 5},
   {"Dominion of Darkness", 6, 6},
   {"The Rift", 12, 4},
   {"Final Level", 16, 1},
   {"Deathmatch Arena", 17, 1}
};

episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

void M_Menu_GameOptions_f (void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = svs.maxclientslimit;
}


int gameoptions_cursor_table[] =    {BUTTON_START,
									 BUTTON_START+6+BUTTON_HEIGHT,
									 BUTTON_START+6+BUTTON_HEIGHT*2,
									 BUTTON_START+6+BUTTON_HEIGHT*3,
									 BUTTON_START+6+BUTTON_HEIGHT*4,
									 BUTTON_START+6+BUTTON_HEIGHT*5,
									 BUTTON_START+6+BUTTON_HEIGHT*6,
									 BUTTON_START+6+BUTTON_HEIGHT*8,
									 BUTTON_START+6+BUTTON_HEIGHT*9};
#define	NUM_GAMEOPTIONS	9

#define	ITEM_M_BEGIN	0
#define	ITEM_M_PLAYERS	1
#define	ITEM_M_RULES	2
#define	ITEM_M_TEAMS	3
#define	ITEM_M_SKILL	4
#define	ITEM_M_FRAGS	5
#define	ITEM_M_TIME 	6
#define	ITEM_M_EPISODE	7
#define	ITEM_M_LEVEL	8

int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	qpic_t	*p;
	int		y;
	char *msg;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	y = gameoptions_cursor_table[ITEM_M_BEGIN];
	M_DrawTextBox (152, y-8, 10, 1);
	M_Print (160, y, "Begin game");

	y = gameoptions_cursor_table[ITEM_M_PLAYERS];
	M_Print (0, y, "      Max players");
	M_PrintWhite (160, y, va("%i", maxplayers) );

	y = gameoptions_cursor_table[ITEM_M_RULES];
	M_Print (0, y, "        Game type");
	if (coop.value)
		M_PrintWhite (160, y, "Cooperative");
	else
		M_PrintWhite (160, y, "Deathmatch");

	y = gameoptions_cursor_table[ITEM_M_TEAMS];
	M_Print (0, y, "         Teamplay");

	if (rogue)
	{
		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_PrintWhite (160, y, msg);
	}
	else
	{
		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_PrintWhite (160, y, msg);
	}


	y = gameoptions_cursor_table[ITEM_M_SKILL];
	M_Print (0, y, "            Skill");
	if (skill.value == 0)
		M_PrintWhite (160, y, "Easy difficulty");
	else if (skill.value == 1)
		M_PrintWhite (160, y, "Normal difficulty");
	else if (skill.value == 2)
		M_PrintWhite (160, y, "Hard difficulty");
	else
		M_PrintWhite (160, y, "Nightmare difficulty");

	y = gameoptions_cursor_table[ITEM_M_FRAGS];
	M_Print (0, y, "       Frag Limit");
	if (fraglimit.value == 0)
		M_PrintWhite (160, y, "none");
	else
		M_PrintWhite (160, y, va("%i frags", (int)fraglimit.value));

	y = gameoptions_cursor_table[ITEM_M_TIME];
	M_Print (0, y, "       Time Limit");
	if (timelimit.value == 0)
		M_PrintWhite (160, y, "none");
	else
		M_PrintWhite (160, y, va("%i minutes", (int)timelimit.value));

	y = gameoptions_cursor_table[ITEM_M_EPISODE];
	M_Print (0, y, "          Episode");
    M_PrintWhite (160, y, episodes[startepisode].description);

	y = gameoptions_cursor_table[ITEM_M_LEVEL];
	M_Print (0, y, "            Level");
 
	M_PrintWhite (160, y, levels[episodes[startepisode].firstLevel + startlevel].description);
    M_PrintWhite (160, y+BUTTON_HEIGHT, levels[episodes[startepisode].firstLevel + startlevel].name);
 
// line cursor
	M_DrawCharacter (144, gameoptions_cursor_table[gameoptions_cursor], 12+((int)(realtime*4)&1));

// help message
	if (gameoptions_cursor == ITEM_M_PLAYERS)
	{
		M_CenterprintWhite (vid.height-16, "For more than 4 players it is recommended to");
		M_CenterprintWhite (vid.height-8, "use \"-listen\" command line parameter.");
	}
}

void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue ("coop", coop.value ? 0 : 1);
		break;

	case 3:
		if (rogue)
			count = 6;
		else
			count = 2;

		Cvar_SetValue ("teamplay", teamplay.value + dir);
		if (teamplay.value > count)
			Cvar_SetValue ("teamplay", 0);
		else if (teamplay.value < 0)
			Cvar_SetValue ("teamplay", count);
		break;

	case 4:
		Cvar_SetValue ("skill", skill.value + dir);
		if (skill.value > 3)
			Cvar_SetValue ("skill", 0);
		if (skill.value < 0)
			Cvar_SetValue ("skill", 3);
		break;

	case 5:
		Cvar_SetValue ("fraglimit", fraglimit.value + dir*10);
		if (fraglimit.value > 100)
			Cvar_SetValue ("fraglimit", 0);
		if (fraglimit.value < 0)
			Cvar_SetValue ("fraglimit", 100);
		break;

	case 6:
		Cvar_SetValue ("timelimit", timelimit.value + dir*5);
		if (timelimit.value > 60)
			Cvar_SetValue ("timelimit", 0);
		if (timelimit.value < 0)
			Cvar_SetValue ("timelimit", 60);
		break;

	case 7:
		startepisode += dir;
	//MED 01/06/97 added hipnotic count
		if (hipnotic)
			count = 6;
	//PGM 01/07/97 added rogue count
	//PGM 03/02/97 added 1 for dmatch episode
		else if (rogue)
			count = 4;
		else if (registered.value)
			count = 7;
		else
			count = 2;

		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 8:
		startlevel += dir;
    //MED 01/06/97 added hipnotic episodes
		if (hipnotic)
			count = hipnoticepisodes[startepisode].levels;
	//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
			count = rogueepisodes[startepisode].levels;
		else
			count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			SCR_BeginLoadingPlaque ();

			if (hipnotic)
				Cbuf_AddText ( va ("map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name) );
			else if (rogue)
				Cbuf_AddText ( va ("map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name) );
			else
				Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}

		M_NetStart_Change (1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;

void M_Menu_Search_f (void)
{
	key_dest = key_menu;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	qpic_t	*p;
	int x;

	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12, 1);
	M_Print (x, 40, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64];
	qpic_t	*p;

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Q_memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Q_memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Q_memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}
	M_Main_Layout (M_M_MULTI, false);

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
			sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		else
			sprintf(string, "%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		M_Print (16, 32 + 8*n, string);
	}
	M_DrawCharacter (0, 32 + slist_cursor*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		Cbuf_AddText ( va ("connect \"%s\"\n", hostcache[slist_cursor].cname) );
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */
cvar_t r_menutint = {"r_menutint", "0.93 0.42 0.13", true};

void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_graphics", M_Menu_Graphics_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
	Cmd_AddCommand ("credits", M_Menu_Credits_f);

	Cmd_AddCommand ("menu_server", M_Menu_Server_f);

	Cvar_RegisterVariable (&r_menutint);
}

void M_DrawMouse (void)
{
//	Draw_TransPic (current_pos.x - omousex, current_pos.y - omousey, Draw_CachePic( va("gfx/menudot1.lmp") ) );
	Draw_Character (current_pos.x - omousex - 3, current_pos.y - omousey - 3, '+');
//	M_Print(5, 5, va("X: %i", current_pos.x ));
//	M_Print(5, 10, va("Y: %i", current_pos.y ));
}

// Code from FTE, edited by Entar
/*
================
Draw_FadeScreen
================
*/
vec3_t fadecolor;
int faderender;
int fademodified;

void Draw_MenuFade (void)
{
	extern cvar_t r_menutint;

	if (fademodified != r_menutint.value)
	{
		char *t;

		// parse r_menutint and clear defaults
		fadecolor[0] = r_menutint.value;
		fadecolor[1] = 0;
		fadecolor[2] = 0;

		faderender = GL_DST_COLOR;

		t = strstr(r_menutint.string, " ");
		if (t)
		{
			fadecolor[1] = atof(t+1);
			t = strstr(t+1, " ");
			if (t)
				fadecolor[2] = atof(t+1);
			else
				faderender = 0;
		}
		else
			faderender = 0;

		// bounds check and inverse check
		if (faderender)
		{
			if (fadecolor[0] < 0)
			{
				faderender = GL_ONE_MINUS_DST_COLOR;
				fadecolor[0] = -(fadecolor[0]);
			}
			if (fadecolor[0] > 1)
				fadecolor[0] = 1;

			if (fadecolor[1] < 0)
			{
				faderender = GL_ONE_MINUS_DST_COLOR;
				fadecolor[1] = -(fadecolor[1]);
			}
			if (fadecolor[1] > 1)
				fadecolor[1] = 1;

			if (fadecolor[2] < 0)
			{
				faderender = GL_ONE_MINUS_DST_COLOR;
				fadecolor[2] = -(fadecolor[2]);
			}
			if (fadecolor[2] > 1)
				fadecolor[2] = 1;
		}

		fademodified = r_menutint.value;
	}

	if (!faderender)
		return;

	glEnable (GL_BLEND);
	glBlendFunc(faderender, GL_ZERO);
	glDisable(GL_ALPHA_TEST);
	glDisable (GL_TEXTURE_2D);
	glColor4f (fadecolor[0], fadecolor[1], fadecolor[2], 1);
	glBegin (GL_QUADS);

	glVertex2f (0,0);
	glVertex2f (vid.width, 0);
	glVertex2f (vid.width, vid.height);
	glVertex2f (0, vid.height);

	glEnd ();
	glColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);
	glDisable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);

	Sbar_Changed();
}

void M_Draw (void)
{
	if (m_state == m_none || key_dest != key_menu)
		return;

	Draw_MenuFade();

	if (!m_recursiveDraw)
	{
		scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			S_ExtraUpdate ();
		}

		scr_fullupdate = 0;
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_vid_options:
		M_Video_Draw ();
		break;

	case m_video:
		M_VideoModes_Draw ();
		break;

	case m_gfx_options:
		M_Graphics_Draw();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_credits:
		M_Credits_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_serialconfig:
		M_SerialConfig_Draw ();
		break;

	case m_modemconfig:
		M_ModemConfig_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;

	case m_serveroptions: // server options menu
		M_Server_Draw ();
		break;
	}

	// Entar : regardless of what menu we're in, draw the mouse pointer
	M_DrawMouse();

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_Keydown (int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_vid_options:
		M_Video_Key (key);
		return;

	case m_video:
		M_VideoModes_Key (key);
		return;

	case m_gfx_options:
		M_Graphics_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_credits:
		M_Credits_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_serialconfig:
		M_SerialConfig_Key (key);
		return;

	case m_modemconfig:
		M_ModemConfig_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;

	case m_serveroptions:
		M_Server_Key (key);
		return;
	}
}


void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText ("stopdemo\n");
	if (SerialConfig || DirectConfig)
	{
		Cbuf_AddText ("com1 enable\n");
	}

	if (IPXConfig || TCPIPConfig)
		net_hostport = lanConfig_port;
}
