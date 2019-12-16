#include "hud.hpp"
#include "strafing.hpp"
#include "script_playback.hpp"

cvar_t tas_hud_pos = {"tas_hud_pos", "0"};
cvar_t tas_hud_angles = {"tas_hud_angles", "0"};
cvar_t tas_hud_vel = {"tas_hud_vel", "0"};
cvar_t tas_hud_vel2d = {"tas_hud_vel2d", "0"};
cvar_t tas_hud_frame = {"tas_hud_frame", "0"};
cvar_t tas_hud_state = {"tas_hud_state", "0"};

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

void DrawState(int& y, const PlaybackInfo& info)
{
	if(!tas_hud_state.value || !info.stacked)
		return;

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Cvars:");
	for (auto& cvar : info.stacked->convars)
	{
		if (info.current_block && info.current_block->HasConvar(cvar.first)
			&& info.current_block->convars[cvar.first] != cvar.second)
		{
			Draw(y, &tas_hud_state, "%s %.3f -> %.3f", cvar.first.c_str(), cvar.second, info.current_block->convars[cvar.first]);
		}
		else
		{
			Draw(y, &tas_hud_state, "%s %.3f", cvar.first.c_str(), cvar.second);
		}
	}

	if (info.current_block)
	{
		for (auto& cvar : info.current_block->convars)
		{
			if (!info.stacked->HasConvar(cvar.first))
			{
				Draw(y, &tas_hud_state, "%s -> %.3f", cvar.first.c_str(), cvar.second);
			}
		}
	}

	Draw(y, &tas_hud_state, "");
	Draw(y, &tas_hud_state, "Toggles:");
	for (auto& toggle : info.stacked->toggles)
	{
		if (toggle.second)
		{
			if (info.current_block && info.current_block->HasToggle(toggle.first))
				Draw(y, &tas_hud_state, "+%s -> -%s", toggle.first.c_str(), toggle.first.c_str());
			else
				Draw(y, &tas_hud_state, "+%s", toggle.first.c_str());

		}
	}

	if (info.current_block)
	{
		for (auto& toggle : info.current_block->toggles)
		{
			if (!info.stacked->HasToggle(toggle.first))
			{
				Draw(y, &tas_hud_state, "-%s -> +%s", toggle.first.c_str(), toggle.first.c_str());
			}
		}
	}

}

void HUD_Draw_Hook()
{
	if(!sv.active)
		return;

	int x = tas_hud_pos_x.value;
	int y = tas_hud_pos_y.value;
	
	auto player_data = GetPlayerData();
	auto info = GetPlaybackInfo();

	Draw(y, &tas_hud_pos, "pos: (%.3f, %.3f, %.3f)", player_data.origin[0], player_data.origin[1], player_data.origin[2]);
	Draw(y, &tas_hud_angles, "ang: (%.3f, %.3f, %.3f)", cl.viewangles[0], cl.viewangles[1], cl.viewangles[2]);
	Draw(y, &tas_hud_vel, "vel: (%.3f, %.3f, %.3f)", player_data.velocity[0], player_data.velocity[1], player_data.velocity[2]);
	Draw(y, &tas_hud_vel2d, "vel2d: %.3f", player_data.vel2d);
	Draw(y, &tas_hud_frame, "frame: %d / %d", info.current_frame, info.last_frame);
	DrawState(y, info);
}
