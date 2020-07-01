#pragma once

#include "cpp_quakedef.hpp"

#include "script_playback.hpp"
#include "strafing.hpp"

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
	bool collision;

	// Low level stuff that is modified before the frame
	float smove;
	float fmove;
	float upmove;
	vec3_t viewangles;

	// Input stuff
	StrafeVars vars;
	double host_frametime;

	// Jump toggles
	bool tas_lgagst;
	bool tas_jump;

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

struct Simulator
{
	SimulationInfo info;
	int frame;

	void RunFrame(const std::string& cmd);
	void RunFrame();
	static Simulator GetSimulator();
};

SimulationInfo Get_Sim_Info();
void SimulateFrame(SimulationInfo& info);
void SimulateWithStrafe(SimulationInfo& info);
bool Should_Jump(const SimulationInfo& info);
void Simulate_Frame_Hook();
extern cvar_t tas_predict_per_frame, tas_predict, tas_predict_amount;
