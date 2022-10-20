#pragma once

#include "quakedef.h"
#include <SDL2/SDL.h>

int IN_SDL_Event(SDL_Event* event);
void IN_MouseMove(usercmd_t *cmd);
void IN_ActivateMouse ();
void IN_DeactivateMouse ();

extern qboolean mouseactive;
