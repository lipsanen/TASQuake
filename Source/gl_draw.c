/*
Copyright (C) 1996-1997, 2016 Id Software, Inc, CRASH FORT.

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
// gl_draw.c -- this is the only file outside the refresh that touches the vid buffer

#include "quakedef.h"

extern	unsigned d_8to24table2[256];

int		texture_extension_number = 1;

extern qboolean OnChange_gl_picmip (cvar_t *var, char *string);
cvar_t	gl_picmip = {"gl_picmip", "0", 0, OnChange_gl_picmip};

cvar_t	gl_conalpha = {"gl_conalpha", "0.8"};

qboolean OnChange_gl_max_size (cvar_t *var, char *string);
cvar_t	gl_max_size = {"gl_max_size", "1024", 0, OnChange_gl_max_size};

qboolean OnChange_gl_texturemode (cvar_t *var, char *string);
cvar_t	gl_texturemode = {"gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", 0, OnChange_gl_texturemode};

cvar_t	gl_externaltextures_world = {"gl_externaltextures_world", "1"};
cvar_t	gl_externaltextures_bmodels = {"gl_externaltextures_bmodels", "1"};

qboolean OnChange_gl_crosshairimage (cvar_t *var, char *string);
cvar_t	gl_crosshairimage = {"crosshairimage", "", 0, OnChange_gl_crosshairimage};

qboolean OnChange_gl_consolefont (cvar_t *var, char *string);
cvar_t	gl_consolefont = {"gl_consolefont", "original", 0, OnChange_gl_consolefont};

cvar_t	gl_crosshairalpha = {"crosshairalpha", "1"};

qboolean OnChange_gl_smoothfont (cvar_t *var, char *string);
cvar_t gl_smoothfont = {"gl_smoothfont", "0", 0, OnChange_gl_smoothfont};

static qboolean no24bit;

byte	*draw_chars;			// 8*8 graphic characters
mpic_t	*draw_disc;
mpic_t	*draw_backtile;

int		translate_texture;
int		char_texture;

extern	cvar_t	crosshair, cl_crossx, cl_crossy, crosshaircolor, crosshairsize;

mpic_t	crosshairpic;
static qboolean	crosshairimage_loaded = false;

int GL_LoadPicTexture (char *name, mpic_t *pic, byte *data);

mpic_t	conback_data;
mpic_t	*conback = &conback_data;

int		gl_max_size_default;
int		gl_lightmap_format = 3, gl_solid_format = 3, gl_alpha_format = 4;

static	int	gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
static	int	gl_filter_max = GL_LINEAR;


extern	byte	vid_gamma_table[256];
extern	float	vid_gamma;

#define	NUMCROSSHAIRS	5
int		crosshairtextures[NUMCROSSHAIRS];

static byte crosshairdata[NUMCROSSHAIRS][64] = {
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
	0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff,
	0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,

	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 
	0xfe, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0xff, 
	0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

typedef struct
{
	int		texnum;
	char	identifier[MAX_QPATH];
	char	*pathname;
	int		width, height;
	int		scaled_width, scaled_height;
	int		texmode;
	unsigned crc;
	int		bpp;
} gltexture_t;

gltexture_t	gltextures[MAX_GLTEXTURES];
int		numgltextures;

int		currenttexture = -1;		// to avoid unnecessary texture sets

void GL_Bind (int texnum)
{
	if(isSimulator)
		return;

	if (currenttexture == texnum)
		return;

	currenttexture = texnum;
	Q_glBindTexture (GL_TEXTURE_2D, texnum);
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar objects into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

// some cards have low quality of alpha pics, so load the pics
// without transparent pixels into a different scrap block.
// scrap 0 is solid pics, 1 is transparent
#define	MAX_SCRAPS	2
#define	BLOCK_WIDTH	256
#define	BLOCK_HEIGHT	256

int		scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte	scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
int		scrap_dirty = 0;	// bit mask
int		scrap_texnum;

// returns false if allocation failed
qboolean Scrap_AllocBlock (int scrapnum, int w, int h, int *x, int *y)
{
	int	i, j, best, best2;

	best = BLOCK_HEIGHT;

	for (i = 0 ; i < BLOCK_WIDTH - w ; i++)
	{
		best2 = 0;
		
		for (j = 0 ; j < w ; j++)
		{
			if (scrap_allocated[scrapnum][i+j] >= best)
				break;
			if (scrap_allocated[scrapnum][i+j] > best2)
				best2 = scrap_allocated[scrapnum][i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		scrap_allocated[scrapnum][*x+i] = best + h;

	scrap_dirty |= (1 << scrapnum);

	return true;
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	int	i;

	scrap_uploads++;
	for (i = 0 ; i < 2 ; i++)
	{
		if (!(scrap_dirty & (1 << i)))
			continue;
		scrap_dirty &= ~(1 << i);
		GL_Bind (scrap_texnum + i);
		GL_Upload8 (scrap_texels[i], BLOCK_WIDTH, BLOCK_HEIGHT, TEX_ALPHA);
	}
}

//=============================================================================
/* Support Routines */

typedef struct cachepic_s
{
	char	name[MAX_QPATH];
	mpic_t	pic;
} cachepic_t;

#define	MAX_CACHED_PICS		128
cachepic_t cachepics[MAX_CACHED_PICS];
int		numcachepics;

byte	menuplyr_pixels[4096];

int		pic_texels;
int		pic_count;

mpic_t *Draw_PicFromWad (char *name)
{
	if(isSimulator)
		return NULL;

	qpic_t	*p;
	mpic_t	*pic, *pic_24bit;

	p = W_GetLumpName (name);
	pic = (mpic_t *)p;

	if ((pic_24bit = GL_LoadPicImage(va("textures/wad/%s", name), name, 0, 0, TEX_ALPHA)) || 
	    (pic_24bit = GL_LoadPicImage(va("gfx/%s", name), name, 0, 0, TEX_ALPHA)))
	{
		memcpy (&pic->texnum, &pic_24bit->texnum, sizeof(mpic_t) - 8);
		return pic;
	}

	// load little ones into the scrap
	if (p->width < 64 && p->height < 64)
	{
		int	x, y, i, j, k, texnum;

		texnum = memchr (p->data, 255, p->width * p->height) != NULL;
		if (!Scrap_AllocBlock(texnum, p->width, p->height, &x, &y))
		{
			GL_LoadPicTexture (name, pic, p->data);
			return pic;
		}
		k = 0;
		for (i = 0 ; i < p->height ; i++)
			for (j = 0 ; j < p->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH+x+j] = p->data[k];
		texnum += scrap_texnum;
		pic->texnum = texnum;
		pic->sl = (x + 0.01) / (float)BLOCK_WIDTH;
		pic->sh = (x + p->width - 0.01) / (float)BLOCK_WIDTH;
		pic->tl = (y + 0.01) / (float)BLOCK_WIDTH;
		pic->th = (y + p->height - 0.01) / (float)BLOCK_WIDTH;

		pic_count++;
		pic_texels += p->width * p->height;
	}
	else
	{
		GL_LoadPicTexture (name, pic, p->data);
	}

	return pic;
}

/*
================
Draw_CachePic
================
*/
mpic_t *Draw_CachePic (char *path)
{
	int			i;
	cachepic_t	*pic;
	qpic_t		*dat;
	mpic_t		*pic_24bit;

	for (pic = cachepics, i = 0 ; i < numcachepics ; pic++, i++)
		if (!strcmp(path, pic->name))
			return &pic->pic;

	if (numcachepics == MAX_CACHED_PICS)
		Sys_Error ("numcachepics == MAX_CACHED_PICS");
	numcachepics++;
	Q_strncpyz (pic->name, path, sizeof(pic->name));

	// load the pic from disk
	if (!(dat = (qpic_t *)COM_LoadTempFile(path)))
		Sys_Error ("Draw_CachePic: failed to load %s", path);
	SwapPic (dat);

	// HACK HACK HACK --- we need to keep the bytes for
	// the translatable player picture just for the menu
	// configuration dialog
	if (!strcmp(path, "gfx/menuplyr.lmp"))
		memcpy (menuplyr_pixels, dat->data, dat->width * dat->height);

	pic->pic.width = dat->width;
	pic->pic.height = dat->height;

	if ((pic_24bit = GL_LoadPicImage(path, NULL, 0, 0, TEX_ALPHA)))
		memcpy (&pic->pic.texnum, &pic_24bit->texnum, sizeof(mpic_t) - 8);
	else
		GL_LoadPicTexture (path, &pic->pic, dat->data);

	return &pic->pic;
}

void Draw_CharToConback (int num, byte *dest)
{
	int		row, col, drawline, x;
	byte	*source;

	row = num >> 4;
	col = num & 15;
	source = draw_chars + (row<<10) + (col<<3);

	drawline = 8;

	while (drawline--)
	{
		for (x = 0 ; x < 8 ; x++)
			if (source[x] != 255)
				dest[x] = 0x60 + source[x];
		source += 128;
		dest += 320;
	}
}

void Draw_InitConback (void)
{
	int		start;
	qpic_t	*cb;
	mpic_t	*pic_24bit;

	start = Hunk_LowMark ();

	if (!(cb = (qpic_t *)COM_LoadHunkFile("gfx/conback.lmp")))
		Sys_Error ("Couldn't load gfx/conback.lmp");
	SwapPic (cb);

	if (cb->width != 320 || cb->height != 200)
		Sys_Error ("Draw_InitConback: conback.lmp size is not 320x200");

	if ((pic_24bit = GL_LoadPicImage("gfx/conback", "conback", 0, 0, 0)))
	{
		memcpy (&conback->texnum, &pic_24bit->texnum, sizeof(mpic_t) - 8);
	}
	else
	{
		conback->width = cb->width;
		conback->height = cb->height;
		GL_LoadPicTexture ("conback", conback, cb->data);
	}
	conback->width = vid.conwidth;
	conback->height = vid.conheight;

	// free loaded console
	Hunk_FreeToLowMark (start);
}

#define Q_ROUND_POWER2(in, out)				\
	{						\
		for (out = 1 ; out < in ; out <<= 1)	\
			;				\
	}

qboolean OnChange_gl_max_size (cvar_t *var, char *string)
{
	int	i;
	float	newval = Q_atof(string);

	if (newval > gl_max_size_default)
	{
		Con_Printf ("Your hardware doesn`t support texture sizes bigger than %dx%d\n", gl_max_size_default, gl_max_size_default);
		return true;
	}

	Q_ROUND_POWER2(newval, i);

	if (i != newval)
	{
		Con_Printf ("Valid values for %s are powers of 2 only\n", var->name);
		return true;
	}

	return false;
}

typedef struct
{
	char	*name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define GLMODE_NUMMODES	(sizeof(modes) / sizeof(glmode_t))

qboolean OnChange_gl_texturemode (cvar_t *var, char *string)
{
	int		i;
	gltexture_t	*glt;

	for (i = 0 ; i < GLMODE_NUMMODES ; i++)
	{
		if (!Q_strcasecmp(modes[i].name, string))
			break;
	}

	if (i == GLMODE_NUMMODES)
	{
		Con_Printf ("bad filter name: %s\n", string);
		return true;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, glt = gltextures ; i < numgltextures ; i++, glt++)
	{
		if (glt->texmode & TEX_MIPMAP)
		{
			GL_Bind (glt->texnum);
			Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}

	return false;
}

static int Draw_LoadCharset (char *name)
{
	int	texnum;

	if (!Q_strcasecmp(name, "original"))
	{
		int		i;
		char	buf[128*256], *src, *dest;

		// Convert the 128*128 conchars texture to 128*256 leaving empty space between 
		// rows so that chars don't stumble on each other because of texture smoothing.
		// This hack costs us 64K of GL texture memory
		memset (buf, 255, sizeof(buf));
		src = draw_chars;
		dest = buf;
		for (i = 0 ; i < 16 ; i++)
		{
			memcpy (dest, src, 128 * 8);
			src += 128 * 8;
			dest += 128 * 8 * 2;
		}

		char_texture = GL_LoadTexture ("pic:charset", 128, 256, buf, TEX_ALPHA, 1);
		goto done;
	}

	if ((texnum = GL_LoadCharsetImage(va("textures/charsets/%s", name), "pic:charset")))
	{
		char_texture = texnum;
		goto done;
	}

	Con_Printf ("Couldn't load charset \"%s\"\n", name);
	return 1;

done:
	if (!gl_smoothfont.value)
	{
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	return 0;
}

qboolean OnChange_gl_consolefont (cvar_t *var, char *string)
{
	return Draw_LoadCharset (string);
}

void Draw_LoadCharset_f (void)
{
	switch (Cmd_Argc())
	{
	case 1:
		Con_Printf ("Current charset is \"%s\"\n", gl_consolefont.string);
		break;

	case 2:
		Cvar_Set (&gl_consolefont, Cmd_Argv(1));
		break;

	default:
		Con_Printf ("Usage: %s <charset>\n", Cmd_Argv(0));
	}
}

void Draw_InitCharset (void)
{
	int	i;

	draw_chars = W_GetLumpName ("conchars");
	for (i = 0 ; i < 256*64 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	Draw_LoadCharset (gl_consolefont.string);

	if (!char_texture)
		Cvar_Set (&gl_consolefont, "original");

	if (!char_texture)	
		Sys_Error ("Draw_InitCharset: Couldn't load charset");
}

qboolean OnChange_gl_smoothfont (cvar_t *var, char *string)
{
	float	newval;

	newval = Q_atof (string);
	if (!newval == !gl_smoothfont.value || !char_texture)
		return false;

	GL_Bind (char_texture);

	if (newval)
	{
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	return false;
}

qboolean OnChange_gl_crosshairimage (cvar_t *var, char *string)
{
	mpic_t	*pic;

	if (!string[0])
	{
		crosshairimage_loaded = false;
		return false;
	}

	if (!(pic = GL_LoadPicImage(va("crosshairs/%s", string), "crosshair", 0, 0, TEX_ALPHA)))
	{
		crosshairimage_loaded = false;
		Con_Printf ("Couldn't load crosshair \"%s\"\n", string);
		return false;
	}

	Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	crosshairpic = *pic;
	crosshairimage_loaded = true;

	return false;
}

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int	i;

	Cmd_AddCommand ("loadcharset", Draw_LoadCharset_f);

	Cvar_Register (&gl_max_size);
	Cvar_Register (&gl_picmip);
	Cvar_Register (&gl_conalpha);
	Cvar_Register (&gl_smoothfont);
	Cvar_Register (&gl_consolefont);	
	Cvar_Register (&gl_crosshairalpha);
	Cvar_Register (&gl_crosshairimage);
	Cvar_Register (&gl_texturemode);
	Cvar_Register (&gl_externaltextures_world);
	Cvar_Register (&gl_externaltextures_bmodels);

	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_max_size_default);
	Cvar_SetDefault (&gl_max_size, gl_max_size_default);

	no24bit = COM_CheckParm("-no24bit") ? true : false;

	// 3dfx can only handle 256 wide textures
	if (!isSimulator && !Q_strncasecmp((char *)gl_renderer, "3dfx", 4) || strstr((char *)gl_renderer, "Glide"))
		Cvar_SetValue (&gl_max_size, 256);

	Draw_InitCharset ();
	Draw_InitConback ();

	// save a texture slot for translated picture
	translate_texture = texture_extension_number++;

	// save slots for scraps
	scrap_texnum = texture_extension_number;
	texture_extension_number += MAX_SCRAPS;

	// Load the crosshair pics
	for (i = 0 ; i < NUMCROSSHAIRS ; i++)
	{
		crosshairtextures[i] = GL_LoadTexture ("", 8, 8, crosshairdata[i], TEX_ALPHA, 1);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// get the other pics we need
	draw_disc = Draw_PicFromWad ("disc");
	draw_backtile = Draw_PicFromWad ("backtile");
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, int num)
{
	int	row, col;
	float	frow, fcol;

	if (y <= -8)
		return;		// totally off screen

	if (num == 32)
		return;		// space

	num &= 255;
	
	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;

	GL_Bind (char_texture);

	Q_glBegin (GL_QUADS);
	Q_glTexCoord2f (fcol, frow);
	Q_glVertex2f (x, y);
	Q_glTexCoord2f (fcol + 0.0625, frow);
	Q_glVertex2f (x+8, y);
	Q_glTexCoord2f (fcol + 0.0625, frow + 0.03125);
	Q_glVertex2f (x+8, y+8);
	Q_glTexCoord2f (fcol, frow + 0.03125);
	Q_glVertex2f (x, y+8);
	Q_glEnd ();
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, char *str)
{
	float	frow, fcol;
	int	num;

	if (y <= -8)
		return;		// totally off screen

	if (!*str)
		return;

	GL_Bind (char_texture);

	Q_glBegin (GL_QUADS);

	while (*str)		// stop rendering when out of characters
	{
		if ((num = *str++) != 32)	// skip spaces
		{
			frow = (float)(num >> 4) * 0.0625;
			fcol = (float)(num & 15) * 0.0625;
			Q_glTexCoord2f (fcol, frow);
			Q_glVertex2f (x, y);
			Q_glTexCoord2f (fcol + 0.0625, frow);
			Q_glVertex2f (x+8, y);
			Q_glTexCoord2f (fcol + 0.0625, frow + 0.03125);
			Q_glVertex2f (x+8, y+8);
			Q_glTexCoord2f (fcol, frow + 0.03125);
			Q_glVertex2f (x, y+8);
		}
		x += 8;
	}

	Q_glEnd ();
}

/*
================
Draw_Alt_String
================
*/
void Draw_Alt_String (int x, int y, char *str)
{
	float	frow, fcol;
	int	num;

	if (y <= -8)
		return;		// totally off screen

	if (!*str)
		return;

	GL_Bind (char_texture);

	Q_glBegin (GL_QUADS);

	while (*str)		// stop rendering when out of characters
	{
		if ((num = *str++|0x80) != (32|0x80))	// skip spaces
		{
			frow = (float)(num >> 4) * 0.0625;
			fcol = (float)(num & 15) * 0.0625;
			Q_glTexCoord2f (fcol, frow);
			Q_glVertex2f (x, y);
			Q_glTexCoord2f (fcol + 0.0625, frow);
			Q_glVertex2f (x+8, y);
			Q_glTexCoord2f (fcol + 0.0625, frow + 0.03125);
			Q_glVertex2f (x+8, y+8);
			Q_glTexCoord2f (fcol, frow + 0.03125);
			Q_glVertex2f (x, y+8);
		}
		x += 8;
	}

	Q_glEnd ();
}

byte *StringToRGB (char *s)
{
	byte		*col;
	static	byte	rgb[4];

	Cmd_TokenizeString (s);
	if (Cmd_Argc() == 3)
	{
		rgb[0] = (byte)Q_atoi(Cmd_Argv(0));
		rgb[1] = (byte)Q_atoi(Cmd_Argv(1));
		rgb[2] = (byte)Q_atoi(Cmd_Argv(2));
	}
	else
	{
		col = (byte *)&d_8to24table[(byte)Q_atoi(s)];
		rgb[0] = col[0];
		rgb[1] = col[1];
		rgb[2] = col[2];
	}
	rgb[3] = 255;

	return rgb;
}

/*
================
Draw_Crosshair		-- joe, from FuhQuake
================
*/
void Draw_Crosshair (void)
{
	float		x, y, ofs1, ofs2, sh, th, sl, tl;
	byte		*col;
	extern vrect_t	scr_vrect;

	if ((crosshair.value >= 2 && crosshair.value <= NUMCROSSHAIRS + 1) || crosshairimage_loaded)
	{
		x = scr_vrect.x + scr_vrect.width / 2 + cl_crossx.value; 
		y = scr_vrect.y + scr_vrect.height / 2 + cl_crossy.value;

		if (!gl_crosshairalpha.value)
			return;

		Q_glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		col = StringToRGB (crosshaircolor.string);

		if (gl_crosshairalpha.value)
		{
			Q_glDisable (GL_ALPHA_TEST);
			Q_glEnable (GL_BLEND);
			col[3] = bound(0, gl_crosshairalpha.value, 1) * 255;
			Q_glColor4ubv (col);
		}
		else
		{
			Q_glColor3ubv (col);
		}

		if (crosshairimage_loaded)
		{
			GL_Bind (crosshairpic.texnum);
			ofs1 = 4 - 4.0 / crosshairpic.width;
			ofs2 = 4 + 4.0 / crosshairpic.width;
			sh = crosshairpic.sh;
			sl = crosshairpic.sl;
			th = crosshairpic.th;
			tl = crosshairpic.tl;
		}
		else
		{
			GL_Bind (crosshairtextures[(int)crosshair.value-2]);
			ofs1 = 3.5;
			ofs2 = 4.5;
			tl = sl = 0;
			sh = th = 1;
		}

		ofs1 *= (vid.width / 320) * bound(0, crosshairsize.value, 20);
		ofs2 *= (vid.width / 320) * bound(0, crosshairsize.value, 20);

		Q_glBegin (GL_QUADS);
		Q_glTexCoord2f (sl, tl);
		Q_glVertex2f (x - ofs1, y - ofs1);
		Q_glTexCoord2f (sh, tl);
		Q_glVertex2f (x + ofs2, y - ofs1);
		Q_glTexCoord2f (sh, th);
		Q_glVertex2f (x + ofs2, y + ofs2);
		Q_glTexCoord2f (sl, th);
		Q_glVertex2f (x - ofs1, y + ofs2);
		Q_glEnd ();

		if (gl_crosshairalpha.value)
		{
			Q_glDisable (GL_BLEND);
			Q_glEnable (GL_ALPHA_TEST);
		}

		Q_glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		Q_glColor3ubv (color_white);
	}
	else if (crosshair.value)
	{
		Draw_Character (scr_vrect.x + scr_vrect.width / 2 - 4 + cl_crossx.value, scr_vrect.y + scr_vrect.height / 2 - 4 + cl_crossy.value, '+');
	}
}

/*
================
Draw_TextBox
================
*/
void Draw_TextBox (int x, int y, int width, int lines)
{
	mpic_t	*p;
	int	cx, cy, n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	Draw_TransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		Draw_TransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	Draw_TransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		Draw_TransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			Draw_TransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		Draw_TransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	Draw_TransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		Draw_TransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	Draw_TransPic (cx, cy+8, p);
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar (char num)
{
}

/*
=============
Draw_AlphaPic
=============
*/
void Draw_AlphaPic (int x, int y, mpic_t *pic, float alpha)
{
	if (scrap_dirty)
		Scrap_Upload ();
	Q_glDisable (GL_ALPHA_TEST);
	Q_glEnable (GL_BLEND);
	Q_glCullFace (GL_FRONT);
	Q_glColor4f (1, 1, 1, alpha);
	GL_Bind (pic->texnum);
	Q_glBegin (GL_QUADS);
	Q_glTexCoord2f (pic->sl, pic->tl);
	Q_glVertex2f (x, y);
	Q_glTexCoord2f (pic->sh, pic->tl);
	Q_glVertex2f (x+pic->width, y);
	Q_glTexCoord2f (pic->sh, pic->th);
	Q_glVertex2f (x+pic->width, y+pic->height);
	Q_glTexCoord2f (pic->sl, pic->th);
	Q_glVertex2f (x, y+pic->height);
	Q_glEnd ();
	Q_glColor3ubv (color_white);
	Q_glEnable (GL_ALPHA_TEST);
	Q_glDisable (GL_BLEND);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, mpic_t *pic)
{
	if(isSimulator)
		return;

	if (scrap_dirty)
		Scrap_Upload ();
	GL_Bind (pic->texnum);
	Q_glBegin (GL_QUADS);
	Q_glTexCoord2f (pic->sl, pic->tl);
	Q_glVertex2f (x, y);
	Q_glTexCoord2f (pic->sh, pic->tl);
	Q_glVertex2f (x+pic->width, y);
	Q_glTexCoord2f (pic->sh, pic->th);
	Q_glVertex2f (x+pic->width, y+pic->height);
	Q_glTexCoord2f (pic->sl, pic->th);
	Q_glVertex2f (x, y+pic->height);
	Q_glEnd ();
}

void Draw_SubPic (int x, int y, mpic_t *pic, int srcx, int srcy, int width, int height)
{
	if(isSimulator)
		return;

	float	newsl, newtl, newsh, newth, oldglwidth, oldglheight;

	if (scrap_dirty)
		Scrap_Upload ();
	
	oldglwidth = pic->sh - pic->sl;
	oldglheight = pic->th - pic->tl;

	newsl = pic->sl + (srcx * oldglwidth) / pic->width;
	newsh = newsl + (width * oldglwidth) / pic->width;

	newtl = pic->tl + (srcy * oldglheight) / pic->height;
	newth = newtl + (height * oldglheight) / pic->height;
	
	GL_Bind (pic->texnum);
	Q_glBegin (GL_QUADS);
	Q_glTexCoord2f (newsl, newtl);
	Q_glVertex2f (x, y);
	Q_glTexCoord2f (newsh, newtl);
	Q_glVertex2f (x+width, y);
	Q_glTexCoord2f (newsh, newth);
	Q_glVertex2f (x+width, y+height);
	Q_glTexCoord2f (newsl, newth);
	Q_glVertex2f (x, y+height);
	Q_glEnd ();
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic (int x, int y, mpic_t *pic)
{
	if(isSimulator)
		return;

	if (x < 0 || (unsigned)(x + pic->width) > vid.width || y < 0 || (unsigned)(y + pic->height) > vid.height)
		Sys_Error ("Draw_TransPic: bad coordinates");
		
	Draw_Pic (x, y, pic);
}

/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
void Draw_TransPicTranslate (int x, int y, mpic_t *pic, byte *translation)
{
	if(isSimulator)
		return;
		
	int		v, u, c, p;
	unsigned	trans[64*64], *dest;
	byte		*src;

	GL_Bind (translate_texture);

	c = pic->width * pic->height;

	dest = trans;
	for (v=0 ; v<64 ; v++, dest += 64)
	{
		src = &menuplyr_pixels[((v*pic->height)>>6)*pic->width];
		for (u=0 ; u<64 ; u++)
		{
			p = src[(u*pic->width)>>6];
			dest[u] = (p == 255) ? p : d_8to24table[translation[p]];
		}
	}

	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);

	Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Q_glBegin (GL_QUADS);
	Q_glTexCoord2f (0, 0);
	Q_glVertex2f (x, y);
	Q_glTexCoord2f (1, 0);
	Q_glVertex2f (x+pic->width, y);
	Q_glTexCoord2f (1, 1);
	Q_glVertex2f (x+pic->width, y+pic->height);
	Q_glTexCoord2f (0, 1);
	Q_glVertex2f (x, y+pic->height);
	Q_glEnd ();
}

/*
================
Draw_ConsoleBackground
================
*/
void Draw_ConsoleBackground (int lines)
{
	char	ver[80];

	if (!gl_conalpha.value && lines < vid.height)
		goto end;

	if (lines == vid.height)
		Draw_Pic (0, lines - vid.height, conback);
	else
		Draw_AlphaPic (0, lines - vid.height, conback, bound(0, gl_conalpha.value, 1));

end:
	sprintf (ver, "TASQuake %s", TASQUAKE_VERSION);
	Draw_Alt_String (vid.conwidth - strlen(ver) * 8 - 8, lines - 10, ver);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	GL_Bind (draw_backtile->texnum);
	Q_glBegin (GL_QUADS);
	Q_glTexCoord2f (x/64.0, y/64.0);
	Q_glVertex2f (x, y);
	Q_glTexCoord2f ((x+w)/64.0, y/64.0);
	Q_glVertex2f (x+w, y);
	Q_glTexCoord2f ((x+w)/64.0, (y+h)/64.0);
	Q_glVertex2f (x+w, y+h);
	Q_glTexCoord2f (x/64.0, (y+h)/64.0);
	Q_glVertex2f (x, y+h);
	Q_glEnd ();
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	Q_glDisable (GL_TEXTURE_2D);
	Q_glColor3f (host_basepal[c*3] / 255.0, host_basepal[c*3+1] / 255.0, host_basepal[c*3+2] / 255.0);

	Q_glBegin (GL_QUADS);
	Q_glVertex2f (x, y);
	Q_glVertex2f (x+w, y);
	Q_glVertex2f (x+w, y+h);
	Q_glVertex2f (x, y+h);
	Q_glEnd ();

	Q_glEnable (GL_TEXTURE_2D);
	Q_glColor3ubv (color_white);
}

//=============================================================================

/*
================
Draw_FadeScreen
================
*/
void Draw_FadeScreen (void)
{
	Q_glEnable (GL_BLEND);
	Q_glDisable (GL_TEXTURE_2D);
	Q_glColor4f (0, 0, 0, 0.7);

	Q_glBegin (GL_QUADS);
	Q_glVertex2f (0,0);
	Q_glVertex2f (vid.width, 0);
	Q_glVertex2f (vid.width, vid.height);
	Q_glVertex2f (0, vid.height);
	Q_glEnd ();

	Q_glColor3ubv (color_white);
	Q_glEnable (GL_TEXTURE_2D);
	Q_glDisable (GL_BLEND);

	Sbar_Changed ();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	if (!draw_disc)
		return;

	Q_glDrawBuffer  (GL_FRONT);
	Draw_Pic (vid.width - 24, 0, draw_disc);
	Q_glDrawBuffer  (GL_BACK);
}

/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}

/*
	CRASH FORT
*/
extern nqvideo_t nqvideo;
void NQ_CalculateVirtualDisplaySize(void);

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D (void)
{
	NQ_CalculateVirtualDisplaySize();

	glViewport (glx, gly, glwidth, glheight);

	Q_glMatrixMode (GL_PROJECTION);
	Q_glLoadIdentity ();
	Q_glOrtho (0, vid.width, vid.height, 0, -99999, 99999);

	Q_glMatrixMode (GL_MODELVIEW);
	Q_glLoadIdentity ();

	Q_glDisable (GL_DEPTH_TEST);
	Q_glDisable (GL_CULL_FACE);
	Q_glDisable (GL_BLEND);
	Q_glEnable (GL_ALPHA_TEST);

	Q_glColor3ubv (color_white);
}

//=============================================================================

/*
================
ResampleTextureLerpLine
================
*/
void ResampleTextureLerpLine (byte *in, byte *out, int inwidth, int outwidth)
{
	int	j, xi, oldx = 0, f, fstep, endx = (inwidth - 1), lerp;

	fstep = (inwidth << 16) / outwidth;
	for (j = 0, f = 0 ; j < outwidth ; j++, f += fstep)
	{
		xi = f >> 16;
		if (xi != oldx)
		{
			in += (xi - oldx) * 4;
			oldx = xi;
		}
		if (xi < endx)
		{
			lerp = f & 0xFFFF;
			*out++ = (byte)((((in[4] - in[0]) * lerp) >> 16) + in[0]);
			*out++ = (byte)((((in[5] - in[1]) * lerp) >> 16) + in[1]);
			*out++ = (byte)((((in[6] - in[2]) * lerp) >> 16) + in[2]);
			*out++ = (byte)((((in[7] - in[3]) * lerp) >> 16) + in[3]);
		}
		else	// last pixel of the line has no pixel to lerp to
		{
			*out++ = in[0];
			*out++ = in[1];
			*out++ = in[2];
			*out++ = in[3];
		}
	}
}

/*
================
ResampleTexture
================
*/
void ResampleTexture (unsigned *indata, int inwidth, int inheight, unsigned *outdata, int outwidth, int outheight, qboolean quality)
{
	if (quality)
	{
		int		i, j, yi, oldy, f, fstep, endy = (inheight - 1), lerp;
		int		inwidth4 = inwidth * 4, outwidth4 = outwidth * 4;
		byte	*inrow, *out, *row1, *row2, *memalloc;

		out = (byte *)outdata;
		fstep = (inheight << 16) / outheight;

		memalloc = Q_malloc (2 * outwidth4);
		row1 = memalloc;
		row2 = memalloc + outwidth4;
		inrow = (byte *)indata;
		oldy = 0;
		ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);
		ResampleTextureLerpLine (inrow + inwidth4, row2, inwidth, outwidth);
		for (i = 0, f = 0 ; i < outheight ; i++, f += fstep)
		{
			yi = f >> 16;
			if (yi < endy)
			{
				lerp = f & 0xFFFF;
				if (yi != oldy)
				{
					inrow = (byte *)indata + inwidth4 * yi;
					if (yi == oldy + 1)
						memcpy (row1, row2, outwidth4);
					else
						ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);
					ResampleTextureLerpLine (inrow + inwidth4, row2, inwidth, outwidth);
					oldy = yi;
				}
				for (j = outwidth ; j ; j--)
				{
					out[0] = (byte)((((row2[0] - row1[0]) * lerp) >> 16) + row1[0]);
					out[1] = (byte)((((row2[1] - row1[1]) * lerp) >> 16) + row1[1]);
					out[2] = (byte)((((row2[2] - row1[2]) * lerp) >> 16) + row1[2]);
					out[3] = (byte)((((row2[3] - row1[3]) * lerp) >> 16) + row1[3]);
					out += 4;
					row1 += 4;
					row2 += 4;
				}
				row1 -= outwidth4;
				row2 -= outwidth4;
			}
			else
			{
				if (yi != oldy)
				{
					inrow = (byte *)indata + inwidth4 * yi;
					if (yi == oldy + 1)
						memcpy(row1, row2, outwidth4);
					else
						ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);
					oldy = yi;
				}
				memcpy (out, row1, outwidth4);
			}
		}
		free (memalloc);
	}
	else
	{
		int		i, j;
		unsigned int frac, fracstep, *inrow, *out;

		out = outdata;

		fracstep = inwidth * 0x10000 / outwidth;
		for (i = 0 ; i < outheight ; i++)
		{
			inrow = (int *)indata + inwidth * (i * inheight / outheight);
			frac = fracstep >> 1;
			j = outwidth - 4;
			while (j >= 0)
			{
				out[0] = inrow[frac >> 16]; frac += fracstep;
				out[1] = inrow[frac >> 16]; frac += fracstep;
				out[2] = inrow[frac >> 16]; frac += fracstep;
				out[3] = inrow[frac >> 16]; frac += fracstep;
				out += 4;
				j -= 4;
			}
			if (j & 2)
			{
				out[0] = inrow[frac >> 16]; frac += fracstep;
				out[1] = inrow[frac >> 16]; frac += fracstep;
				out += 2;
			}
			if (j & 1)
			{
				out[0] = inrow[frac >> 16]; frac += fracstep;
				out += 1;
			}
		}
	}
}

/*
================
MipMap

Operates in place, quartering the size of the texture
================
*/
void MipMap (byte *in, int *width, int *height)
{
	int		i, j, nextrow;
	byte	*out;

	nextrow = *width << 2;

	out = in;
	if (*width > 1)
	{
		*width >>= 1;
		if (*height > 1)
		{
			*height >>= 1;
			for (i = 0 ; i < *height ; i++, in += nextrow)
			{
				for (j = 0 ; j < *width ; j++, out += 4, in += 8)
				{
					out[0] = (in[0] + in[4] + in[nextrow+0] + in[nextrow+4]) >> 2;
					out[1] = (in[1] + in[5] + in[nextrow+1] + in[nextrow+5]) >> 2;
					out[2] = (in[2] + in[6] + in[nextrow+2] + in[nextrow+6]) >> 2;
					out[3] = (in[3] + in[7] + in[nextrow+3] + in[nextrow+7]) >> 2;
				}
			}
		}
		else
		{
			for (i = 0 ; i < *height ; i++)
			{
				for (j = 0 ; j < *width ; j++, out += 4, in += 8)
				{
					out[0] = (in[0] + in[4]) >> 1;
					out[1] = (in[1] + in[5]) >> 1;
					out[2] = (in[2] + in[6]) >> 1;
					out[3] = (in[3] + in[7]) >> 1;
				}
			}
		}
	}
	else if (*height > 1)
	{
		*height >>= 1;
		for (i = 0 ; i < *height ; i++, in += nextrow)
		{
			for (j = 0 ; j < *width ; j++, out += 4, in += 4)
			{
				out[0] = (in[0] + in[nextrow+0]) >> 1;
				out[1] = (in[1] + in[nextrow+1]) >> 1;
				out[2] = (in[2] + in[nextrow+2]) >> 1;
				out[3] = (in[3] + in[nextrow+3]) >> 1;
			}
		}
	}
}

static void ScaleDimensions (int width, int height, int *scaled_width, int *scaled_height, int mode)
{
	int	maxsize;

	Q_ROUND_POWER2(width, *scaled_width);
	Q_ROUND_POWER2(height, *scaled_height);

	if (mode & TEX_MIPMAP)
	{
		*scaled_width >>= (int)gl_picmip.value;
		*scaled_height >>= (int)gl_picmip.value;
	}

	maxsize = (mode & TEX_MIPMAP) ? gl_max_size.value : gl_max_size_default;

	*scaled_width = bound(1, *scaled_width, maxsize);
	*scaled_height = bound(1, *scaled_height, maxsize);
}

/*
===============
GL_Upload32
===============
*/
void GL_Upload32 (unsigned *data, int width, int height, int mode)
{
	if(isSimulator)
		return;

	int		internal_format, scaled_width, scaled_height, miplevel;
	unsigned int *scaled;

	Q_ROUND_POWER2(width, scaled_width);
	Q_ROUND_POWER2(height, scaled_height);

	scaled = Q_malloc (scaled_width * scaled_height * 4);
	if (width < scaled_width || height < scaled_height)
	{
		ResampleTexture (data, width, height, scaled, scaled_width, scaled_height, !!gl_lerptextures.value);
		width = scaled_width;
		height = scaled_height;
	}
	else
	{
		memcpy (scaled, data, width * height * 4);
	}

	ScaleDimensions (width, height, &scaled_width, &scaled_height, mode);

	while (width > scaled_width || height > scaled_height)
		MipMap ((byte *)scaled, &width, &height);

	internal_format = (mode & TEX_ALPHA) ? gl_alpha_format : gl_solid_format;

	miplevel = 0;
	glTexImage2D (GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	if (mode & TEX_MIPMAP)
	{
		while (width > 1 || height > 1)
		{
			MipMap ((byte *)scaled, &width, &height);
			miplevel++;
			glTexImage2D (GL_TEXTURE_2D, miplevel, internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		}

		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	free (scaled);
}

/*
===============
GL_Upload8
===============
*/
void GL_Upload8 (byte *data, int width, int height, int mode)
{
	int		i, size, p;
	unsigned	*table;
	static unsigned	trans[640*480];

	table = (mode & TEX_BRIGHTEN) ? d_8to24table2 : d_8to24table;
	size = width * height;

	if (size * 4 > sizeof(trans))
		Sys_Error ("GL_Upload8: image too big");

	if (mode & TEX_FULLBRIGHT)
	{
	// this is a fullbright mask, so make all non-fullbright colors transparent
		mode |= TEX_ALPHA;
		for (i = 0 ; i < size ; i++)
		{
			p = data[i];
			if (p < 224)
				trans[i] = table[p] & 0x00FFFFFF;	// transparent
			else
				trans[i] = table[p];			// fullbright
		}
	}
	else if (mode & TEX_ALPHA)
	{
	// if there are no transparent pixels, make it a 3 component texture even
	// if it was specified as otherwise
		mode &= ~TEX_ALPHA;
		for (i = 0 ; i < size ; i++)
		{
			p = data[i];
			if (p == 255)
				mode |= TEX_ALPHA;
			trans[i] = table[p];
		}
	}
	else
	{
		if (size & 3)
			Sys_Error ("GL_Upload8: bad size (%d)", size);

		for (i = 0 ; i < size ; i++)
			trans[i] = table[data[i]];
	}

	GL_Upload32 (trans, width, height, mode);
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture (char *identifier, int width, int height, byte *data, int mode, int bpp)
{
	if(isSimulator)
		return 1;

	int		i, scaled_width, scaled_height, crc = 0;
	gltexture_t	*glt;

	ScaleDimensions (width, height, &scaled_width, &scaled_height, mode);

	// see if the texture is already present
	if (identifier[0])
	{
		crc = CRC_Block (data, width * height * bpp);
		for (i = 0, glt = gltextures ; i < numgltextures ; i++, glt++)
		{
			if (!strncmp(identifier, glt->identifier, sizeof(glt->identifier)-1))
			{
				if (width == glt->width && height == glt->height && 
				    scaled_width == glt->scaled_width && scaled_height == glt->scaled_height && 
				    crc == glt->crc && bpp == glt->bpp && 
				    (mode & ~TEX_COMPLAIN) == (glt->texmode & ~TEX_COMPLAIN))
				{
					GL_Bind (gltextures[i].texnum);
					return gltextures[i].texnum;	// texture cached
				}
				else
				{
					goto GL_LoadTexture_setup;	// reload the texture into the same slot
				}
			}
		}
	}

	if (numgltextures == MAX_GLTEXTURES)
		Sys_Error ("GL_LoadTexture: numgltextures == MAX_GLTEXTURES");

	glt = &gltextures[numgltextures];
	numgltextures++;

	Q_strncpyz (glt->identifier, identifier, sizeof(glt->identifier));
	glt->texnum = texture_extension_number;
	texture_extension_number++;

GL_LoadTexture_setup:
	glt->width = width;
	glt->height = height;
	glt->scaled_width = scaled_width;
	glt->scaled_height = scaled_height;
	glt->texmode = mode;
	glt->crc = crc;
	glt->bpp = bpp;
	if (glt->pathname)
	{
		Z_Free (glt->pathname);
		glt->pathname = NULL;
	}

	if (bpp == 4 && com_netpath[0])
		glt->pathname = CopyString (com_netpath);

	GL_Bind (glt->texnum);

	switch (bpp)
	{
	case 1:
		GL_Upload8 (data, width, height, mode);
		break;

	case 4:
		GL_Upload32 ((void *)data, width, height, mode);
		break;

	default:
		Sys_Error ("GL_LoadTexture: unknown bpp");
		break;
	}

	return glt->texnum;
}

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture (char *name, mpic_t *pic, byte *data)
{
	int	i, width, height;
	char	fullname[MAX_QPATH] = "pic:";

	Q_ROUND_POWER2(pic->width, width);
	Q_ROUND_POWER2(pic->height, height);

	Q_strncpyz (fullname + 4, name, sizeof(fullname) - 4);
	if (width == pic->width && height == pic->height)
	{
		pic->texnum = GL_LoadTexture (fullname, width, height, data, TEX_ALPHA, 1);
		pic->sl = 0;
		pic->sh = 1;
		pic->tl = 0;
		pic->th = 1;
	}
	else
	{
		byte	*src, *dest, *buf;

		buf = Q_calloc (width * height, 1);

		src = data;
		dest = buf;
		for (i = 0 ; i < pic->height ; i++)
		{
			memcpy (dest, src, pic->width);
			src += pic->width;
			dest += width;
		}

		pic->texnum = GL_LoadTexture (fullname, width, height, buf, TEX_ALPHA, 1);
		pic->sl = 0;
		pic->sh = (float)pic->width / width;
		pic->tl = 0;
		pic->th = (float)pic->height / height;

		free (buf);
	}                                 	

	return pic->texnum;
}

/*
================
GL_FindTexture
================
*/
gltexture_t *GL_FindTexture (char *identifier)
{
	int	i;

	for (i = 0 ; i < numgltextures ; i++)
	{
		if (!strcmp(identifier, gltextures[i].identifier))
			return &gltextures[i];
	}

	return NULL;
}

static	gltexture_t	*current_texture = NULL;

#define CHECK_TEXTURE_ALREADY_LOADED\
	if (CheckTextureLoaded(mode))	\
	{								\
		current_texture = NULL;		\
		fclose (f);					\
		return NULL;				\
	}

static qboolean CheckTextureLoaded (int mode)
{
	int	scaled_width, scaled_height;

	if (current_texture && current_texture->pathname && !strcmp(com_netpath, current_texture->pathname))
	{
		ScaleDimensions (current_texture->width, current_texture->height, &scaled_width, &scaled_height, mode);
		if (current_texture->scaled_width == scaled_width && current_texture->scaled_height == scaled_height)
			return true;
	}

	return false;
}

byte *GL_LoadImagePixels (char *filename, int matchwidth, int matchheight, int mode)
{
	char	basename[MAX_QPATH], name[MAX_QPATH];
	byte	*c, *data;
	FILE	*f;

	COM_StripExtension (filename, basename);
	for (c = basename ; *c ; c++)
		if (*c == '*')
			*c = '#';

	Q_snprintfz (name, sizeof(name), "%s.tga", basename);
	if (COM_FOpenFile(name, &f) != -1)
	{
		CHECK_TEXTURE_ALREADY_LOADED;
		if ((data = Image_LoadTGA(f, name, matchwidth, matchheight)))
			return data;
	}

	Q_snprintfz (name, sizeof(name), "%s.png", basename);
	if (COM_FOpenFile(name, &f) != -1)
	{
		CHECK_TEXTURE_ALREADY_LOADED;
		if ((data = Image_LoadPNG(f, name, matchwidth, matchheight)))
			return data;
	}

	Q_snprintfz (name, sizeof(name), "%s.jpg", basename);
	if (COM_FOpenFile(name, &f) != -1)
	{
		CHECK_TEXTURE_ALREADY_LOADED;
		if ((data = Image_LoadJPEG(f, name, matchwidth, matchheight)))
			return data;
	}

	if (mode & TEX_COMPLAIN)
	{
		if (!no24bit)
			Con_Printf ("\x02" "Couldn't load %s image\n", COM_SkipPath(filename));
	}

	return NULL;
}

int GL_LoadTexturePixels (byte *data, char *identifier, int width, int height, int mode)
{
	int		i, j, image_size;
	qboolean	gamma;

	image_size = width * height;
	gamma = (vid_gamma != 1);

	if (mode & TEX_LUMA)
	{
		gamma = false;
	}
	else if (mode & TEX_ALPHA)
	{
		mode &= ~TEX_ALPHA;
		for (j = 0 ; j < image_size ; j++)
		{
			if (((((unsigned *)data)[j] >> 24) & 0xFF) < 255)
			{
				mode |= TEX_ALPHA;
				break;
			}
		}
	}

	if (gamma)
	{
		for (i = 0 ; i < image_size ; i++)
		{
			data[4*i+0] = vid_gamma_table[data[4*i+0]];
			data[4*i+1] = vid_gamma_table[data[4*i+1]];
			data[4*i+2] = vid_gamma_table[data[4*i+2]];
		}
	}

	return GL_LoadTexture (identifier, width, height, data, mode, 4);
}

byte	player_texels32[1024*1024*4];	//HEAVY TESTING!!!
int	player32_width, player32_height;

int GL_LoadTextureImage (char *filename, char *identifier, int matchwidth, int matchheight, int mode)
{
	int		texnum;
	byte		*data;
	gltexture_t	*gltexture;

	if (no24bit)
		return 0;

	if (!identifier)
		identifier = filename;

	gltexture = current_texture = GL_FindTexture (identifier);

	if (!(data = GL_LoadImagePixels(filename, matchwidth, matchheight, mode)))
	{
		texnum = (gltexture && !current_texture) ? gltexture->texnum : 0;
	}
	else
	{
		texnum = GL_LoadTexturePixels (data, identifier, image_width, image_height, mode);
		// save 32 bit texels if hi-res player skin found
		if (!strncmp(identifier, "player_", 7))
		{
			memcpy (player_texels32, data, image_width * image_height * 4);
			player32_width = image_width;
			player32_height = image_height;
		}
		free (data);
	}

	current_texture = NULL;
	return texnum;
}

mpic_t *GL_LoadPicImage (char *filename, char *id, int matchwidth, int matchheight, int mode)
{
	int		width, height, i;
	char		identifier[MAX_QPATH] = "pic:";
	byte		*data, *src, *dest, *buf;
	static	mpic_t	pic;

	if (no24bit)
		return NULL;

	if (!(data = GL_LoadImagePixels(filename, matchwidth, matchheight, 0)))
		return NULL;

	pic.width = image_width;
	pic.height = image_height;

	if (mode & TEX_ALPHA)
	{
		mode &= ~TEX_ALPHA;
		for (i=0 ; i < image_width * image_height ; i++)
		{
			if (((((unsigned *)data)[i] >> 24) & 0xFF) < 255)
			{
				mode |= TEX_ALPHA;
				break;
			}
		}
	}

	Q_ROUND_POWER2(pic.width, width);
	Q_ROUND_POWER2(pic.height, height);

	Q_strncpyz (identifier + 4, id ? id : filename, sizeof(identifier) - 4);
	if (width == pic.width && height == pic.height)
	{
		pic.texnum = GL_LoadTexture (identifier, pic.width, pic.height, data, mode, 4);
		pic.sl = 0;
		pic.sh = 1;
		pic.tl = 0;
		pic.th = 1;
	}
	else
	{
		buf = Q_calloc (width * height, 4);

		src = data;
		dest = buf;
		for (i = 0 ; i < pic.height ; i++)
		{
			memcpy (dest, src, pic.width * 4);
			src += pic.width * 4;
			dest += width * 4;
		}
		pic.texnum = GL_LoadTexture (identifier, width, height, buf, mode & ~TEX_MIPMAP, 4);
		pic.sl = 0;
		pic.sh = (float)pic.width / width;
		pic.tl = 0;
		pic.th = (float)pic.height / height;
		free (buf);
	}

	free (data);

	return &pic;
}

int GL_LoadCharsetImage (char *filename, char *identifier)
{
	int		i, j, texnum, image_size;
	byte		*data, *buf, *dest, *src;
	qboolean	transparent = false;

	if (no24bit)
		return 0;

	if (!(data = GL_LoadImagePixels(filename, 0, 0, 0)))
		return 0;

	if (!identifier)
		identifier = filename;

	image_size = image_width * image_height;

	for (j = 0 ; j < image_size ; j++)
	{
		if (((((unsigned *)data)[j] >> 24) & 0xFF) < 255)
		{
			transparent = true;
			break;
		}
	}
	if (!transparent)
	{
		for (i = 0 ; i < image_width * image_height ; i++)
		{
			if (data[i*4] == data[0] && data[i*4+1] == data[1] && data[i*4+2] == data[2])
				data[i*4+3] = 0;
		}
	}

	buf = dest = Q_calloc (image_size * 2, 4);
	src = data;
	for (i=0 ; i<16 ; i++)
	{
		memcpy (dest, src, image_size >> 2);	
		src += image_size >> 2;
		dest += image_size >> 1;				
	}

	texnum = GL_LoadTexture (identifier, image_width, image_height * 2, buf, TEX_ALPHA, 4);

	free (buf);
	free (data);

	return texnum;
}

static	GLenum	oldtarget = GL_TEXTURE0_ARB;
static	int	cnttextures[4] = {-1, -1, -1, -1};     // cached
static qboolean	mtexenabled = false;

void GL_SelectTexture (GLenum target) 
{
	if (target == oldtarget)
		return;

	qglActiveTexture (target);

	cnttextures[oldtarget-GL_TEXTURE0_ARB] = currenttexture;
	currenttexture = cnttextures[target-GL_TEXTURE0_ARB];
	oldtarget = target;
}

void GL_DisableMultitexture (void) 
{
	if (mtexenabled)
	{
		Q_glDisable (GL_TEXTURE_2D);
		GL_SelectTexture (GL_TEXTURE0_ARB);
		mtexenabled = false;
	}
}

void GL_EnableMultitexture (void) 
{
	if (gl_mtexable)
	{
		GL_SelectTexture (GL_TEXTURE1_ARB);
		Q_glEnable (GL_TEXTURE_2D);
		mtexenabled = true;
	}
}

void GL_EnableTMU (GLenum target)
{
	GL_SelectTexture (target);
	Q_glEnable (GL_TEXTURE_2D);
}

void GL_DisableTMU (GLenum target)
{
	GL_SelectTexture (target);
	Q_glDisable (GL_TEXTURE_2D);
}
