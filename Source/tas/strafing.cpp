#include "strafing.hpp"
#include <cmath>
#include "afterframes.hpp"
#include "utils.hpp"

enum class StrafeType { None = 0, MaxAccel, Straight };

cvar_t tas_strafe_lgagst = {"tas_strafe_lgagst", "0"};
cvar_t tas_strafe = { "tas_strafe", "0" };
cvar_t tas_strafe_yaw = {"tas_strafe_yaw", "0"};
cvar_t tas_strafe_yaw_offset = { "tas_strafe_yaw_offset", "0" };

static bool shouldJump = false;
static bool autojump = false;

void IN_TAS_Jump_Down(void)
{
	autojump = true;
}

void IN_TAS_Jump_Up(void)
{
	autojump = false;
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


	if (data.onGround)
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

	return data;
}

static double MaxAccelTheta(const PlayerData& data)
{
	double accelspeed = data.accelerate * data.wishspeed * data.entFriction * data.frameTime;
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
	double vel_theta = 0;
	double target_theta = tas_strafe_yaw.value * M_DEG2RAD;
	if (!IsZero(data.vel2d))
		vel_theta = std::atan2(data.velocity[1], data.velocity[0]);
	else
		vel_theta = target_theta;

	double theta = MaxAccelTheta(data);

	return std::copysign(theta, NormalizeRad(target_theta - vel_theta)) * M_RAD2DEG;
}

static void StrafeMaxAccel(usercmd_t* cmd)
{
	auto data = GetPlayerData();
	double yaw = MaxAccelIntoYawAngle(data);
	float lookdir = AngleModDeg(tas_strafe_yaw.value + tas_strafe_yaw_offset.value);

	double vel_yaw;
	if (IsZero(data.vel2d))
		vel_yaw = tas_strafe_yaw.value;
	else
		vel_yaw = std::atan2(data.velocity[1], data.velocity[0]) * M_RAD2DEG;

	yaw = NormalizeDeg(vel_yaw + yaw);
	double diff = (lookdir - yaw) * M_DEG2RAD;

	double fmove = std::cos(diff) * sv_maxspeed.value;
	double smove = std::sin(diff) * sv_maxspeed.value;
	ApproximateRatioWithIntegers(fmove, smove, 32767);

	cmd->forwardmove = fmove;
	cmd->sidemove = smove;
	cl.viewangles[YAW] = lookdir;
}

static void StrafeStraight(usercmd_t* cmd)
{
	cmd->forwardmove = sv_maxspeed.value;
	cmd->sidemove = 0;
	cl.viewangles[YAW] = tas_strafe_yaw.value;
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
}

void Strafe_Jump_Check()
{
	auto data = GetPlayerData();
	const float lgagst = 460.0f;
	bool wantsToJump = (data.vel2d > lgagst && tas_strafe.value == 1 && tas_strafe_lgagst.value) || autojump;

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
}
