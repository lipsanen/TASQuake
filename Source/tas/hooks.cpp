#include "cpp_quakedef.hpp"
#include "hooks.h"
#include "afterframes.hpp"
#include "strafing.hpp"
#include "test.hpp"
#include "reset.hpp"
#include "script_playback.hpp"
#include "hud.hpp"
#include "draw.hpp"
#include "simulate.hpp"

cvar_t	tas_pause_onload = { "tas_pause_onload", "0" };
cvar_t tas_playing = { "tas_playing", "0" };
cvar_t tas_timescale = { "tas_timescale", "1"};
static bool set_seed = false;
static int unpause_countdown = -1;
static unsigned int seed_number = 0;
static bool should_prevent_unpause = false;

void Cmd_TAS_Set_Seed_Onload(void)
{
	if (Cmd_Argc() == 1)
	{
		seed_number = 0;
	}
	else
	{
		int seed = atoi(Cmd_Argv(1));
		seed_number = seed;
	}

	set_seed = true;

}

void SV_Physics_Client_Hook()
{
	Strafe_Jump_Check();
}

void CL_SendMove_Hook(usercmd_t* cmd)
{
	Strafe(cmd);
}

void Cmd_TAS_Pause(void)
{
	if (tas_gamestate == paused)
	{
		Con_Printf("TAS: Unpausing.\n");
		tas_gamestate = unpaused;
	}
	else
	{
		Con_Printf("TAS: Pausing.\n");
		tas_gamestate = paused;
	}
}

void Cmd_Print_Seed(void)
{
	Con_Printf("Seed is %d\n", Get_RNG_Seed());
}

void TAS_Set_Seed(int seed)
{
	set_seed = true;
	seed_number = seed;	
}


void TAS_Init()
{
	Cmd_AddCommand("tas_script_init", Cmd_TAS_Script_Init);
	Cmd_AddCommand("tas_script_stop", Cmd_TAS_Script_Stop);
	Cmd_AddCommand("tas_script_play", Cmd_TAS_Script_Play);
	Cmd_AddCommand("tas_script_load", Cmd_TAS_Script_Load);
	Cmd_AddCommand("tas_script_skip", Cmd_TAS_Script_Skip);
	Cmd_AddCommand("tas_script_skip_block", Cmd_TAS_Script_Skip_Block);
	Cmd_AddCommand("tas_script_advance", Cmd_TAS_Script_Advance);
	Cmd_AddCommand("tas_script_advance_block", Cmd_TAS_Script_Advance_Block);

	Cmd_AddCommand("tas_edit_prune", Cmd_TAS_Edit_Prune);
	Cmd_AddCommand("tas_edit_save", Cmd_TAS_Edit_Save);
	Cmd_AddCommand("tas_edit_set_pitch", Cmd_TAS_Edit_Set_Pitch);
	Cmd_AddCommand("tas_edit_set_yaw", Cmd_TAS_Edit_Set_Yaw);
	Cmd_AddCommand("tas_edit_set_view", Cmd_TAS_Edit_Set_View);
	Cmd_AddCommand("tas_edit_strafe", Cmd_TAS_Edit_Strafe);
	Cmd_AddCommand("tas_edit_swim", Cmd_TAS_Edit_Swim);
	Cmd_AddCommand("tas_edit_shrink", Cmd_TAS_Edit_Shrink);
	Cmd_AddCommand("tas_edit_delete", Cmd_TAS_Edit_Delete);
	Cmd_AddCommand("tas_edit_shift", Cmd_TAS_Edit_Shift);
	Cmd_AddCommand("tas_edit_shift_stack", Cmd_TAS_Edit_Shift_Stack);
	Cmd_AddCommand("tas_edit_add_empty", Cmd_TAS_Edit_Add_Empty);

	Cmd_AddCommand("tas_cancel", Cmd_TAS_Cancel); // Keep as it is
	Cmd_AddCommand("tas_confirm", Cmd_TAS_Confirm); // Confirm change

	Cmd_AddCommand("tas_bookmark_frame", Cmd_TAS_Bookmark_Frame);
	Cmd_AddCommand("tas_bookmark_block", Cmd_TAS_Bookmark_Block);
	Cmd_AddCommand("tas_bookmark_skip", Cmd_TAS_Bookmark_Skip);

	Cmd_AddCommand("tas_cmd_reset", Cmd_TAS_Cmd_Reset_f);
	Cmd_AddCommand("tas_reset_movement", Cmd_TAS_Reset_Movement);
	Cmd_AddCommand("tas_run_test", Cmd_Run_Test);
	Cmd_AddCommand("tas_generate_test", Cmd_GenerateTest);
	Cmd_AddCommand("tas_print_seed", Cmd_Print_Seed);
	Cmd_AddCommand("tas_pause", Cmd_TAS_Pause);
	Cmd_AddCommand("tas_print_moves", Cmd_Print_Moves);
	Cmd_AddCommand("tas_print_vel", Cmd_Print_Vel);
	Cmd_AddCommand("tas_print_origin", Cmd_Print_Origin);
	Cmd_AddCommand("tas_set_seed", Cmd_TAS_Set_Seed_Onload);
	Cmd_AddCommand("tas_afterframes", Cmd_TAS_AfterFrames);
	Cmd_AddCommand("tas_afterframes_await_load", Cmd_TAS_AfterFrames_Await_Load);
	Cmd_AddCommand("tas_afterframes_clear", Cmd_TAS_AfterFrames_Clear);
	Cmd_AddCommand("+tas_jump", IN_TAS_Jump_Down);
	Cmd_AddCommand("-tas_jump", IN_TAS_Jump_Up);
	Cmd_AddCommand("+tas_lgagst", IN_TAS_Lgagst_Down);
	Cmd_AddCommand("-tas_lgagst", IN_TAS_Lgagst_Up);
	Cvar_Register(&tas_playing);
	Cvar_Register(&tas_pause_onload);
	Cvar_Register(&tas_strafe);
	Cvar_Register(&tas_strafe_type);
	Cvar_Register(&tas_strafe_yaw);
	Cvar_Register(&tas_strafe_pitch);
	Cvar_Register(&tas_strafe_lgagst_speed);
	Cvar_Register(&tas_strafe_version);
	Cvar_Register(&tas_view_pitch);
	Cvar_Register(&tas_view_yaw);
	Cvar_Register(&tas_anglespeed);
	Cvar_Register(&tas_edit_backups);
	Cvar_Register(&tas_edit_snap_threshold);
	Cvar_Register(&tas_hud_frame);
	Cvar_Register(&tas_hud_block);
	Cvar_Register(&tas_hud_pos);
	Cvar_Register(&tas_hud_pos_inc);
	Cvar_Register(&tas_hud_pos_x);
	Cvar_Register(&tas_hud_pos_y);
	Cvar_Register(&tas_hud_vel);
	Cvar_Register(&tas_hud_vel2d);
	Cvar_Register(&tas_hud_vel3d);
	Cvar_Register(&tas_hud_velang);
	Cvar_Register(&tas_hud_angles);
	Cvar_Register(&tas_hud_pflags);
	Cvar_Register(&tas_hud_waterlevel);
	Cvar_Register(&tas_hud_state);
	Cvar_Register(&tas_timescale);
	Cvar_Register(&tas_predict);
	Cvar_Register(&tas_predict_per_frame);
	Cvar_Register(&tas_predict_min);
	Cvar_Register(&tas_predict_max);
}

void TAS_Set_Seed(unsigned int seed)
{
	seed_number = seed;
	set_seed = true;
}

qboolean Cmd_ExecuteString_Hook(const char * text)
{
	return Script_Playback_Cmd_ExecuteString_Hook(text);
}

void IN_Move_Hook(usercmd_t * cmd)
{
	Script_Playback_IN_Move_Hook(cmd);
}

void SCR_Draw_TAS_HUD_Hook(void)
{
	HUD_Draw_Hook();
}

void Draw_Lines_Hook(void)
{
	Draw_Elements();
}

void Host_Connect_f_Hook()
{
	if (tas_pause_onload.value)
	{
		AddAfterframes(0, "tas_afterframes 0 pause");
	}

	UnpauseAfterframes();
}

void CL_SignonReply_Hook()
{
	if(cls.signon == 4)
		unpause_countdown = 3;
}

void _Host_Frame_Hook()
{
	if (unpause_countdown > 0)
	{
		--unpause_countdown;
		if (unpause_countdown == 0)
		{
			tas_gamestate = unpaused;
			unpause_countdown = -1;
		}		
	}

	Simulate_Frame_Hook();
	Script_Playback_Host_Frame_Hook();

	if (tas_gamestate == unpaused)
	{
		Test_Host_Frame_Hook();

		if (set_seed && tas_gamestate == unpaused)
		{
			set_seed = false;
			srand(seed_number);
		}

		char* queued = GetQueuedCommands();
		if (queued)
		{
			Cbuf_AddText(queued);
		}
	}

}