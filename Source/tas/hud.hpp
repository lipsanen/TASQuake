#pragma once

#include "cpp_quakedef.hpp"

// desc: Display position in HUD
extern cvar_t tas_hud_pos;

// desc: Displays xyz-velocity in HUD
extern cvar_t tas_hud_vel;
// desc: Displays 2d speed in HUD
extern cvar_t tas_hud_vel2d;
// desc: Displays 3d speed in HUD
extern cvar_t tas_hud_vel3d;
// desc: Displays velocity angle in HUD
extern cvar_t tas_hud_velang;
// desc: View angles element
extern cvar_t tas_hud_angles;
// desc: Displays player flags in HUD
extern cvar_t tas_hud_pflags;
// desc: Displays frame number in HUD
extern cvar_t tas_hud_frame;
// desc: Displays block number in HUD
extern cvar_t tas_hud_block;
// desc: X position of the HUD
extern cvar_t tas_hud_pos_x;
// desc: Y position of the HUD
extern cvar_t tas_hud_pos_y;
// desc: The vertical spacing between TAS HUD elements
extern cvar_t tas_hud_pos_inc;
// desc: Displays lots of TAS info about frameblocks in HUD
extern cvar_t tas_hud_state;
// desc: Displays waterlevel in HUD
extern cvar_t tas_hud_waterlevel;
//desc: Displays a bar that tells you how well you are strafing.
extern cvar_t tas_hud_strafe;
//desc: Displays movemessages sent
extern cvar_t tas_hud_movemessages;
//desc: Draws output of strafe algorithm on previous frame
extern cvar_t tas_hud_strafeinfo;
//desc: Displays the index of the RNG
extern cvar_t tas_hud_rng;
//desc: Displays the number of particles alive
extern cvar_t tas_hud_particles;
//desc: Displays the optimizer HUD
extern cvar_t tas_hud_optimizer;

void HUD_Draw_Hook();
