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
	if (vidmode_active && !Q_atof(value))
	{
		Con_Printf ("Cannot turn %s off when using -fullscreen mode\n", var->name);
		return true;
	}

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
	if(r_norefresh.value != 0)
		return;

	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;
    Q_glClearColor(0, 0, 0, 1);
    Q_glClear(GL_COLOR_BUFFER_BIT);
}

void GL_EndRendering (void)
{
	if(r_norefresh.value != 0)
		return;

    SDL_GL_SwapWindow(window);

	if (fullsbardraw)
		Sbar_Changed ();

}

int VID_Has_Focus()
{
	return (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void Sys_SendKeyEvents (void)
{
    SDL_Event event;
    int result;
    do {
        result = SDL_PollEvent(&event);

        if(result) {
            IN_SDL_Event(&event);
        }
    } while (result);
}

void VID_LockBuffer (void)
{
}

void VID_UnlockBuffer (void)
{
}

void signal_handler (int sig)
{
	printf ("Received signal %d, exiting...\n", sig);
	void* array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	backtrace_symbols_fd(array, size, stderr);

	Sys_Quit ();
	exit (0);
}

void InitSig (void)
{
	signal (SIGHUP, signal_handler);
	signal (SIGINT, signal_handler);
	signal (SIGQUIT, signal_handler);
	signal (SIGILL, signal_handler);
	signal (SIGTRAP, signal_handler);
	signal (SIGIOT, signal_handler);
	signal (SIGBUS, signal_handler);
	signal (SIGFPE, signal_handler);
	signal (SIGSEGV, signal_handler);
	signal (SIGTERM, signal_handler);
}

static int Tasquake_SDL_Init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    if (SDL_CreateWindowAndRenderer(scr_width, scr_height, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    glcontext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0); // no vsync

    return 0;
}

static void VID_Calc_Params()
{
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

void VID_Window_Event(SDL_WindowEvent* event)
{
	if(event->event == SDL_WINDOWEVENT_RESIZED)
	{
		scr_width = event->data1;
		scr_height = event->data2;
		VID_Calc_Params();
	}
}

void VID_Init (unsigned char *palette)
{
	int width_index = COM_CheckParm("-width");
	int height_index = COM_CheckParm("-height");

	if(width_index > 0)
	{
		scr_width = Q_atoi(com_argv[width_index+1]);
	}

	if(height_index > 0)
	{
		scr_height = Q_atoi(com_argv[height_index+1]);
	}

	VID_Calc_Params();
    InitSig();
    Tasquake_SDL_Init();
	GL_Init ();

	Check_Gamma (palette);
	VID_SetPalette (palette);

	Con_Printf ("Video mode %dx%d initialized.\n", scr_width, scr_height);

	if (fullscreen)
		vid_windowedmouse = false;

	if (COM_CheckParm("-fullsbar"))
		fullsbardraw = true;
}

void VID_Shutdown(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}
