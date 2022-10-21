#include "quakedef.h"
#include "tas/hooks.h"
#include <SDL2/SDL.h>
#include <signal.h>
#include "vid_sdl.h"

// Lots of code taken from quakespasm
extern cvar_t cl_maxpitch;
extern cvar_t cl_minpitch;
qboolean mouseactive = false;
static int total_dx = 0, total_dy = 0;

static int ToQuakeKey(SDL_Scancode scancode);
void IN_DeactivateMouse();

void IN_MouseMotion(int dx, int dy)
{
	if(!mouseactive)
		return;

	if(!VID_Has_Focus())
	{
		IN_DeactivateMouse ();
	}
	else 
	{
		if(key_dest == key_game && mouseactive) {
			total_dx += dx;
			total_dy += dy;
		}
	}
}

void IN_ActivateMouse()
{
	total_dx = 0;
	total_dy = 0;
	SDL_SetRelativeMouseMode(SDL_TRUE);
	mouseactive = true;
}

void IN_DeactivateMouse()
{
	SDL_SetRelativeMouseMode(SDL_FALSE);
	mouseactive = false;
}

void IN_MouseMove(usercmd_t *cmd)
{
    double	mouse_x, mouse_y;

	if (!mouseactive)
		return;

	mouse_x = sensitivity.value * total_dx;
	mouse_y = sensitivity.value * total_dy;
	total_dx = total_dy = 0;
	IN_MouseMove_Hook(mouse_x, mouse_y);

// add mouse X/Y movement to cmd
	if ((in_strafe.state & 1) || (lookstrafe.value && mlook_active))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;

	if (mlook_active)
		V_StopPitchDrift ();

	if (mlook_active && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		cl.viewangles[PITCH] = bound(-70, cl.viewangles[PITCH], 80);
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}
}

static void IN_MouseButton(SDL_Event* event, qboolean down) {
	if(down && event->button.button == SDL_BUTTON_LEFT && !mouseactive && key_dest == key_game) {
		IN_ActivateMouse();
	} else {
		switch(event->button.button) {
		case SDL_BUTTON_LEFT: Key_Event(K_MOUSE1, down); break;
		case SDL_BUTTON_RIGHT: Key_Event(K_MOUSE2, down); break;
		case SDL_BUTTON_MIDDLE: Key_Event(K_MOUSE3, down); break;
		case SDL_BUTTON_X1: Key_Event(K_MOUSE4, down); break;
		case SDL_BUTTON_X2: Key_Event(K_MOUSE5, down); break;
		default: break;
		}
	}
}

int IN_SDL_Event(SDL_Event* event)
{
    switch(event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            Key_Event (ToQuakeKey(event->key.keysym.scancode), event->type == SDL_KEYDOWN);
        break;
        case SDL_MOUSEWHEEL:
        if (event->wheel.y > 0)
        {
            Key_Event(K_MWHEELUP, true);
            Key_Event(K_MWHEELUP, false);
        }
        else if (event->wheel.y < 0)
        {
            Key_Event(K_MWHEELDOWN, true);
            Key_Event(K_MWHEELDOWN, false);
        }
        break;
        case SDL_MOUSEMOTION:
            IN_MouseMotion(event->motion.xrel, event->motion.yrel);
        break;
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			IN_MouseButton(event, event->type == SDL_MOUSEBUTTONDOWN);
			break;
        default:
		case SDL_WINDOWEVENT:
			VID_Window_Event(event);
		break;
        break;
    }

    if (event->type == SDL_QUIT) {
        raise(SIGTERM);
    }
}

static int ToQuakeKey(SDL_Scancode scancode)
{
	switch (scancode)
	{
	case SDL_SCANCODE_TAB: return K_TAB;
	case SDL_SCANCODE_RETURN: return K_ENTER;
	case SDL_SCANCODE_RETURN2: return K_ENTER;
	case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
	case SDL_SCANCODE_SPACE: return K_SPACE;

	case SDL_SCANCODE_A: return 'a';
	case SDL_SCANCODE_B: return 'b';
	case SDL_SCANCODE_C: return 'c';
	case SDL_SCANCODE_D: return 'd';
	case SDL_SCANCODE_E: return 'e';
	case SDL_SCANCODE_F: return 'f';
	case SDL_SCANCODE_G: return 'g';
	case SDL_SCANCODE_H: return 'h';
	case SDL_SCANCODE_I: return 'i';
	case SDL_SCANCODE_J: return 'j';
	case SDL_SCANCODE_K: return 'k';
	case SDL_SCANCODE_L: return 'l';
	case SDL_SCANCODE_M: return 'm';
	case SDL_SCANCODE_N: return 'n';
	case SDL_SCANCODE_O: return 'o';
	case SDL_SCANCODE_P: return 'p';
	case SDL_SCANCODE_Q: return 'q';
	case SDL_SCANCODE_R: return 'r';
	case SDL_SCANCODE_S: return 's';
	case SDL_SCANCODE_T: return 't';
	case SDL_SCANCODE_U: return 'u';
	case SDL_SCANCODE_V: return 'v';
	case SDL_SCANCODE_W: return 'w';
	case SDL_SCANCODE_X: return 'x';
	case SDL_SCANCODE_Y: return 'y';
	case SDL_SCANCODE_Z: return 'z';

	case SDL_SCANCODE_1: return '1';
	case SDL_SCANCODE_2: return '2';
	case SDL_SCANCODE_3: return '3';
	case SDL_SCANCODE_4: return '4';
	case SDL_SCANCODE_5: return '5';
	case SDL_SCANCODE_6: return '6';
	case SDL_SCANCODE_7: return '7';
	case SDL_SCANCODE_8: return '8';
	case SDL_SCANCODE_9: return '9';
	case SDL_SCANCODE_0: return '0';

	case SDL_SCANCODE_MINUS: return '-';
	case SDL_SCANCODE_EQUALS: return '=';
	case SDL_SCANCODE_LEFTBRACKET: return '[';
	case SDL_SCANCODE_RIGHTBRACKET: return ']';
	case SDL_SCANCODE_BACKSLASH: return '\\';
	case SDL_SCANCODE_NONUSHASH: return '#';
	case SDL_SCANCODE_SEMICOLON: return ';';
	case SDL_SCANCODE_APOSTROPHE: return '\'';
	case SDL_SCANCODE_GRAVE: return '`';
	case SDL_SCANCODE_COMMA: return ',';
	case SDL_SCANCODE_PERIOD: return '.';
	case SDL_SCANCODE_SLASH: return '/';
	case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

	case SDL_SCANCODE_BACKSPACE: return K_BACKSPACE;
	case SDL_SCANCODE_UP: return K_UPARROW;
	case SDL_SCANCODE_DOWN: return K_DOWNARROW;
	case SDL_SCANCODE_LEFT: return K_LEFTARROW;
	case SDL_SCANCODE_RIGHT: return K_RIGHTARROW;

	case SDL_SCANCODE_LALT: return K_ALT;
	case SDL_SCANCODE_RALT: return K_ALT;
	case SDL_SCANCODE_LCTRL: return K_CTRL;
	case SDL_SCANCODE_RCTRL: return K_CTRL;
	case SDL_SCANCODE_LSHIFT: return K_SHIFT;
	case SDL_SCANCODE_RSHIFT: return K_SHIFT;

	case SDL_SCANCODE_F1: return K_F1;
	case SDL_SCANCODE_F2: return K_F2;
	case SDL_SCANCODE_F3: return K_F3;
	case SDL_SCANCODE_F4: return K_F4;
	case SDL_SCANCODE_F5: return K_F5;
	case SDL_SCANCODE_F6: return K_F6;
	case SDL_SCANCODE_F7: return K_F7;
	case SDL_SCANCODE_F8: return K_F8;
	case SDL_SCANCODE_F9: return K_F9;
	case SDL_SCANCODE_F10: return K_F10;
	case SDL_SCANCODE_F11: return K_F11;
	case SDL_SCANCODE_F12: return K_F12;
	case SDL_SCANCODE_INSERT: return K_INS;
	case SDL_SCANCODE_DELETE: return K_DEL;
	case SDL_SCANCODE_PAGEDOWN: return K_PGDN;
	case SDL_SCANCODE_PAGEUP: return K_PGUP;
	case SDL_SCANCODE_HOME: return K_HOME;
	case SDL_SCANCODE_END: return K_END;

	case SDL_SCANCODE_NUMLOCKCLEAR: return KP_NUMLOCK;
	case SDL_SCANCODE_KP_DIVIDE: return KP_SLASH;
	case SDL_SCANCODE_KP_MULTIPLY: return KP_STAR;
	case SDL_SCANCODE_KP_MINUS: return KP_MINUS;
	case SDL_SCANCODE_KP_7: return KP_HOME;
	case SDL_SCANCODE_KP_8: return KP_UPARROW;
	case SDL_SCANCODE_KP_9: return KP_PGUP;
	case SDL_SCANCODE_KP_PLUS: return KP_PLUS;
	case SDL_SCANCODE_KP_4: return KP_LEFTARROW;
	case SDL_SCANCODE_KP_5: return KP_5;
	case SDL_SCANCODE_KP_6: return KP_RIGHTARROW;
	case SDL_SCANCODE_KP_1: return KP_END;
	case SDL_SCANCODE_KP_2: return KP_DOWNARROW;
	case SDL_SCANCODE_KP_3: return KP_PGDN;
	case SDL_SCANCODE_KP_ENTER: return KP_ENTER;
	case SDL_SCANCODE_KP_0: return KP_INS;
	case SDL_SCANCODE_KP_PERIOD: return KP_DEL;

	case SDL_SCANCODE_PAUSE: return K_PAUSE;

	default: return 0;
	}
}

void IN_Move (usercmd_t *cmd)
{
	if (mouseactive && (tas_playing.value == 0 || tas_gamestate == paused))
	{
		IN_MouseMove (cmd);
	}

	IN_Move_Hook(cmd);
}

void IN_Init (void)
{
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}
