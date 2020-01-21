#pragma once

#include "cpp_quakedef.hpp"

extern cvar_t tas_hud_pos;
extern cvar_t tas_hud_vel;
extern cvar_t tas_hud_vel2d;
extern cvar_t tas_hud_vel3d;
extern cvar_t tas_hud_velang;
extern cvar_t tas_hud_angles;
extern cvar_t tas_hud_pflags;
extern cvar_t tas_hud_frame;
extern cvar_t tas_hud_block;
extern cvar_t tas_hud_pos_x;
extern cvar_t tas_hud_pos_y;
extern cvar_t tas_hud_pos_inc;
extern cvar_t tas_hud_state;
extern cvar_t tas_hud_waterlevel;
extern cvar_t tas_hud_strafe;

void HUD_Draw_Hook();
