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
// vid_wgl.c -- Windows 9x/NT OpenGL driver

#include "quakedef.h"
#include "winquake.h"
#include "resource.h"
#include <commctrl.h>

qboolean gl_have_stencil = false;

/*
	CRASH FORT
*/
nqvideo_t nqvideo;

/*
	CRASH FORT

	Sets the global values glx, gly, glwidth, glheight

	From a 640x480 base resolution, calculates what the size will be in the actual window size.
	Uses vertical borders for widescreen resolutions.
*/
void NQ_CalculateVirtualDisplaySize(void)
{
	const float aspect43 = 4.0f / 3.0f;

	if (nqvideo.noscale)
	{
		glwidth = nqvideo.displaywidth;
		glheight = nqvideo.displayheight;

		glx = (nqvideo.desktopwidth - glwidth) / 2;
		gly = (nqvideo.desktopheight - glheight) / 2;
	}

	else
	{
		glwidth = nqvideo.displaywidth;
		glheight = glwidth / aspect43;

		if (glheight > nqvideo.displayheight)
		{
			glwidth = nqvideo.displayheight * aspect43;
			glheight = nqvideo.displayheight;
		}

		glx = (nqvideo.displaywidth - glwidth) / 2;
		gly = (nqvideo.displayheight - glheight) / 2;
	}
}

qboolean	DDActive;
qboolean	scr_skipupdate;

static qboolean	vid_initialized = false;
static qboolean vid_canalttab = false;
static qboolean vid_wassuspended = false;
static qboolean windowed_mouse = true;
extern qboolean	mouseactive;	// from in_win.c
static	HICON	hIcon;

HWND	mainwindow;

qboolean fullsbardraw = false;

HGLRC	baseRC;
HDC		maindc;

glvert_t glv;

unsigned short	*currentgammaramp = NULL;
static unsigned short systemgammaramp[3][256];

qboolean	vid_gammaworks = false;
qboolean	vid_hwgamma_enabled = false;
qboolean	customgamma = false;

void RestoreHWGamma (void);

modestate_t	modestate = MS_UNINIT;

void VID_MenuDraw (void);
void VID_MenuKey (int key);

LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AppActivate (BOOL fActive, BOOL minimize);
void ClearAllStates (void);
void VID_UpdateWindowStatus (void);
void GL_Init (void);

cvar_t		_windowed_mouse = {"_windowed_mouse", "1", CVAR_ARCHIVE};
cvar_t		vid_hwgammacontrol = {"vid_hwgammacontrol", "1"};

typedef BOOL (APIENTRY *SWAPINTERVALFUNCPTR)(int);
SWAPINTERVALFUNCPTR wglSwapIntervalEXT = NULL;
static qboolean update_vsync = false;
qboolean OnChange_vid_vsync (cvar_t *var, char *string);
cvar_t		vid_vsync = {"vid_vsync", "0", 0, OnChange_vid_vsync};

int		window_center_x, window_center_y, window_x, window_y, window_width, window_height;
RECT		window_rect;

void CheckVsyncControlExtensions (void)
{
	if (!COM_CheckParm("-noswapctrl") && CheckExtension("WGL_EXT_swap_control"))
	{
		if ((wglSwapIntervalEXT = (void *)wglGetProcAddress("wglSwapIntervalEXT")))
		{
			Con_Printf("Vsync control extensions found\n");
			Cvar_Register (&vid_vsync);
		}
	}
}

qboolean OnChange_vid_vsync (cvar_t *var, char *string)
{
	update_vsync = true;
	return false;
}

// direct draw software compatability stuff

void VID_LockBuffer (void)
{
}

void VID_UnlockBuffer (void)
{
}

void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

void CenterWindow (HWND hWndCenter, int width, int height, BOOL lefttopjustify)
{
	int     CenterX, CenterY;

	CenterX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	CenterY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	if (CenterX > CenterY * 2)
		CenterX >>= 1;	// dual screens
	CenterX = (CenterX < 0) ? 0 : CenterX;
	CenterY = (CenterY < 0) ? 0 : CenterY;
	SetWindowPos (hWndCenter, NULL, CenterX, CenterY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_DRAWFRAME);
}

/*
================
VID_UpdateWindowStatus
================
*/
void VID_UpdateWindowStatus (void)
{
	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	IN_UpdateClipCursor ();
}

//=================================================================

/*
=================
GL_BeginRendering
=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = nqvideo.virtualwidth;
	*height = nqvideo.virtualheight;
}

/*
=================
GL_EndRendering
=================
*/
void GL_EndRendering (void)
{
	if (!scr_skipupdate || block_drawing)
	{
		if (wglSwapIntervalEXT && update_vsync && vid_vsync.string[0])
			wglSwapIntervalEXT (vid_vsync.value ? 1 : 0);
		update_vsync = false;

		SwapBuffers (maindc);
	}

	// handle the mouse state when windowed if that's changed
	if (modestate == MS_WINDOWED)
	{
		if (!_windowed_mouse.value)
		{
			if (windowed_mouse)
			{
				IN_DeactivateMouse ();
				IN_ShowMouse ();
				windowed_mouse = false;
			}
		}
		else
		{
			windowed_mouse = true;
			if (key_dest == key_game && !mouseactive && ActiveApp)
			{
				IN_ActivateMouse ();
				IN_HideMouse ();
			}
			else if (mouseactive && key_dest != key_game)
			{
				IN_DeactivateMouse ();
				IN_ShowMouse ();
			}
		}
	}

	/*
		CRASH FORT
		Makes sure the cursor gets released in pause mode, why would this be here don't ask me
	*/
	else if (modestate == MS_FULLDIB)
	{
		if (key_dest == key_game && !mouseactive && ActiveApp)
		{
			IN_ActivateMouse();
			IN_HideMouse();
		}

		else if (mouseactive && key_dest != key_game)
		{
			IN_DeactivateMouse();
			IN_ShowMouse();
		}
	}

	if (fullsbardraw)
		Sbar_Changed ();
}

void VID_SetDefaultMode (void)
{
	IN_DeactivateMouse ();
}

void VID_ShiftPalette (unsigned char *palette)
{
}

/*
======================
VID_SetDeviceGammaRamp

Note: ramps must point to a static array
======================
*/
void VID_SetDeviceGammaRamp (unsigned short *ramps)
{
	if (vid_gammaworks)
	{
		currentgammaramp = ramps;
		if (vid_hwgamma_enabled)
		{
			/*SetDeviceGammaRamp (maindc, ramps);*/
			customgamma = true;
		}
	}
}

void InitHWGamma (void)
{
	if (COM_CheckParm("-nohwgamma"))
		return;

	vid_gammaworks = GetDeviceGammaRamp (maindc, systemgammaramp);
}

void RestoreHWGamma (void)
{
	if (vid_gammaworks && customgamma)
	{
		customgamma = false;
		/*SetDeviceGammaRamp (maindc, systemgammaramp);*/
	}
}

//=================================================================

void VID_Shutdown (void)
{
   	HGLRC	hRC;
   	HDC	hDC;

	if (vid_initialized)
	{
		RestoreHWGamma ();

		vid_canalttab = false;
		hRC = wglGetCurrentContext ();
		hDC = wglGetCurrentDC ();

		wglMakeCurrent (NULL, NULL);

		if (hRC)
			wglDeleteContext (hRC);

		if (hDC && mainwindow)
			ReleaseDC (mainwindow, hDC);

		/*if (modestate == MS_FULLDIB)
			ChangeDisplaySettings (NULL, 0);*/

		if (maindc && mainwindow)
			ReleaseDC (mainwindow, maindc);

		AppActivate (false, false);
	}
}

int bChosePixelFormat(HDC hDC, PIXELFORMATDESCRIPTOR *pfd, PIXELFORMATDESCRIPTOR *retpfd)
{
	int	pixelformat;

	if (!(pixelformat = ChoosePixelFormat(hDC, pfd)))
	{
		MessageBox (NULL, "ChoosePixelFormat failed", "Error", MB_OK);
		return 0;
	}

	if (!(DescribePixelFormat(hDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), retpfd)))
	{
		MessageBox(NULL, "DescribePixelFormat failed", "Error", MB_OK);
		return 0;
	}

	return pixelformat;
}

BOOL bSetupPixelFormat (HDC hDC)
{
	int	pixelformat;
	PIXELFORMATDESCRIPTOR retpfd, pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,						// version number
		PFD_DRAW_TO_WINDOW 		// support window
		| PFD_SUPPORT_OPENGL 	// support OpenGL
		| PFD_DOUBLEBUFFER,		// double buffered
		PFD_TYPE_RGBA,			// RGBA type
		24,						// 24-bit color depth
		0, 0, 0, 0, 0, 0,		// color bits ignored
		0,						// no alpha buffer
		0,						// shift bit ignored
		0,						// no accumulation buffer
		0, 0, 0, 0, 			// accum bits ignored
		24,						// 24-bit z-buffer	
		8,						// 8-bit stencil buffer
		0,						// no auxiliary buffer
		PFD_MAIN_PLANE,			// main layer
		0,						// reserved
		0, 0, 0					// layer masks ignored
	};

	if (!(pixelformat = bChosePixelFormat(hDC, &pfd, &retpfd)))
		return FALSE;

	if (retpfd.cDepthBits < 24)
	{
		pfd.cDepthBits = 24;
		if (!(pixelformat = bChosePixelFormat(hDC, &pfd, &retpfd)))
			return FALSE;
	}

	if (!SetPixelFormat(hDC, pixelformat, &retpfd))
	{
		MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
		return FALSE;
	}

	if (retpfd.cDepthBits < 24)
		gl_allow_ztrick = false;

	gl_have_stencil = true;
	return TRUE;
}

/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
void ClearAllStates (void)
{
	int	i;
	
// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		if (keydown[i])
			Key_Event (i, false);
	}

	Key_ClearStates ();
	IN_ClearStates ();
}

void AppActivate (BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static BOOL	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	if (fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();

			if (vid_canalttab && !Minimized && currentgammaramp)
				VID_SetDeviceGammaRamp (currentgammaramp);

			if (vid_canalttab && vid_wassuspended)
			{
				vid_wassuspended = false;
				/*if (ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
					Sys_Error ("Couldn't set fullscreen DIB mode");*/
				
				ShowWindow(mainwindow, SW_SHOWNORMAL);
				
				// scr_fullupdate = 0;
				Sbar_Changed ();
			}
		}

		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value && key_dest == key_game)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
		}
	}

	if (!fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			RestoreHWGamma ();
			if (vid_canalttab) { 
				/*ChangeDisplaySettings (NULL, 0);*/
				vid_wassuspended = true;
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
		}
	}
}

LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int IN_MapKey (int key);

/*
=============
Main Window procedure
=============
*/
LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int	fActive, fMinimized;
	LONG	ret = 1;
	extern	cvar_t		cl_confirmquit;

	switch (uMsg)
	{
	case WM_KILLFOCUS:
		if (modestate == MS_FULLDIB)
			ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
		break;

	case WM_CREATE:
		break;

	case WM_MOVE:
	{
		window_x = (int)LOWORD(lParam);
		window_y = (int)HIWORD(lParam);

		VID_UpdateWindowStatus();

		break;
	}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		Key_Event (IN_MapKey(lParam), true);
		break;
			
	case WM_KEYUP:
	case WM_SYSKEYUP:
		Key_Event (IN_MapKey(lParam), false);
		break;

	case WM_SYSCHAR:
	// keep Alt-Space from happening
		break;

	/*
		CRASH FORT
	*/
	case WM_LBUTTONDOWN:
	{
		/*
			Trick to simulate window moving in borderless windows by dragging anywhere,
			but only if the mouse is free and not playing
		*/
		if (nqvideo.borderless && !mouseactive && key_dest != key_game)
		{
			/*
				Equivalent of GET_X_LPARAM and GET_Y_LPARAM from Windowsx.h,
				but no need to drag in that huge header
			*/
			int posx = ((int)(short)LOWORD(lParam));
			int posy = ((int)(short)HIWORD(lParam));

			SendMessageA(mainwindow, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(posx, posy));
		}

		break;
	}

	/*
		CRASH FORT
	*/
	case WM_INPUT:
	{
		RAWINPUT buf;
		UINT bufsize = sizeof(buf);

		UINT size = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &buf, &bufsize, sizeof(RAWINPUTHEADER));

		if (size <= sizeof(buf))
		{
			if (buf.header.dwType == RIM_TYPEMOUSE)
			{
				IN_RawMouseEvent(&buf.data.mouse);				
			}
		}

		break;
	}

	case WM_SIZE:
		break;

	case WM_CLOSE:
		if (!cl_confirmquit.value || 
		    MessageBox(mainwindow, "Are you sure you want to quit?", "Confirm Exit", MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
			Sys_Quit ();
	        break;

	case WM_ACTIVATE:
		fActive = LOWORD(wParam);
		fMinimized = (BOOL)HIWORD(wParam);
		AppActivate (!(fActive == WA_INACTIVE), fMinimized);

	// fix the leftover Alt from any Alt-Tab or the like that switched us away
		ClearAllStates ();

		break;

	case WM_DESTROY:
		if (mainwindow)
			DestroyWindow (mainwindow);
		PostQuitMessage (0);
		break;

	case MM_MCINOTIFY:
		ret = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
		break;

	default:
	// pass all unhandled messages to DefWindowProc
		ret = DefWindowProc (hWnd, uMsg, wParam, lParam);
		break;
	}

	// return 1 if handled message, 0 if not
	return ret;
}

enum
{
	NQ_STYLE_DEFAULT = WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
	NQ_WINDOWED_STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	NQ_FULLSCREEN_STYLE = WS_POPUP,
	NQ_BORDERLESS_STYLE = WS_POPUP,
};

void NQ_SetMode(void)
{
	// so Con_Printfs don't mess us up by forcing vid and snd updates
	qboolean temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;

	CDAudio_Pause ();

	if (nqvideo.fullscreen)
	{
		IN_ActivateMouse();
		IN_HideMouse();
	}

	else
	{
		if (_windowed_mouse.value && key_dest == key_game)
		{
			IN_ActivateMouse();
			IN_HideMouse();
		}

		else
		{
			IN_DeactivateMouse();
			IN_ShowMouse();
		}
	}

	VID_UpdateWindowStatus ();

	CDAudio_Resume ();
	scr_disabled_for_loading = temp;

// now we try to make sure we get the focus on the mode switch, because
// sometimes in some systems we don't. We grab the foreground, then
// finish setting up, pump all our messages, and sleep for a little while
// to let messages finish bouncing around the system, then we put
// ourselves at the top of the z order, then grab the foreground again,
// Who knows if it helps, but it probably doesn't hurt
	SetForegroundWindow (mainwindow);

	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	Sleep (100);

	SetWindowPos (mainwindow, HWND_TOP, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
	SetForegroundWindow (mainwindow);

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates ();

//	VID_SetPalette (palette);

	vid.recalc_refdef = 1;
}

void NQ_InitWindow(void)
{
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(wcex));

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hInstance = global_hInstance;
	wcex.lpszClassName = "App_TASQuake";
	wcex.lpfnWndProc = MainWndProc;

	/* 
		CRASH FORT
		Makes sure the right cursor is displayed when it gets made visible, like in pause mode
	*/
	wcex.hCursor = LoadCursorA(NULL, IDC_ARROW);

	if (!RegisterClassExA(&wcex))
	{
		Sys_Error("Couldn't register window class");
	}

	DWORD style = NQ_STYLE_DEFAULT;
	int posx = 0;
	int posy = 0;
	int width = 0;
	int height = 0;

	if (nqvideo.fullscreen)
	{
		style |= NQ_FULLSCREEN_STYLE;

		width = nqvideo.desktopwidth;
		height = nqvideo.desktopheight;
	}

	else
	{		
		if (nqvideo.borderless)
		{
			style |= NQ_BORDERLESS_STYLE;
		}

		else
		{
			style |= NQ_WINDOWED_STYLE;
		}

		width = nqvideo.displaywidth;
		height = nqvideo.displayheight;

		posx = (nqvideo.desktopwidth - width) / 2;
		posy = (nqvideo.desktopheight - height) / 2;

		RECT displayrect;
		displayrect.left = posx;
		displayrect.top = posy;
		displayrect.right = posx + width;
		displayrect.bottom = posy + height;

		AdjustWindowRectEx(&displayrect, style, FALSE, 0);

		posx = displayrect.left;
		posy = displayrect.top;
		width = displayrect.right - displayrect.left;
		height = displayrect.bottom - displayrect.top;
	}

	HWND window = CreateWindowExA
	(
		0,
		wcex.lpszClassName,
		"TASQuake",
		style,
		posx,
		posy,
		width,
		height,
		NULL,
		NULL,
		global_hInstance,
		NULL
	);

	if (!window)
	{
		Sys_Error("Couldn't create window");
	}

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

	HDC hdc = GetDC(window);
	PatBlt(hdc, 0, 0, nqvideo.displaywidth, nqvideo.displayheight, BLACKNESS);
	ReleaseDC(window, hdc);

	vid.width = nqvideo.virtualwidth;
	vid.height = nqvideo.virtualheight;
	vid.conwidth = vid.width;
	vid.conheight = vid.height;

	vid.numpages = 8;

	window_x = posx;
	window_y = posy;
	window_width = nqvideo.displaywidth;
	window_height = nqvideo.displayheight;

	mainwindow = window;

	SendMessageA(mainwindow, WM_SETICON, (WPARAM)TRUE, (LPARAM)hIcon);
	SendMessageA(mainwindow, WM_SETICON, (WPARAM)FALSE, (LPARAM)hIcon);

	NQ_SetMode();
}

void NQ_Init(void)
{
	HDC hdc = GetDC(NULL);

	if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
	{
		Sys_Error("Can't run in non-RGB mode");
	}

	ReleaseDC(NULL, hdc);

	nqvideo.desktopwidth = GetSystemMetrics(SM_CXSCREEN);
	nqvideo.desktopheight = GetSystemMetrics(SM_CYSCREEN);

	nqvideo.borderless = false;
	nqvideo.fullscreen = false;
	nqvideo.noscale = false;
	
	nqvideo.displaywidth = 1280;
	nqvideo.displayheight = 960;
	
	/*
		Default Quake scale resolution
	*/
	nqvideo.virtualwidth = 640;
	nqvideo.virtualheight = 480;

	if (COM_CheckParm("-window"))
	{
		nqvideo.borderless = COM_CheckParm("-borderless") > 0;

		modestate = MS_WINDOWED;

		nqvideo.fullscreen = false;

		int i;

		if ((i = COM_CheckParm("-width")) && i + 1 < com_argc)
		{
			nqvideo.displaywidth = Q_atoi(com_argv[i + 1]);
		}

		if ((i = COM_CheckParm("-height")) && i + 1 < com_argc)
		{
			nqvideo.displayheight = Q_atoi(com_argv[i + 1]);
		}
	}

	else
	{
		nqvideo.noscale = COM_CheckParm("-noscale") > 0;

		modestate = MS_FULLDIB;
		nqvideo.fullscreen = true;

		if (nqvideo.noscale)
		{
			int i;

			if ((i = COM_CheckParm("-width")) && i + 1 < com_argc)
			{
				nqvideo.displaywidth = Q_atoi(com_argv[i + 1]);
			}

			if ((i = COM_CheckParm("-height")) && i + 1 < com_argc)
			{
				nqvideo.displayheight = Q_atoi(com_argv[i + 1]);
			}
		}

		else
		{
			nqvideo.displaywidth = nqvideo.desktopwidth;
			nqvideo.displayheight = nqvideo.desktopheight;
		}
	}

	NQ_InitWindow();
}

/*
===================
VID_Init
===================
*/
void VID_Init (unsigned char *palette)
{
	DEVMODE		devmode;

	memset (&devmode, 0, sizeof(devmode));

	Cvar_Register (&_windowed_mouse);
	Cvar_Register (&vid_hwgammacontrol);

	hIcon = LoadIconA(global_hInstance, MAKEINTRESOURCE (IDI_ICON2));

	NQ_Init();

	vid_initialized = true;

	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	Check_Gamma (palette);
	VID_SetPalette (palette);

	maindc = GetDC (mainwindow);
	if (!bSetupPixelFormat(maindc))
		Sys_Error ("bSetupPixelFormat failed");

	InitHWGamma ();

	if (!(baseRC = wglCreateContext(maindc)))
		Sys_Error ("Could not initialize GL (wglCreateContext failed).\n\nMake sure you in are 65535 color mode, and try running -window.");
	if (!wglMakeCurrent(maindc, baseRC))
		Sys_Error ("wglMakeCurrent failed");

	GL_Init ();
	CheckVsyncControlExtensions ();

	vid_menudrawfn = NULL;
	vid_menukeyfn = NULL;

	vid_canalttab = true;

	if (COM_CheckParm("-fullsbar"))
		fullsbardraw = true;
}
