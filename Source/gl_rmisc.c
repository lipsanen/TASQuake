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
// gl_rmisc.c

#include "quakedef.h"

qboolean	r_loadq3player = false;

void CL_CopyPlayerInfo (entity_t *ent, entity_t *player);

void R_InitOtherTextures (void)
{
	underwatertexture = GL_LoadTextureImage ("textures/water_caustic", NULL, 0, 0,  TEX_MIPMAP | TEX_ALPHA | TEX_COMPLAIN);
	detailtexture = GL_LoadTextureImage ("textures/detail", NULL, 0, 0, TEX_MIPMAP | TEX_ALPHA | TEX_COMPLAIN);
	damagetexture = GL_LoadTextureImage ("textures/particles/blood_screen", NULL, 0, 0, TEX_ALPHA | TEX_COMPLAIN);
}

/*
==================
R_InitTextures
==================
*/
void R_InitTextures (void)
{
	int		x, y, m;
	byte	*dest;

// create a simple checkerboard texture for the default
	r_notexture_mip = Hunk_AllocName (sizeof(texture_t) + 16*16 + 8*8 + 4*4 + 2*2, "notexture");

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof(texture_t);
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16*16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8*8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4*4;

	for (m = 0 ; m < 4 ; m++)
	{
		dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
		for (y = 0 ; y < (16>>m) ; y++)
			for (x = 0 ; x < (16>>m) ; x++)
				*dest++ = ((y < (8 >> m)) ^ (x < (8 >> m))) ? 0 : 0x0e;
	}
}

int	fb_skins[MAX_SCOREBOARD];

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
void R_TranslatePlayerSkin (int playernum)
{
	int			top, bottom, i, j, size, scaled_width, scaled_height, inwidth, inheight;
	byte		translate[256], *original, *inrow;
	unsigned	translate32[256], pixels[512*256], *out, frac, fracstep;
	model_t		*model;
	aliashdr_t	*paliashdr;
	extern qboolean	Img_HasFullbrights (byte *pixels, int size);

	GL_DisableMultitexture ();

	top = cl.scores[playernum].colors & 0xf0;
	bottom = (cl.scores[playernum].colors & 15) << 4;

	for (i = 0 ; i < 256 ; i++)
		translate[i] = i;

	for (i = 0 ; i < 16 ; i++)
	{
		// the artists made some backward ranges. sigh.
		translate[TOP_RANGE+i] = (top < 128) ? top + i : top + 15 - i;
		translate[BOTTOM_RANGE+i] = (bottom < 128) ? bottom + i : bottom + 15 - i;
	}

	// locate the original skin pixels
	currententity = &cl_entities[1+playernum];
	if (!(model = currententity->model))
		return;		// player doesn't have a model yet
	if (model->type != mod_alias)
		return;		// only translate skins on alias models

	paliashdr = (aliashdr_t *)Mod_Extradata (model);
	size = paliashdr->skinwidth * paliashdr->skinheight;
	if (currententity->skinnum < 0 || currententity->skinnum >= paliashdr->numskins)
	{
		Con_DPrintf ("(%d): Invalid player skin #%d\n", playernum, currententity->skinnum);
		original = (byte *)paliashdr + paliashdr->texels[0];
	}
	else
	{
		original = (byte *)paliashdr + paliashdr->texels[currententity->skinnum];
	}

	if (size & 3)
		Sys_Error ("R_TranslatePlayerSkin: bad size (%d)", size);

	inwidth = paliashdr->skinwidth;
	inheight = paliashdr->skinheight;

	// because this happens during gameplay, do it fast
	// instead of sending it through GL_Upload8
	GL_Bind (playertextures + playernum);

	// c'mon, it's 2006 already....
	if (gl_lerptextures.value)
	{
		byte	*translated;
		static unsigned translated32[1024*1024];
		extern byte	player_texels32[1024*1024*4];
		extern int	player32_width, player32_height;

		// 8 bit
		translated = Q_malloc (inwidth * inheight);

		for (i = 0 ; i < size ; i++)
			translated[i] = translate[original[i]];

		GL_Upload8 (translated, inwidth, inheight, 0);

		fb_skins[playernum] = 0;
		if (Img_HasFullbrights(original, inwidth * inheight))
		{
			fb_skins[playernum] = playertextures + playernum + MAX_SCOREBOARD;

			GL_Bind (fb_skins[playernum]);
			GL_Upload8 (translated, inwidth, inheight, TEX_FULLBRIGHT);
		}

		free (translated);

		// 32 bit - FIXME
/*		for (i=0 ; i<256 ; i++)
			translate32[i] = d_8to24table[translate[i]];

		size = player32_width * player32_height;
		for (i = 0, j = 0 ; i < size ; i++, j += 4)
		{
			byte	avg;

			avg = (player_texels32[j+0] + player_texels32[j+1] + player_texels32[j+2]) / 3;
			translated32[i] = translate32[avg];
		}

		GL_Upload32 (translated32, player32_width, player32_height, 0);*/
	}
	else
	{
		scaled_width = min(gl_max_size.value, 512);
		scaled_height = min(gl_max_size.value, 256);

		// allow users to crunch sizes down even more if they want
		scaled_width >>= (int)gl_playermip.value;
		scaled_height >>= (int)gl_playermip.value;

		scaled_width = max(scaled_width, 1);
		scaled_height = max(scaled_height, 1);

		for (i = 0 ; i < 256 ; i++)
			translate32[i] = d_8to24table[translate[i]];

		out = pixels;
		memset (pixels, 0, sizeof(pixels));
		fracstep = (inwidth << 16) / scaled_width;

		for (i = 0 ; i < scaled_height ; i++, out += scaled_width)
		{
			inrow = original + inwidth * (i * inheight / scaled_height);
			frac = fracstep >> 1;
			for (j = 0 ; j < scaled_width ; j++, frac += fracstep)
				out[j] = translate32[inrow[frac>>16]];
		}

		glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		Q_glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		fb_skins[playernum] = 0;
		if (Img_HasFullbrights(original, inwidth * inheight))
		{
			fb_skins[playernum] = playertextures + playernum + MAX_SCOREBOARD;

			GL_Bind (fb_skins[playernum]);

			out = pixels;
			memset (pixels, 0, sizeof(pixels));
			fracstep = (inwidth << 16) / scaled_width;

			// make all non-fullbright colors transparent
			for (i = 0 ; i < scaled_height ; i++, out += scaled_width)
			{
				inrow = original + inwidth * (i * inheight / scaled_height);
				frac = fracstep >> 1;
				for (j = 0 ; j < scaled_width ; j++, frac += fracstep)
				{
					if (inrow[frac>>16] < 224)
						out[j] = translate32[inrow[frac>>16]] & 0x00FFFFFF; // transparent
					else
						out[j] = translate32[inrow[frac>>16]]; // fullbright
				}
			}

			glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

			Q_glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			Q_glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
}

void R_PreMapLoad (char *mapname)
{
	Cvar_ForceSet (&cl_mapname, mapname);
	lightmode = 2;
}

/*
===============
R_NewMap
===============
*/
void R_NewMap (void)
{
	int	i, waterline;

	for (i = 0 ; i < 256 ; i++)
		d_lightstylevalue[i] = 264;		// normal light value

	memset (&r_worldentity, 0, sizeof(r_worldentity));
	r_worldentity.model = cl.worldmodel;

	// clear out efrags in case the level hasn't been reloaded
	// FIXME: is this one short?
	for (i = 0 ; i < cl.worldmodel->numleafs ; i++)
		cl.worldmodel->leafs[i].efrags = NULL;

	r_viewleaf = NULL;
	R_ClearParticles ();
	R_ClearDecals ();

	GL_BuildLightmaps ();

	// identify sky texture
	for (i = 0 ; i < cl.worldmodel->numtextures ; i++)
	{
		if (!cl.worldmodel->textures[i])
			continue;

		for (waterline = 0 ; waterline < 2 ; waterline++)
		{
 			cl.worldmodel->textures[i]->texturechain[waterline] = NULL;
			cl.worldmodel->textures[i]->texturechain_tail[waterline] = &cl.worldmodel->textures[i]->texturechain[waterline];
		}
	}

	// HACK HACK HACK - create two extra entities if drawing the player's multimodel
	if (r_loadq3player)
	{
		memset (&q3player_body, 0, sizeof(tagentity_t));
		CL_CopyPlayerInfo (&q3player_body.ent, &cl_entities[cl.viewentity]);
		memset (&q3player_head, 0, sizeof(tagentity_t));
		CL_CopyPlayerInfo (&q3player_head.ent, &cl_entities[cl.viewentity]);
	}
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f (void)
{
	int		i;
	float	start, stop, time;

	if (cls.state != ca_connected)
		return;

	Q_glDrawBuffer (GL_FRONT);
	Q_glFinish ();

	start = Sys_DoubleTime ();
	for (i = 0 ; i < 128 ; i++)
	{
		r_refdef.viewangles[1] = i * (360.0 / 128.0);
		R_RenderView ();
	}

	Q_glFinish ();
	stop = Sys_DoubleTime ();
	time = stop - start;
	Con_Printf ("%f seconds (%f fps)\n", time, 128.0 / time);

	Q_glDrawBuffer (GL_BACK);
	GL_EndRendering ();
}

void D_FlushCaches (void)
{
}
