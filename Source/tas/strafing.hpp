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
	double view_theta;
	double vel_theta;
};

extern cvar_t tas_strafe;
extern cvar_t tas_strafe_type;
extern cvar_t tas_strafe_yaw;
extern cvar_t tas_strafe_lgagst_speed;
extern cvar_t tas_view_yaw;
extern cvar_t tas_view_pitch;
extern cvar_t tas_anglespeed;
extern cvar_t tas_strafe_version;
extern const float INVALID_ANGLE;
enum class StrafeType { None = 0, MaxAccel = 1, MaxAngle = 2, Straight = 3 };

struct StrafeVars
{
	float tas_view_yaw;
	float tas_view_pitch;
	int tas_strafe;
	float tas_strafe_yaw;
	StrafeType tas_strafe_type;
	float tas_anglespeed;
	double host_frametime;

	StrafeVars();
};

StrafeVars Get_Current_Vars();
PlayerData GetPlayerData();
PlayerData GetPlayerData(edict_t* player, const StrafeVars& vars);
void StrafeSim(usercmd_t* cmd, edict_t* player, float *yaw, float *pitch, const StrafeVars& vars);
void Strafe(usercmd_t* cmd);
void Strafe_Jump_Check();
void IN_TAS_Jump_Down(void);
void IN_TAS_Jump_Up(void);
void IN_TAS_Lgagst_Down(void);
void IN_TAS_Lgagst_Up(void);
void Cmd_Print_Vel(void);
void Cmd_Print_Origin(void);
void Cmd_Print_Moves(void);