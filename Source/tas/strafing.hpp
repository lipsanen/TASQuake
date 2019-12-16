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
enum class StrafeType { None = 0, MaxAccel = 1, Straight = 3 };

PlayerData GetPlayerData();
void Strafe(usercmd_t* cmd);
void Strafe_Jump_Check();
void IN_TAS_Jump_Down(void);
void IN_TAS_Jump_Up(void);
void IN_TAS_Lgagst_Down(void);
void IN_TAS_Lgagst_Up(void);
void Cmd_Print_Vel(void);
void Cmd_Print_Origin(void);
void Cmd_Print_Moves(void);