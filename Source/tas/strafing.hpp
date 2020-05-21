#pragma once

#include "cpp_quakedef.hpp"

struct PlayerData
{
	double accelerate;
	bool onGround;
	float origin[3];
	float velocity[3];
	double entFriction;
	double frameTime;
	double wishspeed;
	float vel2d;
	double vel_theta;
};

extern cvar_t tas_strafe;
extern cvar_t tas_strafe_type;
extern cvar_t tas_strafe_yaw;
extern cvar_t tas_strafe_pitch;
extern cvar_t tas_strafe_lgagst_speed;
extern cvar_t tas_view_yaw;
extern cvar_t tas_view_pitch;
extern cvar_t tas_anglespeed;
extern cvar_t tas_strafe_version;
extern const float INVALID_ANGLE;
enum class StrafeType
{
	None = 0,
	MaxAccel = 1,
	MaxAngle = 2,
	Straight = 3,
	Swim = 4,
	Reverse = 5
};

struct StrafeVars
{
	float tas_view_yaw;
	float tas_view_pitch;
	int tas_strafe;
	float tas_strafe_yaw;
	float tas_strafe_pitch;
	StrafeType tas_strafe_type;
	float tas_anglespeed;
	double host_frametime;
	int tas_strafe_version;
	bool simulated;

	StrafeVars();
};

struct MoveInfo {
	int fmove;
	int smove;
	int upmove;
	float yaw;
	float pitch;
	bool jump;
};

bool Is_TAS_Jump_Down();
bool Is_TAS_Lgagst_Down();
StrafeVars Get_Current_Vars();
PlayerData GetPlayerData();
PlayerData GetPlayerData(edict_t* player, const StrafeVars& vars);
void StrafeSim(usercmd_t* cmd, edict_t* player, float* yaw, float* pitch, const StrafeVars& vars);
void Strafe(usercmd_t* cmd);
void Strafe_Jump_Check();
// desc: Autojump
void IN_TAS_Jump_Down(void);
void IN_TAS_Jump_Up(void);
// desc: LGAGST = leave ground at air-ground speed threshold. Use in TASing to automatically jump \
when it's faster to strafe in the air. Also always jumps when you walk off an edge.
void IN_TAS_Lgagst_Down(void);
void IN_TAS_Lgagst_Up(void);
// desc: Prints velocity on next physics frame
void Cmd_TAS_Print_Vel(void);
// desc: Prints origin on next physics frame
void Cmd_TAS_Print_Origin(void);
double MaxAccelTheta(const PlayerData& data, const StrafeVars& vars);
MoveInfo GetPrevMoveInfo();