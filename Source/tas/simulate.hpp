#pragma once

#include "cpp_quakedef.hpp"
#include "strafing.hpp"

struct SimulationInfo
{
	edict_t ent;
	vec3_t angles;
	double time;
	double hfr;
	float smove; 
	float fmove; 
	float upmove;
	float jumpflag;
	bool jump;
};

SimulationInfo Get_Current_Status();
void SimulateFrame(SimulationInfo& info);
void SimulateWithStrafe(SimulationInfo& info, const StrafeVars& vars);