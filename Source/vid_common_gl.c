/*
Copyright (C) 2002-2003 A Nourai

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
// vid_common_gl.c -- Common code for vid_wgl.c and vid_glx.c

#include "quakedef.h"

#ifdef _WIN32
#define qglGetProcAddress wglGetProcAddress
#else
void *glXGetProcAddress (const GLubyte *procName);
#define qglGetProcAddress glXGetProcAddress
#endif

const	char	*gl_vendor;
const	char	*gl_renderer;
const	char	*gl_version;
const	char	*gl_extensions;

qboolean	gl_mtexable = false;
int			gl_textureunits = 1;

lpMTexFUNC	qglMultiTexCoord2f = NULL;
lpSelTexFUNC qglActiveTexture = NULL;

qboolean	gl_add_ext = false;

float		gldepthmin, gldepthmax;
qboolean	gl_allow_ztrick = true;

float		vid_gamma = 1.0;
byte		vid_gamma_table[256];

unsigned	d_8to24table[256];
unsigned	d_8to24table2[256];

byte		color_white[4] = {255, 255, 255, 0};
byte		color_black[4] = {0, 0, 0, 0};

extern qboolean	fullsbardraw;

qboolean CheckExtension (const char *extension)
{
	char		*where, *terminator;
	const	char	*start;

	if (!gl_extensions && !(gl_extensions = glGetString(GL_EXTENSIONS)))
		return false;

	if (!extension || *extension == 0 || strchr(extension, ' '))
		return false;

	for (start = gl_extensions ; where = strstr(start, extension) ; start = terminator)
	{
		terminator = where + strlen(extension);
		if ((where == start || *(where - 1) == ' ') && (*terminator == 0 || *terminator == ' '))
			return true;
	}

	return false;
}

void CheckMultiTextureExtensions (void)
{
	if (!COM_CheckParm("-nomtex") && CheckExtension("GL_ARB_multitexture"))
	{
		if (strstr(gl_renderer, "Savage"))
			return;
		qglMultiTexCoord2f = (void *)qglGetProcAddress ("glMultiTexCoord2fARB");
		qglActiveTexture = (void *)qglGetProcAddress ("glActiveTextureARB");
		if (!qglMultiTexCoord2f || !qglActiveTexture)
			return;
		Con_Printf ("Multitexture extensions found\n");
		gl_mtexable = true;
	}

	glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &gl_textureunits);
	gl_textureunits = min(gl_textureunits, 4);

	if (COM_CheckParm("-maxtmu2") || !strcmp(gl_vendor, "ATI Technologies Inc."))
		gl_textureunits = min(gl_textureunits, 2);

	if (gl_textureunits < 2)
		gl_mtexable = false;

	if (!gl_mtexable)
		gl_textureunits = 1;
	else
		Con_Printf ("Enabled %i texture units on hardware\n", gl_textureunits);
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);
	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	if (COM_CheckParm("-gl_ext"))
		Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

	if (!Q_strncasecmp((char *)gl_renderer, "PowerVR", 7))
		fullsbardraw = true;

	glClearColor (0, 0, 0, 0);
	glCullFace (GL_FRONT);
	glEnable (GL_TEXTURE_2D);

	glEnable (GL_ALPHA_TEST);
	glAlphaFunc (GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	CheckMultiTextureExtensions ();

	gl_add_ext = CheckExtension ("GL_ARB_texture_env_add");
}

void Check_Gamma (unsigned char *pal)
{
	int		i;
	float	inf;
	unsigned char palette[768];

	if ((i = COM_CheckParm("-gamma")) && i + 1 < com_argc)
		vid_gamma = bound(0.3, Q_atof(com_argv[i+1]), 1);
	else
		vid_gamma = 1;

	Cvar_SetDefault (&v_gamma, vid_gamma);

	if (vid_gamma != 1)
	{
		for (i = 0 ; i < 256 ; i++)
		{
			inf = min(255 * pow((i + 0.5) / 255.5, vid_gamma) + 0.5, 255);
			vid_gamma_table[i] = inf;
		}
	}
	else
	{
		for (i = 0 ; i < 256 ; i++)
			vid_gamma_table[i] = i;
	}

	for (i = 0 ; i < 768 ; i++)
		palette[i] = vid_gamma_table[pal[i]];

	memcpy (pal, palette, sizeof(palette));
}

void VID_SetPalette (unsigned char *palette)
{
	int		i;
	byte	*pal;
	unsigned r, g, b, *table;

// 8 8 8 encoding
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		*table++ = (255<<24) + (r<<0) + (g<<8) + (b<<16);
	}
	d_8to24table[255] = 0;	// 255 is transparent

// Tonik: create a brighter palette for bmodel textures
	pal = palette;
	table = d_8to24table2;
	for (i = 0 ; i < 256 ; i++)
	{
		r = min(pal[0] * (2.0 / 1.5), 255);
		g = min(pal[1] * (2.0 / 1.5), 255);
		b = min(pal[2] * (2.0 / 1.5), 255);
		pal += 3;
		*table++ = (255<<24) + (r<<0) + (g<<8) + (b<<16);
	}
	d_8to24table2[255] = 0;	// 255 is transparent
}
