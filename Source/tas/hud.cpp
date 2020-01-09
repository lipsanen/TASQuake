#include "hud.hpp"
#include "strafing.hpp"
#include "script_playback.hpp"
#include "hooks.h"

cvar_t tas_hud_pos = {"tas_hud_pos", "0"};
cvar_t tas_hud_angles = {"tas_hud_angles", "0"};
cvar_t tas_hud_vel = {"tas_hud_vel", "0"};
cvar_t tas_hud_vel2d = {"tas_hud_vel2d", "0"};
cvar_t tas_hud_vel3d = {"tas_hud_vel3d", "0"};
cvar_t tas_hud_frame = {"tas_hud_frame", "0"};
cvar_t tas_hud_block = {"tas_hud_block", "0"};
cvar_t tas_hud_state = {"tas_hud_state", "0"};
cvar_t tas_hud_pflags = {"tas_hud_pflags", "0"};
cvar_t tas_hud_waterlevel = {"tas_hud_waterlevel", "0"};

cvar_t tas_hud_pos_x = { "tas_hud_pos_x", "10" };
cvar_t tas_hud_pos_y = { "tas_hud_pos_y", "60" };
cvar_t tas_hud_pos_inc = { "tas_hud_pos_inc", "8" };

void Draw(int& y, cvar_t* cvar, const char* format, ...)
{
	if(!cvar->value)
		return;

	static char BUFFER[128];
	va_list args;
	va_start(args, format);
	vsnprintf(BUFFER, ARRAYSIZE(BUFFER), format, args);
	Draw_String((int)tas_hud_pos_x.value, y, BUFFER);
	y += tas_hud_pos_inc.value;
}

bool Should_Print_Cvar(const std::string& name, float value)
{
	auto cvar = Cvar_FindVar(const_cast<char*>(name.c_str()));
	float default_value;
	sscanf(cvar->defaultvalue, "%f", &default_value);

	return cvar != NULL && default_value != value;
}

void DrawFrameState(int& y, const PlaybackInfo& info)
{
	if(!tas_hud_state.value || info.Get_Last_Frame() == 0 || !tas_playing.value)
		return;

	auto current_block = info.Get_Current_Block();

	if(current_block->frame != info.current_frame)
		return;

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Cvars:");
	for (auto& cvar : info.stacked.convars)
	{
		if (current_block && current_block->HasConvar(cvar.first)
			&& current_block->convars.at(cvar.first) != cvar.second)
		{
			Draw(y, &tas_hud_state, "%s %.3f -> %.3f", cvar.first.c_str(), cvar.second, current_block->convars.at(cvar.first));
		}
		else
		{
			if(Should_Print_Cvar(cvar.first, cvar.second))
				Draw(y, &tas_hud_state, "%s %.3f", cvar.first.c_str(), cvar.second);
		}
	}

	if (current_block)
	{
		for (auto& cvar : current_block->convars)
		{
			if (!info.stacked.HasConvar(cvar.first) && Should_Print_Cvar(cvar.first, cvar.second))
			{
				Draw(y, &tas_hud_state, "%s -> %.3f", cvar.first.c_str(), cvar.second);
			}
		}
	}

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Toggles:");
	for (auto& toggle : info.stacked.toggles)
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

}

void Draw_PFlags(int& y)
{
	if(!tas_hud_pflags.value)
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

void HUD_Draw_Hook()
{
	if(!sv.active)
		return;

	int x = tas_hud_pos_x.value;
	int y = tas_hud_pos_y.value;
	
	auto player_data = GetPlayerData();
	auto info = GetPlaybackInfo();
	int last_frame = info.Get_Last_Frame();
	int current_block_no = info.GetCurrentBlockNumber();
	int blocks = info.Get_Number_Of_Blocks();

	Draw(y, &tas_hud_pos, "pos: (%.3f, %.3f, %.3f)", player_data.origin[0], player_data.origin[1], player_data.origin[2]);
	Draw(y, &tas_hud_angles, "ang: (%.3f, %.3f, %.3f)", cl.viewangles[0], cl.viewangles[1], cl.viewangles[2]);
	Draw(y, &tas_hud_vel, "vel: (%.3f, %.3f, %.3f)", player_data.velocity[0], player_data.velocity[1], player_data.velocity[2]);
	Draw(y, &tas_hud_vel2d, "vel2d: %.3f", player_data.vel2d);
	Draw(y, &tas_hud_vel3d, "vel3d: %.3f", VectorLength(player_data.velocity));
	Draw(y, &tas_hud_frame, "frame: %d / %d", info.current_frame, last_frame);
	Draw(y, &tas_hud_block, "block: %d / %d", current_block_no, blocks - 1);
	Draw(y, &tas_hud_waterlevel, "waterlevel: %d", (int)sv_player->v.waterlevel);
	Draw_PFlags(y);
	DrawFrameState(y, info);
}
