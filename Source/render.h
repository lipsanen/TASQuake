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

// refresh.h -- public interface to refresh functions

#define	TOP_RANGE		16		// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

typedef struct efrag_s
{
	struct mleaf_s	*leaf;
	struct efrag_s	*leafnext;
	struct entity_s	*entity;
	struct efrag_s	*entnext;
} efrag_t;

typedef struct entity_s
{
	qboolean forcelink;			// model changed

	int		update_type;

	entity_state_t baseline;	// to fill in defaults in updates

	double	msgtime;			// time of last update
	vec3_t	msg_origins[2];		// last two updates (0 is newest)
	vec3_t	origin;
	vec3_t	msg_angles[2];		// last two updates (0 is newest)
	vec3_t	angles;
	struct model_s	*model;		// NULL = no model
	struct efrag_s	*efrag;		// linked list of efrags
	int		frame;
	float	syncbase;			// for client-side animations
	byte	*colormap;
	int		effects;			// light, particals, etc
	int		skinnum;			// for Alias models
	int		visframe;			// last frame this entity was found in an active leaf

	int		dlightframe;		// dynamic lighting
	int		dlightbits;

// FIXME: could turn these into a union
	int		trivial_accept;
	struct mnode_s *topnode;	// for bmodels, first world node that splits bmodel, or NULL if not split

	int		modelindex;
	vec3_t	trail_origin;
	qboolean traildrawn;

#ifdef GLQUAKE
	qboolean noshadow;

	// interpolation
	float	frame_start_time;
	float	frame_interval;
	int		pose1, pose2;
	float	framelerp;

	float	translate_start_time;
	vec3_t	origin1, origin2;

	float	rotate_start_time;
	vec3_t	angles1, angles2;

	// nehahra support
	float	transparency;
	float	smokepuff_time;

	qboolean istransparent;
#endif
} entity_t;

// md3 related
typedef struct tagentity_s
{
	entity_t	ent;

	float		tag_translate_start_time;
	vec3_t		tag_pos1, tag_pos2;

	float		tag_rotate_start_time[3];
	vec3_t		tag_rot1[3], tag_rot2[3];
} tagentity_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vrect_t	vrect;						// subwindow in video for refresh
										// FIXME: not need vrect next field here?
	vrect_t	aliasvrect;					// scaled Alias version
	int		vrectright, vrectbottom;	// right & bottom screen coords
	int		aliasvrectright, aliasvrectbottom; // scaled Alias versions
	float	vrectrightedge;				// rightmost right edge we care about, for use in edge list
	float	fvrectx, fvrecty;			// for floating-point compares
	float	fvrectx_adj, fvrecty_adj;	// left and top edges, for clamping
	int		vrect_x_adj_shift20;		// (vrect.x + 0.5 - epsilon) << 20
	int		vrectright_adj_shift20;		// (vrectright + 0.5 - epsilon) << 20
	float	fvrectright_adj, fvrectbottom_adj; // right and bottom edges, for clamping
	float	fvrectright;				// rightmost edge, for Alias clamping
	float	fvrectbottom;				// bottommost edge, for Alias clamping
	float	horizontalFieldOfView;		// at Z = 1.0, this many X is visible 
										// 2.0 = 90 degrees
	float	xOrigin;					// should probably always be 0.5
	float	yOrigin;					// between be around 0.3 to 0.5

	vec3_t	vieworg;
	vec3_t	viewangles;
	
	float	fov_x, fov_y;

	int		ambientlight;
} refdef_t;

// refresh
extern	refdef_t	r_refdef;
extern	vec3_t		r_origin, vpn, vright, vup;

extern	struct texture_s *r_notexture_mip;

typedef enum
{
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2
} r_ptype_t;

typedef struct r_particle_s
{
	vec3_t	org;
	float	color;
	vec3_t	vel;
	float	ramp;
	float	die;
	r_ptype_t	type;
	struct r_particle_s* next;
} r_particle_t;

extern	entity_t	r_worldentity;
extern r_particle_t* r_particles, * r_active_particles, * r_free_particles;

void R_Init (void);
void R_InitTextures (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first
void R_RenderOverlay (void);  // Jukspa: call after renderview
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect); // called whenever r_refdef or vid change
void R_InitSky (miptex_t *mt);		// called at level load

void R_AddEfrags (entity_t *ent);
void R_RemoveEfrags (entity_t *ent);

void R_NewMap (void);

// particles

typedef enum trail_type_s
{
	ROCKET_TRAIL, GRENADE_TRAIL, BLOOD_TRAIL, BIG_BLOOD_TRAIL, TRACER1_TRAIL,
	TRACER2_TRAIL, VOOR_TRAIL, LAVA_TRAIL, BUBBLE_TRAIL, NEHAHRA_SMOKE
} trail_type_t;

void R_ParseParticleEffect (void);
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail (vec3_t start, vec3_t end, vec3_t *trail_origin, trail_type_t type);

void R_EntityParticles (entity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_ColorMappedExplosion (vec3_t org, int colorStart, int colorLength);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);

void R_PushDlights (void);
void R_InitParticles (void);
void R_ClearParticles (void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);

// surface cache related
extern qboolean	r_cache_thrash;	// set if thrashing the surface cache

int D_SurfaceCacheForRes (int width, int height);
void D_FlushCaches (void);
void D_DeleteSurfaceCache (void);
void D_InitCaches (void *buffer, int size);
void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj);
