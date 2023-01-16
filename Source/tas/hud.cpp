#include "hud.hpp"

#include "hooks.h"

#include "script_playback.hpp"
#include "optimizer.hpp"
#include "strafing.hpp"
#include "ipc_prediction.hpp"
#include "real_prediction.hpp"
#include "prediction.hpp"
#include "libtasquake/utils.hpp"

cvar_t tas_hud_pos = {"tas_hud_pos", "0"};
cvar_t tas_hud_angles = {"tas_hud_angles", "0"};
cvar_t tas_hud_vel = {"tas_hud_vel", "0"};
cvar_t tas_hud_vel2d = {"tas_hud_vel2d", "0"};
cvar_t tas_hud_vel3d = {"tas_hud_vel3d", "0"};
cvar_t tas_hud_velang = {"tas_hud_velang", "0"};
cvar_t tas_hud_frame = {"tas_hud_frame", "0"};
cvar_t tas_hud_block = {"tas_hud_block", "0"};
cvar_t tas_hud_state = {"tas_hud_state", "0"};
cvar_t tas_hud_pflags = {"tas_hud_pflags", "0"};
cvar_t tas_hud_waterlevel = {"tas_hud_waterlevel", "0"};
cvar_t tas_hud_pos_x = {"tas_hud_pos_x", "10"};
cvar_t tas_hud_pos_y = {"tas_hud_pos_y", "60"};
cvar_t tas_hud_pos_inc = {"tas_hud_pos_inc", "8"};
cvar_t tas_hud_strafe = {"tas_hud_strafe", "0"};
cvar_t tas_hud_movemessages = {"tas_hud_movemessages", "0"};
cvar_t tas_hud_strafeinfo = {"tas_hud_strafeinfo", "0"};
cvar_t tas_hud_rng = {"tas_hud_rng", "0"};
cvar_t tas_hud_particles = { "tas_hud_particles", "0" };
cvar_t tas_hud_optimizer = { "tas_hud_optimizer", "0" };
cvar_t tas_hud_prediction_type = { "tas_hud_prediction_type", "0"};

void Draw(int& y, cvar_t* cvar, const char* format, ...)
{
	if (!cvar->value)
		return;

	static char BUFFER[128];
	va_list args;
	va_start(args, format);
	vsnprintf(BUFFER, ARRAYSIZE(BUFFER), format, args);
	Draw_String((int)tas_hud_pos_x.value, y, BUFFER);
	y += tas_hud_pos_inc.value;
}

void Draw_Fill_RGB(int x, int y, int w, int h, float r, float g, float b)
{
	Q_glDisable(GL_TEXTURE_2D);
	glColor3f(r, g, b);

	Q_glBegin(GL_QUADS);
	Q_glVertex2f(x, y);
	Q_glVertex2f(x + w, y);
	Q_glVertex2f(x + w, y + h);
	Q_glVertex2f(x, y + h);
	Q_glEnd();

	Q_glEnable(GL_TEXTURE_2D);
	Q_glColor3ubv(color_white);
}

bool Should_Print_Cvar(const std::string& name, float value)
{
	auto cvar = Cvar_FindVar(const_cast<char*>(name.c_str()));
	float default_value;
	sscanf(cvar->defaultvalue, "%f", &default_value);

	return cvar != NULL && default_value != value;
}

static void DrawOptimizerState(int& y, const PlaybackInfo* info) {
	if (!tas_hud_optimizer.value || info->Get_Last_Frame() == 0 || !tas_playing.value)
		return;
	
	Draw(y, &tas_hud_optimizer, "Optimizer:");
	Draw(y, &tas_hud_optimizer, "Goal: %s", TASQuake::OptimizerGoalStr());
	Draw(y, &tas_hud_optimizer, "Original: %f", TASQuake::OriginalEfficacy());
	Draw(y, &tas_hud_optimizer, "Optimized: %f", TASQuake::OptimizedEfficacy());
	Draw(y, &tas_hud_optimizer, "Iterations: %lu", TASQuake::OptimizerIterations());
}

void DrawFrameState(int& y, const PlaybackInfo* info)
{
	if (!tas_hud_state.value || info->Get_Last_Frame() == 0 || !tas_playing.value)
		return;

	auto current_block = info->Get_Current_Block();

	if (current_block && current_block->frame != info->current_frame)
		current_block = nullptr;

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Cvars:");
	for (auto& cvar : info->stacked.convars)
	{
		if (current_block && current_block->HasConvar(cvar.first)
		    && current_block->convars.at(cvar.first) != cvar.second)
		{
			Draw(y,
			     &tas_hud_state,
			     "%s %.3f -> %.3f",
			     cvar.first.c_str(),
			     cvar.second,
			     current_block->convars.at(cvar.first));
		}
		else
		{
			if (Should_Print_Cvar(cvar.first, cvar.second))
				Draw(y, &tas_hud_state, "%s %.3f", cvar.first.c_str(), cvar.second);
		}
	}

	if (current_block)
	{
		for (auto& cvar : current_block->convars)
		{
			if (!info->stacked.HasConvar(cvar.first) && Should_Print_Cvar(cvar.first, cvar.second))
			{
				Draw(y, &tas_hud_state, "%s -> %.3f", cvar.first.c_str(), cvar.second);
			}
		}
	}

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Toggles:");
	for (auto& toggle : info->stacked.toggles)
	{
		if (toggle.second)
		{
			if (current_block && current_block->HasToggle(toggle.first))
				Draw(y, &tas_hud_state, "+%s -> -%s", toggle.first.c_str(), toggle.first.c_str());
			else
				Draw(y, &tas_hud_state, "+%s", toggle.first.c_str());
		}
	}

	if (current_block)
	{
		for (auto& toggle : current_block->toggles)
		{
			if (toggle.second)
			{
				Draw(y, &tas_hud_state, "-%s -> +%s", toggle.first.c_str(), toggle.first.c_str());
			}
		}
	}

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Commands:");

	if (current_block)
	{
		for (auto& cmd : current_block->commands)
		{
			Draw(y, &tas_hud_state, "%s", cmd.c_str());
		}
	}
}

void Draw_PFlags(int& y)
{
	if (!tas_hud_pflags.value)
		return;
	static char BUFFER[64];

	strcpy(BUFFER, "Flags: ");
	int index = 7;

	for (int flag = 4096; flag >= 1; flag >>= 1)
	{
		BUFFER[index++] = ((int)sv_player->v.flags & flag ? '1' : '0');
	}

	BUFFER[index++] = '\0';

	Draw(y, &tas_hud_pflags, BUFFER);
	Draw(y, &tas_hud_pflags, "       JWPOINGMICCSF");
	Draw(y, &tas_hud_pflags, "       RJGGTTMOWLOWL");
}

void Draw_VelAngle(int& y, const PlayerData& player_data)
{
	if (!tas_hud_velang.value)
		return;

	vec3_t ang;
	vectoangles((vec_t*)player_data.velocity, ang);
	Draw(y, &tas_hud_velang, "velang: (%.3f, %.3f)", -ang[PITCH], ang[YAW]);
}

static float FwdScale()
{
	vec3_t angles;
	vec3_t fwd;

	angles[PITCH] = AngleModDeg(cl.viewangles[PITCH]);
	angles[YAW] = cl.viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, fwd, NULL, NULL);

	return VectorLength2D(fwd);
}

static float AccelTheta(float fwd_scale, const PlayerData& player_data, const StrafeVars& vars)
{
	static float prev_yaw = 0;
	static float current_yaw = 0;
	float fmove = 0;
	float smove = 0;
	float speed_modifier = ((in_speed.state & 1) != 0) ? cl_movespeedkey.value : 1;

	if (current_yaw != cl.viewangles[YAW])
	{
		prev_yaw = current_yaw;
		current_yaw = cl.viewangles[YAW];
	}

	if (vars.tas_strafe)
	{
		usercmd_t cmd;
		StrafeSim(&cmd, sv_player, &cl.viewangles[YAW], &cl.viewangles[PITCH], vars);
		fmove = cmd.forwardmove * fwd_scale;
		smove = cmd.sidemove;

		return atan2f(-smove, fmove) + cl.viewangles[YAW] * M_DEG2RAD;
	}
	else
	{
		float predicted_yaw = current_yaw + (NormalizeDeg(NormalizeDeg(current_yaw) - NormalizeDeg(prev_yaw)));
		if ((in_forward.state & 1) != 0)
			fmove += cl_forwardspeed.value * speed_modifier * fwd_scale;
		if ((in_back.state & 1) != 0)
			fmove -= cl_backspeed.value * speed_modifier * fwd_scale;
		if ((in_moveright.state & 1) != 0)
			smove += cl_sidespeed.value * speed_modifier;
		if ((in_moveleft.state & 1) != 0)
			smove -= cl_sidespeed.value * speed_modifier;

		return atan2f(-smove, fmove) + predicted_yaw * M_DEG2RAD;
	}
}

void Draw_StrafeStuff(const PlayerData& player_data)
{
	int x, y;

	if (cl.intermission || !tas_hud_strafe.value)
		return;

	int position;
	float fwd_scale = FwdScale();
	StrafeVars vars = Get_Current_Vars();
	float scale;

	if (player_data.onGround)
	{
		scale = 15.0f * M_DEG2RAD;
	}
	else
	{
		scale = 4.5f * M_DEG2RAD;
	}

	float vel_theta;
	if (player_data.vel2d < 0.01)
		vel_theta = cl.viewangles[YAW] * M_DEG2RAD;
	else
		vel_theta = player_data.vel_theta;

	float accel_theta = NormalizeRad(AccelTheta(fwd_scale, player_data, vars));
	float diff = abs(accel_theta - vel_theta);
	float angle = MaxAccelTheta(player_data, vars);

	position = (diff - angle) / scale * 80 + 80;
	position = bound(0, position, 160);

	x = vid.width / 2 - 80;
	y = vid.height / 2 + 60;

	Draw_Fill(x, y - 1, 160, 1, 10);
	Draw_Fill(x, y + 9, 160, 1, 10);
	Draw_Fill(x, y, 160, 9, 52);
	Draw_Fill(x + 78, y, 5, 9, 10);
	Draw_Fill_RGB(x + position - 1, y, 3, 9, 0, 1, 0);
}


void Draw_StrafeInfo(int& y)
{
	if (!tas_hud_strafeinfo.value)
		return;

	auto info = GetPrevMoveInfo();

	Draw(y, &tas_hud_strafeinfo, "moves: (%d, %d, %d)", info.fmove, info.smove, info.upmove);
	Draw(y, &tas_hud_strafeinfo, "angles: (%f, %f)", info.pitch, info.yaw);
	Draw(y, &tas_hud_strafeinfo, "jump: %d", info.jump ? 1 : 0);
}

static void DrawParticleCount(int& y)
{
	if(!tas_hud_particles.value)
		return;

	int count = 0;

	r_particle_t* p = r_free_particles;

	for(p=r_free_particles; p; p = p->next, ++count);
	Draw(y, &tas_hud_particles, "particles: %d", count);
}

static void DrawPredictionType(int& y)
{
	if(!tas_hud_prediction_type.value)
		return;

	if(IPC_Prediction_HasLine()) {
		Draw(y, &tas_hud_prediction_type, "prediction line: multi-game");
	} else if(GamePrediction_HasLine()) {
		Draw(y, &tas_hud_prediction_type, "prediction line: game");
	} else if(Prediction_HasLine()) {
		Draw(y, &tas_hud_prediction_type, "prediction line: simulated");
	} else {
		Draw(y, &tas_hud_prediction_type, "prediction line: none");
	}
}

void HUD_Draw_Hook()
{
#ifdef SIM
	return;
#else
	if (!sv.active)
		return;
#endif

	int x = tas_hud_pos_x.value;
	int y = tas_hud_pos_y.value;

	auto player_data = GetPlayerData();
	auto info = GetPlaybackInfo();
	int last_frame = info->Get_Last_Frame();
	int current_block_no = info->GetBlockNumber();
	int blocks = info->Get_Number_Of_Blocks();

	Draw(y,
	     &tas_hud_pos,
	     "pos: (%.3f, %.3f, %.3f)",
	     player_data.origin[0],
	     player_data.origin[1],
	     player_data.origin[2]);
	Draw(y, &tas_hud_angles, "ang: (%.3f, %.3f, %.3f)", cl.viewangles[0], cl.viewangles[1], cl.viewangles[2]);
	Draw(y,
	     &tas_hud_vel,
	     "vel: (%.3f, %.3f, %.3f)",
	     player_data.velocity[0],
	     player_data.velocity[1],
	     player_data.velocity[2]);
	Draw(y, &tas_hud_vel2d, "vel2d: %.3f", player_data.vel2d);
	Draw(y, &tas_hud_vel3d, "vel3d: %.3f", VectorLength(player_data.velocity));
	Draw_VelAngle(y, player_data);
	Draw(y, &tas_hud_frame, "frame: %d / %d", info->current_frame, last_frame);
	Draw(y, &tas_hud_block, "block: %d / %d", current_block_no, blocks - 1);
	Draw(y, &tas_hud_waterlevel, "waterlevel: %d", (int)sv_player->v.waterlevel);
	Draw(y, &tas_hud_movemessages, "cl.movemessages: %d", cl.movemessages);
	Draw(y, &tas_hud_rng, "rng index: %d", Get_RNG_Index());
	DrawPredictionType(y);
	DrawOptimizerState(y, info);
	DrawParticleCount(y);
	Draw_PFlags(y);
	DrawFrameState(y, info);
	Draw_StrafeStuff(player_data);
	Draw_StrafeInfo(y);
}
