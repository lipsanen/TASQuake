#pragma once

#include "cpp_quakedef.hpp"
#include "strafing.hpp"
#include "script_playback.hpp"

struct KeyState
{
	float state;
};

struct SimulationInfo
{
	// The player entity
	edict_t ent;

	// Set during the frame	
	float jumpflag;
	double time;

	// Low level stuff that is modified before the frame
	float smove;
	float fmove;
	float upmove;
	vec3_t angles;

	// Input stuff
	StrafeVars vars;
	double host_frametime;

	// Raw cvars
	float cl_forwardspeed;
	float cl_sidespeed;
	float cl_upspeed;
	float cl_movespeedkey;

	KeyState key_jump;
	KeyState key_up;
	KeyState key_down;
	KeyState key_forward;
	KeyState key_moveleft;
	KeyState key_moveright;
	KeyState key_back;
	KeyState key_speed;
};

SimulationInfo Get_Current_Status();
void ApplyFrameblock(SimulationInfo& info, const FrameBlock& block);
void SimulateFrame(SimulationInfo& info);
void SimulateWithStrafe(SimulationInfo& info, const StrafeVars& vars);