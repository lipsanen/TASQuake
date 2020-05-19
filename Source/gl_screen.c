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
// gl_screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"
#ifdef _WIN32
#include "movie.h"
#endif

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net 
turn off messages option

the refresh is always rendered, unless the console is full screen


console is:
	notify lines
	half
	full

*/


int		glx, gly, glwidth, glheight;

// only the refresh window will be updated unless these variables are flagged 
int		scr_copytop;
int		scr_copyeverything;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

float		oldscreensize, oldfov, oldsbar;
cvar_t		scr_viewsize = {"viewsize", "100", CVAR_ARCHIVE};
cvar_t		scr_fov = {"fov", "90"};	// 10 - 170
cvar_t		scr_consize = {"scr_consize", "0.5"};
cvar_t		scr_conspeed = {"scr_conspeed", "1000"};
cvar_t		scr_centertime = {"scr_centertime", "2"};
cvar_t		scr_showram = {"showram", "1"};
cvar_t		scr_showturtle = {"showturtle", "0"};
cvar_t		scr_showpause = {"showpause", "1"};
cvar_t		scr_printspeed = {"scr_printspeed", "8"};
cvar_t		gl_triplebuffer = {"gl_triplebuffer", "1", CVAR_ARCHIVE};
cvar_t		scr_sshot_format = {"scr_sshot_format", "jpg"};
cvar_t		scr_autoid = {"scr_autoid", "0"};

extern	cvar_t	crosshair;
extern	cvar_t	cl_crossx;
extern	cvar_t	cl_crossy;

qboolean	scr_initialized;		// ready to draw

mpic_t          *scr_ram;
mpic_t          *scr_net; 
mpic_t          *scr_turtle;

int		scr_fullupdate;

int		clearconsole;
int		clearnotify;

int		sb_lines;

viddef_t	vid;				// global video state

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

qboolean	block_drawing;

void SCR_ScreenShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int		scr_center_lines;
int		scr_erase_lines;
int		scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	Q_strncpyz (scr_centerstring, str, sizeof(scr_centerstring));
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_DrawCenterString (void)
{
	char	*start;
	int	l, j, x, y, remaining;

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = vid.height * 0.35;
	else
		y = 48;

	do {
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (vid.width - l*8) / 2;
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);	
			if (!remaining--)
				return;
		}
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_copytop = 1;
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;

	if (key_dest != key_game)
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float   a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);

	a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

float InvCalcFov (float fov_y, float width, float height)
{
	float	a, x, fov_x;

	a = fov_y * M_PI / 360;

	x = height / tan(a);

	fov_x = atan(width / x) * (360 / M_PI);

	return fov_x;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	int		h;
	float		size;
	qboolean	full = false;

	scr_fullupdate = 0;		// force a background redraw
	vid.recalc_refdef = 0;

// force the status bar to redraw
	Sbar_Changed ();

//========================================
	
// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetValue (&scr_viewsize, 30);
	if (scr_viewsize.value > 120)
		Cvar_SetValue (&scr_viewsize, 120);

// bound field of view
	if (scr_fov.value < 10)
		Cvar_SetValue (&scr_fov, 10);
	if (scr_fov.value > 170)
		Cvar_SetValue (&scr_fov, 170);

// intermission is always full screen	
	size = cl.intermission ? 120 : scr_viewsize.value;

	if (size >= 120)
		sb_lines = 0;		// no status bar at all
	else if (size >= 110)
		sb_lines = 24;		// no inventory
	else
		sb_lines = 24 + 16 + 8;

	if (scr_viewsize.value >= 100.0)
	{
		full = true;
		size = 100.0;
	}
	else
	{
		size = scr_viewsize.value;
	}

	if (cl.intermission)
	{
		full = true;
		size = 100.0;
		sb_lines = 0;
	}
	size /= 100.0;

	h = (!cl_sbar.value && full) ? vid.height : vid.height - sb_lines;

	if (!r_norefresh.value)
	{
		r_refdef.vrect.width = vid.width * size;
		if (r_refdef.vrect.width < 96)
		{
			size = 96.0 / r_refdef.vrect.width;
			r_refdef.vrect.width = 96;	// min for icons
		}

		r_refdef.vrect.height = vid.height * size;

		if (cl_sbar.value || !full)
		{
			if (r_refdef.vrect.height > vid.height - sb_lines)
				r_refdef.vrect.height = vid.height - sb_lines;
		}
		else if (r_refdef.vrect.height > vid.height)
		{
			r_refdef.vrect.height = vid.height;
		}

		r_refdef.vrect.x = (vid.width - r_refdef.vrect.width) / 2;

		if (full)
			r_refdef.vrect.y = 0;
		else
			r_refdef.vrect.y = (h - r_refdef.vrect.height) / 2;

		r_refdef.fov_x = scr_fov.value;
		r_refdef.fov_y = CalcFov(r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);
	}
	else
	{
		r_refdef.vrect.width = 0;
		r_refdef.vrect.height = 0;
		r_refdef.vrect.x = 0;
		r_refdef.vrect.y = 0;
		r_refdef.fov_x = 1;
		r_refdef.fov_y = 1;
	}

//	r_refdef.fov_x = InvCalcFov (r_refdef.fov_y, 640, 400);	//oldman

	scr_vrect = r_refdef.vrect;
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValue (&scr_viewsize, scr_viewsize.value + 10);
	vid.recalc_refdef = 1;
}

/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValue (&scr_viewsize, scr_viewsize.value - 10);
	vid.recalc_refdef = 1;
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	Cvar_Register (&scr_fov);
	Cvar_Register (&scr_viewsize);
	Cvar_Register (&scr_consize);
	Cvar_Register (&scr_conspeed);
	Cvar_Register (&scr_showram);
	Cvar_Register (&scr_showturtle);
	Cvar_Register (&scr_showpause);
	Cvar_Register (&scr_centertime);
	Cvar_Register (&scr_printspeed);
	Cvar_Register (&gl_triplebuffer);
	Cvar_Register (&scr_sshot_format);
	Cvar_Register (&scr_autoid);

// register our commands
	Cmd_AddCommand ("screenshot", SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup", SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown", SCR_SizeDown_f);

	scr_ram = Draw_PicFromWad ("ram");
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");

#ifdef _WIN32
	Movie_Init ();
#endif

	scr_initialized = true;
}

/*
==============
SCR_DrawRam
==============
*/
void SCR_DrawRam (void)
{
	if (!scr_showram.value || !r_cache_thrash)
		return;

	Draw_Pic (scr_vrect.x + 32, scr_vrect.y, scr_ram);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static	int	count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3 || cls.demoplayback)
		return;

	Draw_Pic (scr_vrect.x + 64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	mpic_t	*pic;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	if (!cl.paused)
		return;

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ((vid.width - pic->width) / 2, (vid.height - 48 - pic->height) / 2, pic);
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	mpic_t	*pic;

	if (!scr_drawloading)
		return;
		
	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ((vid.width - pic->width) / 2, (vid.height - 48 - pic->height) / 2, pic);
}

//=============================================================================

/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = vid.height;		// full screen
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
	{
		scr_conlines = vid.height * scr_consize.value;
		if (scr_conlines < 30)
			scr_conlines = 30;
		if (scr_conlines > vid.height - 10)
			scr_conlines = vid.height - 10;
	}
	else
	{
		scr_conlines = 0;			// none visible
	}

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed.value * host_frametime * vid.height / 320;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed.value * host_frametime * vid.height / 320;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
		Sbar_Changed ();
	else if (clearnotify++ < vid.numpages)
	{
	}
	else
		con_notifylines = 0;
}
	
/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		scr_copyeverything = 1;
		Con_DrawConsole (scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}

//=============================================================================

int qglProject (float objx, float objy, float objz, float *model, float *proj, int *view, float* winx, float* winy, float* winz)
{
	int	i;
	float	in[4], out[4];

	in[0] = objx, in[1] = objy, in[2] = objz, in[3] = 1.0;

	for (i=0 ; i<4 ; i++)
		out[i] = in[0] * model[0*4+i] + in[1] * model[1*4+i] + in[2] * model[2*4+i] + in[3] * model[3*4+i];

	for (i=0 ; i<4 ; i++)
		in[i]  = out[0] * proj[0*4+i] + out[1] * proj[1*4+i] + out[2] * proj[2*4+i] + out[3] * proj[3*4+i];

	if (!in[3])
		return 0;

	VectorScale (in, 1 / in[3], in);

	*winx = view[0] + (1 + in[0]) * view[2] / 2;
	*winy = view[1] + (1 + in[1]) * view[3] / 2;
	*winz = (1 + in[2]) / 2;

	return 1;
}

typedef struct player_autoid_s
{
	float		x, y;
	scoreboard_t	*player;
} autoid_player_t;

static	autoid_player_t	autoids[MAX_SCOREBOARDNAME];
static	int		autoid_count;

#define ISDEAD(i) ((i) >= 41 && (i) <= 102)

void SCR_SetupAutoID (void)
{
	int		i, view[4];
	float		model[16], project[16], winz, *origin;
	entity_t	*state;
	autoid_player_t	*id;

	autoid_count = 0;

	if (!scr_autoid.value || cls.state != ca_connected || !cls.demoplayback)
		return;

	glGetFloatv (GL_MODELVIEW_MATRIX, model);
	glGetFloatv (GL_PROJECTION_MATRIX, project);
	glGetIntegerv (GL_VIEWPORT, view);

	for (i = 0 ; i < cl.maxclients ; i++)
	{
		state = &cl_entities[1+i];

		if ((state->modelindex == cl_modelindex[mi_player] && ISDEAD(state->frame)) || 
		     state->modelindex == cl_modelindex[mi_h_player])
			continue;

		if (R_CullSphere(state->origin, 0))
			continue;

		id = &autoids[autoid_count];
		id->player = &cl.scores[i];
		origin = state->origin;
		if (qglProject(origin[0], origin[1], origin[2] + 28, model, project, view, &id->x, &id->y, &winz))
			autoid_count++;
	}
}

void SCR_DrawAutoID (void)
{
	int	i, x, y;

	if (!scr_autoid.value || !cls.demoplayback)
		return;

	for (i = 0 ; i < autoid_count ; i++)
	{
		x = autoids[i].x * vid.width / glwidth;
		y = (glheight - autoids[i].y) * vid.height / glheight;
		Draw_String (x - strlen(autoids[i].player->name) * 4, y - 8, autoids[i].player->name);
	}
}

/* 
============================================================================== 
 
				SCREEN SHOTS 
 
============================================================================== 
*/ 

extern	unsigned short	ramps[3][256];

void ApplyGamma (byte *buffer, int size)
{
	int	i;

	if (!vid_hwgamma_enabled)
		return;

	for (i=0 ; i<size ; i+=3)
	{
		buffer[i+0] = ramps[0][buffer[i+0]] >> 8;
		buffer[i+1] = ramps[1][buffer[i+1]] >> 8;
		buffer[i+2] = ramps[2][buffer[i+2]] >> 8;
	}
}

int SCR_ScreenShot (char *name)
{
	qboolean	ok = false;
	int		buffersize = glwidth * glheight * 3;
	byte		*buffer;
	char		*ext;

	ext = COM_FileExtension (name);

	buffer = Q_malloc (buffersize);
	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	ApplyGamma (buffer, buffersize);

	if (!Q_strcasecmp(ext, "jpg"))
		ok = Image_WriteJPEG (name, jpeg_compression_level.value, buffer + buffersize - 3 * glwidth, -glwidth, glheight);
	else if (!Q_strcasecmp(ext, "png"))
		ok = Image_WritePNG (name, png_compression_level.value, buffer + buffersize - 3 * glwidth, -glwidth, glheight);
	else
		ok = Image_WriteTGA (name, buffer, glwidth, glheight);

	free (buffer);

	return ok;
}

/* 
================== 
SCR_ScreenShot_f
================== 
*/  
void SCR_ScreenShot_f (void) 
{
	int	i, success;
	char	name[MAX_OSPATH], ext[4], *sshot_dir = "joequake/shots";

	if (Cmd_Argc() == 2)
	{
		Q_strncpyz (name, Cmd_Argv(1), sizeof(name));
	}
	else if (Cmd_Argc() == 1)
	{
		// find a file name to save it to 
		if (!Q_strcasecmp(scr_sshot_format.string, "jpg") || !Q_strcasecmp(scr_sshot_format.string, "jpeg"))
			Q_strncpyz (ext, "jpg", 4);
		else if (!Q_strcasecmp(scr_sshot_format.string, "png"))
			Q_strncpyz (ext, "png", 4);
		else
			Q_strncpyz (ext, "tga", 4);

		for (i=0 ; i<999 ; i++) 
		{ 
			Q_snprintfz (name, sizeof(name), "TASQuake%03i.%s", i, ext);
			if (Sys_FileTime(va("%s/%s/%s", com_basedir, sshot_dir, name)) == -1)
				break;	// file doesn't exist
		} 

		if (i == 1000)
		{
			Con_Printf ("ERROR: Cannot create more than 1000 screenshots\n");
			return;
		}
	}
	else
	{
		Con_Printf ("Usage: %s [filename]", Cmd_Argv(0));
		return;
	}

	success = SCR_ScreenShot (va("%s/%s", sshot_dir, name));
	Con_Printf ("%s %s\n", success ? "Wrote" : "Couldn't write", name);
} 

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected || cls.signon != SIGNONS)
		return;

// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	scr_fullupdate = 0;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
	scr_fullupdate = 0;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	scr_fullupdate = 0;
	Con_ClearNotify ();
}

//=============================================================================

char		*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	char	*start;
	int	l, j, x, y;

	start = scr_notifystring;

	y = vid.height * 0.35;

	do {
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (vid.width - l*8) / 2;
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);	
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

//=============================================================================

void SCR_TileClear (void)
{
	if (r_refdef.vrect.x > 0)
	{
		// left
		Draw_TileClear (0, 0, r_refdef.vrect.x, vid.height - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width, 0, 
			vid.width - r_refdef.vrect.x + r_refdef.vrect.width, vid.height - sb_lines);
	}
	if (r_refdef.vrect.y > 0)
	{
		// top
		Draw_TileClear (r_refdef.vrect.x, 0, r_refdef.vrect.x + r_refdef.vrect.width, r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x, r_refdef.vrect.y + r_refdef.vrect.height, 
			r_refdef.vrect.width, vid.height - sb_lines - (r_refdef.vrect.height + r_refdef.vrect.y));
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	if (block_drawing)
		return;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
			scr_disabled_for_loading = false;
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet

#ifdef _WIN32
	{	// don't suck up any cpu if minimized
		extern	int	Minimized;

		if (Minimized)
			return;
	}
#endif

	vid.numpages = 2 + gl_triplebuffer.value;

	scr_copytop = 0;
	scr_copyeverything = 0;

	if (oldsbar != cl_sbar.value)
	{
		oldsbar = cl_sbar.value;
		vid.recalc_refdef = true;
	}

	// determine size of refresh window
	if (oldfov != scr_fov.value)
	{
		oldfov = scr_fov.value;
		vid.recalc_refdef = true;
	}

	if (oldscreensize != scr_viewsize.value)
	{
		oldscreensize = scr_viewsize.value;
		vid.recalc_refdef = true;
	}

	if (vid.recalc_refdef)
		SCR_CalcRefdef ();

	if ((v_contrast.value > 1 && !vid_hwgamma_enabled) || gl_clear.value)
		Sbar_Changed ();

// do 3D refresh drawing, and then update the screen
	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	SCR_SetUpToDrawConsole ();

	V_RenderView ();
	
	SCR_SetupAutoID ();
	R_RenderOverlay();

	GL_Set2D();

	// added by joe - IMPORTANT: this _must_ be here so that 
	//			     palette flashes take effect in windowed mode too.
	R_PolyBlend();

	R_Q3DamageDraw();

	// draw any areas not covered by the refresh
	SCR_TileClear();

	if (scr_drawdialog)
	{
		Sbar_Draw();
		Draw_FadeScreen();
		SCR_DrawNotifyString();
		scr_copyeverything = true;
	}
	else if (scr_drawloading)
	{
		SCR_DrawLoading();
		Sbar_Draw();
	}
	else if (cl.intermission == 1 && key_dest == key_game)
	{
		Sbar_IntermissionOverlay();
		SCR_DrawVolume();
		SCR_Draw_TAS_Hud();
	}
	else if (cl.intermission == 2 && key_dest == key_game)
	{
		Sbar_FinaleOverlay();
		SCR_CheckDrawCenterString();
		SCR_DrawVolume();
		SCR_Draw_TAS_Hud();
	}
	else
	{
		Draw_Crosshair();
		SCR_DrawRam();
		SCR_DrawNet();
		SCR_DrawTurtle();
		SCR_DrawPause();
		SCR_DrawAutoID();
		if (nehahra)
			SHOWLMP_drawall();
		SCR_CheckDrawCenterString();
		SCR_DrawClock();
		SCR_DrawFPS();
		SCR_DrawSpeed();
		SCR_DrawStats();
		SCR_DrawVolume();
		SCR_DrawPlaybackStats();
		SCR_Draw_TAS_Hud();
		Sbar_Draw();
		SCR_DrawConsole();
		M_Draw();
	}

	R_BrightenScreen();

	V_UpdatePalette();

#ifdef _WIN32
	Movie_UpdateScreen();
#endif

	GL_EndRendering ();
}
