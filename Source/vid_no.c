#include "quakedef.h"
#include "in_sdl.h"
#include <signal.h>
#include <SDL2/SDL.h>
#include <GL/gl.h>

qboolean vid_gammaworks = false;
qboolean vid_hwgamma_enabled = false;
qboolean customgamma = false;
qboolean fullsbardraw = false;
qboolean fullscreen = false;
qboolean gl_have_stencil = true;
uint32_t vid_window_flags = 0;
static	qboolean	vidmode_active = false;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_GLContext glcontext;

qboolean OnChange_windowed_mouse (cvar_t *var, char *value);
cvar_t	_windowed_mouse = {"_windowed_mouse", "1", 0, OnChange_windowed_mouse};

qboolean OnChange_windowed_mouse (cvar_t *var, char *value)
{
	return false;
}

static int scr_width = 640, scr_height = 480, scrnum;

void VID_SetDeviceGammaRamp (unsigned short *ramps)
{
	if (vid_gammaworks)
	{
	}
}

void NQ_CalculateVirtualDisplaySize(void)
{

}

void GL_BeginRendering (int *x, int *y, int *width, int *height)
{

}

void GL_EndRendering (void)
{

}

int VID_Has_Focus()
{
	return 0;
}

void Sys_SendKeyEvents (void)
{

}

void VID_LockBuffer (void)
{
}

void VID_UnlockBuffer (void)
{
}

void signal_handler (int sig)
{

}

void InitSig (void)
{

}

static int Tasquake_SDL_Init()
{

    return 0;
}

static void VID_Calc_Params()
{

}

void VID_Window_Event(SDL_WindowEvent* event)
{

}

void VID_Init (unsigned char *palette)
{
	gl_renderer = "trick or treat";

	if(scr_width < 320)
		scr_width = 320;
	if(scr_height < 240)
		scr_height = 240;

	vid.maxwarpwidth = 320;
	vid.maxwarpheight = 200;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	vid.conwidth = 640;
	vid.conwidth &= 0xfff8;	// make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth * (float)scr_height / scr_width;

	if (vid.conheight > scr_height)
		vid.conheight = scr_height;
	if (vid.conwidth > scr_width)
		vid.conwidth = scr_width;
	vid.width = vid.conwidth;
	vid.height = vid.conheight;

	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 2;
	vid.recalc_refdef = 1;
}

void VID_Shutdown(void)
{

}
