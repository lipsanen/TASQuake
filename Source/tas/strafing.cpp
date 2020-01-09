﻿#include "strafing.hpp"
#include <cmath>
#include "afterframes.hpp"
#include "utils.hpp"
#include "hooks.h"
#include "simulate.hpp"

cvar_t tas_strafe = { "tas_strafe", "0" };
cvar_t tas_strafe_type = { "tas_strafe_type", "1" };
cvar_t tas_strafe_yaw = { "tas_strafe_yaw", "0" };
cvar_t tas_strafe_yaw_offset = { "tas_strafe_yaw_offset", "0" };
cvar_t tas_strafe_lgagst_speed = { "tas_strafe_lgagst_speed", "460" };
cvar_t tas_view_yaw = { "tas_view_yaw", "999" };
cvar_t tas_view_pitch = { "tas_view_pitch", "999" };
cvar_t tas_anglespeed = { "tas_anglespeed", "5" };
cvar_t tas_strafe_version = { "tas_strafe_version", "2"};
const float INVALID_ANGLE = 999;

static bool autojump = false;
static bool tas_lgagst = false;
static bool print_origin = false;
static bool print_vel = false;
static bool print_moves = false;

void IN_TAS_Jump_Down(void)
{
	autojump = true;
}

void IN_TAS_Jump_Up(void)
{
	autojump = false;
}

void IN_TAS_Lgagst_Down(void)
{
	tas_lgagst = true;
}

void IN_TAS_Lgagst_Up(void)
{
	tas_lgagst = false;
}

void Cmd_Print_Vel(void)
{
	print_vel = true;
}

void Cmd_Print_Origin(void)
{
	print_origin = true;
}

void Cmd_Print_Moves(void)
{
	print_moves = true;
}

static float Get_EntFriction(float* vel, float* player_origin)
{
	float	speed;
	vec3_t	start, stop;
	trace_t	trace;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);

	// if the leading edge is over a dropoff, increase friction
	start[0] = stop[0] = player_origin[0] + vel[0] / speed * 16;
	start[1] = stop[1] = player_origin[1] + vel[1] / speed * 16;
	start[2] = player_origin[2] + sv_player->v.mins[2];
	stop[2] = start[2] - 34;

	trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, sv_player);

	if (trace.fraction == 1.0)
		return sv_edgefriction.value;
	else
		return 1;
}

bool Is_TAS_Jump_Down()
{
	return autojump;
}

bool Is_TAS_Lgagst_Down()
{
	return tas_lgagst;
}

StrafeVars Get_Current_Vars()
{
	StrafeVars vars;
	vars.host_frametime = host_frametime;
	vars.tas_anglespeed = tas_anglespeed.value;
	vars.tas_strafe = (int)tas_strafe.value;
	vars.tas_strafe_type = static_cast<StrafeType>((int)tas_strafe_type.value);
	vars.tas_strafe_yaw = (float)tas_strafe_yaw.value;
	vars.tas_view_pitch = (float)tas_view_pitch.value;
	vars.tas_view_yaw = (float)tas_view_yaw.value;
	vars.tas_strafe_version = (int)tas_strafe_version.value;
	vars.simulated = false;
	return vars;
}

PlayerData GetPlayerData()
{
	StrafeVars vars = Get_Current_Vars();
	return GetPlayerData(sv_player, vars);
}

PlayerData GetPlayerData(edict_t* player, const StrafeVars& vars)
{
	PlayerData data;
	
	memcpy(data.origin, player->v.origin, sizeof(float) * 3);
	memcpy(data.velocity, player->v.velocity, sizeof(float) * 3);

	data.onGround = ((int)player->v.flags & FL_ONGROUND) != 0;
	data.accelerate = sv_accelerate.value;
	data.entFriction = Get_EntFriction(data.velocity, data.origin);
	data.frameTime = vars.host_frametime;
	data.wishspeed = sv_maxspeed.value;
	float vel2d = std::sqrt(data.velocity[0] * data.velocity[0] + data.velocity[1] * data.velocity[1]);


	if (data.onGround && std::abs(vel2d) > 0)
	{
		float control = (vel2d < sv_stopspeed.value) ? sv_stopspeed.value : vel2d;
		float newspeed = vel2d - data.frameTime * control * data.entFriction * sv_friction.value;

		if (newspeed < 0)
			newspeed = 0;
		newspeed /= vel2d;

		data.velocity[0] = data.velocity[0] * newspeed;
		data.velocity[1] = data.velocity[1] * newspeed;
		data.velocity[2] = data.velocity[2] * newspeed;
	}

	data.vel2d = std::sqrt(data.velocity[0] * data.velocity[0] + data.velocity[1] * data.velocity[1]);
	if(!IsZero(data.vel2d))
		data.vel_theta = NormalizeRad(std::atan2(data.velocity[1], data.velocity[0]));
	else
	{
		if (vars.tas_strafe_version == 1)
			data.vel_theta = NormalizeRad(vars.tas_strafe_yaw); // Old bugged vel theta calculation
		else
			data.vel_theta = NormalizeRad(vars.tas_strafe_yaw * M_DEG2RAD);
	}

	return data;
}

static double MaxAccelTheta(const PlayerData& data, const StrafeVars& vars)
{
	double accelspeed = data.accelerate * data.wishspeed * data.frameTime;
	if (accelspeed <= 0)
		return M_PI;

	if (IsZero(data.vel2d))
		return 0;

	double wishspeed_capped = data.onGround ? data.wishspeed : 30;
	double tmp = wishspeed_capped - accelspeed;
	if (tmp <= 0.0)
		return M_PI / 2;

	if (tmp < data.vel2d)
		return std::acos(tmp / data.vel2d);

	return 0.0;
}

static double MaxAngleTheta(const PlayerData& data, const StrafeVars& vars)
{
	double accelspeed = data.accelerate * data.wishspeed * data.frameTime;
	if (accelspeed <= 0)
	{
		accelspeed *= -1;
		double wishspeed_capped = data.onGround ? data.wishspeed : 30;

		if (accelspeed >= data.vel2d)
		{
			if(wishspeed_capped >= data.vel2d)
				return 0;
			else
				return std::acos(wishspeed_capped / data.vel2d);
		}
		else
		{
			if (wishspeed_capped >= data.vel2d)
				return std::acos(accelspeed / data.vel2d);
			else {
				return std::acos(min(accelspeed, wishspeed_capped) / data.vel2d);
			}
		}
	}
	else
	{
		if (accelspeed >= data.vel2d)
			return M_PI;
		else
			return std::acos(-1 * accelspeed / data.vel2d);
	}
}

static double MaxAccelIntoYawAngle(const PlayerData& data, const StrafeVars& vars)
{
	double target_theta = vars.tas_strafe_yaw * M_DEG2RAD;
	double theta = MaxAccelTheta(data, vars);
	double diff = NormalizeRad(target_theta - data.vel_theta);
	double out = std::copysign(theta, diff) * M_RAD2DEG;

	return out;
}

static double MaxAngleIntoYawAngle(const PlayerData& data, const StrafeVars& vars)
{
	double target_theta = vars.tas_strafe_yaw * M_DEG2RAD;
	double theta = MaxAngleTheta(data, vars);
	double diff = NormalizeRad(target_theta - data.vel_theta);
	double out = std::copysign(theta, diff) * M_RAD2DEG;

	return out;
}

static double StrafeMaxAccel(edict_t* player, const StrafeVars& vars)
{
	auto data = GetPlayerData(player, vars);
	double yaw = MaxAccelIntoYawAngle(data, vars);

	float target_dir = vars.tas_strafe_yaw;
	target_dir = NormalizeDeg(target_dir);
	target_dir = AngleModDeg(target_dir);

	double vel_yaw = data.vel_theta * M_RAD2DEG;

	return vel_yaw + yaw;
}

static double StrafeMaxAngle(edict_t* player, const StrafeVars& vars)
{
	auto data = GetPlayerData(player, vars);
	double yaw = MaxAngleIntoYawAngle(data, vars);

	float target_dir = vars.tas_strafe_yaw;
	target_dir = NormalizeDeg(target_dir);
	target_dir = AngleModDeg(target_dir);

	double vel_yaw = data.vel_theta * M_RAD2DEG;

	return vel_yaw + yaw;
}

static double StrafeStraight(const StrafeVars& vars)
{
	return vars.tas_strafe_yaw;
}

void StrafeInto(usercmd_t* cmd, double yaw, float view_yaw, const StrafeVars& vars)
{
	double lookyaw;

	if (vars.tas_strafe_version <= 1)
		lookyaw = NormalizeDeg(view_yaw);
	else
		lookyaw = NormalizeDeg(AngleModDeg(view_yaw));

	double diff = NormalizeDeg(lookyaw - yaw) * M_DEG2RAD;

	double fmove = std::cos(diff) * sv_maxspeed.value;
	double smove = std::sin(diff) * sv_maxspeed.value;
	ApproximateRatioWithIntegers(fmove, smove, 32767);
	cmd->forwardmove = fmove;
	cmd->sidemove = smove;
}

float MoveViewTowards(float target, float current, bool yaw, const StrafeVars& vars)
{
	float diff;
	if(yaw)
		diff = NormalizeDeg(target - current);
	else
		diff = target - current;
	float abs_diff = std::abs(diff);

	if(abs_diff < vars.tas_anglespeed)
		current = target;
	else
	{
		abs_diff = min(abs_diff, vars.tas_anglespeed);
		current += std::copysign(abs_diff, diff);
	}
	
	if (yaw)
		return ToQuakeAngle(current);
	else
		return current;

}

void SetView(float* yaw, float* pitch, const StrafeVars& vars)
{
	if(!vars.simulated && (sv.paused || tas_gamestate == paused || key_dest != key_game))
		return;

	if (vars.tas_view_pitch != INVALID_ANGLE)
	{
		*pitch = MoveViewTowards(vars.tas_view_pitch, *pitch, false, vars);
	}

	*yaw = NormalizeDeg(*yaw);
	float tas_yaw = NormalizeDeg(vars.tas_view_yaw);
	float strafe_yaw = NormalizeDeg(vars.tas_strafe_yaw);

	if (vars.tas_view_yaw != INVALID_ANGLE)
	{
		*yaw = MoveViewTowards(tas_yaw, *yaw, true, vars);
	}
	else if (vars.tas_strafe != 0 && vars.tas_view_yaw == INVALID_ANGLE)
	{
		*yaw = MoveViewTowards(strafe_yaw, *yaw, true, vars);
	}
}

/*
static SimulationInfo past;
static SimulationInfo present;*/


void Strafe(usercmd_t* cmd)
{
	auto vars = Get_Current_Vars();
	StrafeSim(cmd, sv_player, &cl.viewangles[YAW], &cl.viewangles[PITCH], vars);
}

void StrafeSim(usercmd_t* cmd, edict_t* player, float *yaw, float *pitch, const StrafeVars& vars)
{
	double strafe_yaw = 0;
	bool strafe = false;
	SetView(yaw, pitch, vars);

	if(vars.tas_strafe)
	{
		if (vars.tas_strafe_type == StrafeType::MaxAccel)
		{
			strafe_yaw = StrafeMaxAccel(player, vars);
			strafe = true;
		}
		else if (vars.tas_strafe_type == StrafeType::MaxAngle)
		{
			strafe_yaw = StrafeMaxAngle(player, vars);
			strafe = true;
		}
		else if (vars.tas_strafe_type == StrafeType::Straight)
		{
			strafe_yaw = StrafeStraight(vars);
			strafe = true;
		}

		if (strafe)
		{
			StrafeInto(cmd, strafe_yaw, *yaw, vars);
		}

	}
}

/*
void CheckDiff(vec3_t orig, vec3_t pred, const char* name)
{
	vec3_t vel_diff;
	VectorCopy(orig, vel_diff);
	VectorScale(vel_diff, -1, vel_diff);
	VectorAdd(vel_diff, pred, vel_diff);
	float length = VectorLength(vel_diff);

	if (length > 0.01)
	{
		Con_Printf("%s off by %.3f\n", name, length);
	}
}*/


void Strafe_Jump_Check()
{
	auto sim = Get_Sim_Info();
	bool jump = Should_Jump(sim);

	if (!jump && (in_jump.state & 1) == 1 && (sim.tas_jump || sim.tas_lgagst))
	{
		AddAfterframes(0, "-jump");
	}
	else if (jump && (in_jump.state & 1) == 0)
	{
		AddAfterframes(0, "+jump");
	}

	auto data = GetPlayerData();

	if (print_vel)
	{
		print_vel = false;
		Con_Printf("speed %.3f\n", data.vel2d);
		Con_Printf("vel (%.3f, %.3f, %.3f)\n", data.velocity[0], data.velocity[1], data.velocity[2]);
	}

	if (print_origin)
	{
		print_origin = false;
		Con_Printf("pos (%.3f, %.3f, %.3f)\n", data.origin[0], data.origin[1], data.origin[2]);
	}

}

StrafeVars::StrafeVars()
{
}
