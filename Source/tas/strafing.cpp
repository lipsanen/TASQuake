#include "strafing.hpp"
#include <cmath>
#include "afterframes.hpp"
#include "utils.hpp"

cvar_t tas_strafe = { "tas_strafe", "0" };
cvar_t tas_strafe_yaw = {"tas_strafe_yaw", "0"};
cvar_t tas_strafe_yaw_offset = { "tas_strafe_yaw_offset", "0" };

static bool shouldJump = false;
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

static PlayerData GetPlayerData()
{
	PlayerData data;
	
	memcpy(data.origin, sv_player->v.origin, sizeof(float) * 3);
	memcpy(data.velocity, sv_player->v.velocity, sizeof(float) * 3);

	data.onGround = ((int)sv_player->v.flags & FL_ONGROUND) != 0;
	data.accelerate = sv_accelerate.value;
	data.entFriction = Get_EntFriction(data.velocity, data.origin);
	data.frameTime = host_frametime;
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
		data.vel_theta = NormalizeRad(tas_strafe_yaw.value);

	return data;
}

static double MaxAccelTheta(const PlayerData& data)
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

static double MaxAccelIntoYawAngle(const PlayerData& data)
{
	double target_theta = tas_strafe_yaw.value * M_DEG2RAD;
	double theta = MaxAccelTheta(data);
	double diff = NormalizeRad(target_theta - data.vel_theta);
	double out = std::copysign(theta, diff) * M_RAD2DEG;

	return out;
}

static void StrafeMaxAccel(usercmd_t* cmd)
{
	auto data = GetPlayerData();
	double yaw = MaxAccelIntoYawAngle(data);

	float lookdir = tas_strafe_yaw.value + tas_strafe_yaw_offset.value;
	lookdir = NormalizeDeg(lookdir);
	lookdir = AngleModDeg(lookdir);

	double vel_yaw = data.vel_theta * M_RAD2DEG;

	yaw = vel_yaw + yaw;
	double diff = NormalizeDeg(lookdir - yaw) * M_DEG2RAD;

	double fmove = std::cos(diff) * sv_maxspeed.value;
	double smove = std::sin(diff) * sv_maxspeed.value;
	ApproximateRatioWithIntegers(fmove, smove, 32767);

	cmd->forwardmove = fmove;
	cmd->sidemove = smove;
	cl.viewangles[YAW] = ToQuakeAngle(lookdir);
}

static void StrafeStraight(usercmd_t* cmd)
{
	cmd->forwardmove = sv_maxspeed.value;
	cmd->sidemove = 0;
	cl.viewangles[YAW] = ToQuakeAngle(tas_strafe_yaw.value);
}

void Strafe(usercmd_t* cmd)
{
	StrafeType strafeType = (StrafeType)static_cast<int>(tas_strafe.value);

	if (strafeType == StrafeType::MaxAccel)
	{
		StrafeMaxAccel(cmd);
	}
	else if (strafeType == StrafeType::Straight)
	{
		StrafeStraight(cmd);
	}

	if (print_moves)
	{
		Con_Printf("fmove: %.3f, smove %.3f, yaw %.3f\n", cmd->forwardmove, cmd->sidemove, cmd->viewangles[PITCH]);
		print_moves = false;
	}

}

void Strafe_Jump_Check()
{
	auto data = GetPlayerData();
	const float lgagst = 460.0f;
	bool wantsToJump = (data.vel2d > lgagst && tas_strafe.value == 1 && tas_lgagst) || autojump;

	if (wantsToJump && data.onGround)
	{
		AddAfterframes(0, "+jump");
		shouldJump = true;
	}
	else if(shouldJump)
	{
		AddAfterframes(0, "-jump");
		shouldJump = false;
	}

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
