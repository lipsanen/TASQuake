#include "cpp_quakedef.hpp"

struct SimulationInfo
{
	edict_t ent;
	double time;
	double hfr;
	float pitch;
	float yaw;
	float smove; 
	float fmove; 
	float upmove;
};

SimulationInfo GetCurrentStatus();
void SimulateFrame(SimulationInfo& info);