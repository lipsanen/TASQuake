#include "cpp_quakedef.hpp"

#include "hooks.h"

#include "afterframes.hpp"
#include "camera.hpp"
#include "draw.hpp"
#include "hud.hpp"
#include "reset.hpp"
#include "script_playback.hpp"
#include "simulate.hpp"
#include "strafing.hpp"
#include "state_test.hpp"
#include "savestate.hpp"
#include "data_export.hpp"
#include "test_runner.hpp"
#include "ipc_main.hpp"
#include "rewards.hpp"
#include "bookmark.hpp"
#include "utils.hpp"

// desc: When set to 1, pauses the game on load
cvar_t tas_pause_onload = {"tas_pause_onload", "0"};
cvar_t tas_playing = {"tas_playing", "0"};
// desc: Controls the timescale of the game
cvar_t tas_timescale = {"tas_timescale", "1"};
static bool set_seed = false;
static int unpause_countdown = -1;
static unsigned int seed_number = 0;

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
	if (cls.demoplayback)
		return;

	Strafe_Jump_Check();
}

void CL_SendMove_Hook(usercmd_t* cmd)
{
	if (cls.demoplayback)
		return;

	Strafe(cmd);
}

qboolean V_CalcRefDef_Hook(void)
{
	return Camera_Refdef_Hook();
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
	Con_Printf("Seed is %u\n", Get_RNG_Seed());
	Con_Printf("Increment count this frame is %d\n", Get_RNG_Count());
	Con_Printf("Index is %d\n", Get_RNG_Index());
}

void Cmd_Print_Time(void)
{
	Con_Printf("Time is %f\n", sv.time);
	Con_Printf("Client time is %f\n", cl.time);
}

void TAS_Set_Seed(int seed)
{
	set_seed = true;
	seed_number = seed;
}

void TAS_Init()
{
	Savestate_Init();

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
	Cmd_AddCommand("tas_edit_shots", Cmd_TAS_Edit_Shots);
	Cmd_AddCommand("tas_edit_random_toggle", Cmd_TAS_Edit_Random_Toggle);

	Cmd_AddCommand("tas_cancel", Cmd_TAS_Cancel);
	Cmd_AddCommand("tas_confirm", Cmd_TAS_Confirm);
	Cmd_AddCommand("tas_revert", Cmd_TAS_Revert);

	Cmd_AddCommand("tas_bookmark_frame", Cmd_TAS_Bookmark_Frame);
	Cmd_AddCommand("tas_bookmark_block", Cmd_TAS_Bookmark_Block);
	Cmd_AddCommand("tas_bookmark_skip", Cmd_TAS_Bookmark_Skip);

	Cmd_AddCommand("tas_cmd_reset", Cmd_TAS_Cmd_Reset);
	Cmd_AddCommand("tas_reset_movement", Cmd_TAS_Reset_Movement);
	Cmd_AddCommand("tas_ipc_simulate", Cmd_TAS_IPC_Simulate);
	Cmd_AddCommand("tas_ipc_condition", Cmd_TAS_IPC_Condition);
	Cmd_AddCommand("tas_ipc_condition_disable", Cmd_TAS_IPC_Condition_Disable);
	Cmd_AddCommand("tas_ipc_print_posvel", Cmd_TAS_IPC_Print_Posvel);
	Cmd_AddCommand("tas_ipc_print_seed", Cmd_TAS_IPC_Print_Seed);
	Cmd_AddCommand("tas_ipc_print_playback", Cmd_TAS_IPC_Print_Playback);
	
	Cmd_AddCommand("tas_print_seed", Cmd_Print_Seed);
	Cmd_AddCommand("tas_print_time", Cmd_Print_Time);
	Cmd_AddCommand("tas_pause", Cmd_TAS_Pause);
	Cmd_AddCommand("tas_print_vel", Cmd_TAS_Print_Vel);
	Cmd_AddCommand("tas_print_origin", Cmd_TAS_Print_Origin);
	Cmd_AddCommand("tas_set_seed", Cmd_TAS_Set_Seed_Onload);
	Cmd_AddCommand("tas_afterframes", Cmd_TAS_AfterFrames);
	Cmd_AddCommand("tas_afterframes_await_load", Cmd_TAS_AfterFrames_Await_Load);
	Cmd_AddCommand("tas_afterframes_clear", Cmd_TAS_AfterFrames_Clear);
	Cmd_AddCommand("tas_dump_sv", Cmd_TAS_Dump_SV);
	Cmd_AddCommand("+tas_jump", IN_TAS_Jump_Down);
	Cmd_AddCommand("-tas_jump", IN_TAS_Jump_Up);
	Cmd_AddCommand("+tas_lgagst", IN_TAS_Lgagst_Down);
	Cmd_AddCommand("-tas_lgagst", IN_TAS_Lgagst_Up);

	Cmd_AddCommand("tas_reward_delete_all", Cmd_TAS_Reward_Delete_All);
	Cmd_AddCommand("tas_reward_dump", Cmd_TAS_Reward_Dump);
	Cmd_AddCommand("tas_reward_gate", Cmd_TAS_Reward_Gate);
	Cmd_AddCommand("tas_reward_intermission", Cmd_TAS_Reward_Intermission);
	Cmd_AddCommand("tas_reward_load", Cmd_TAS_Reward_Load);
	Cmd_AddCommand("tas_reward_pop", Cmd_TAS_Reward_Pop);
	Cmd_AddCommand("tas_reward_save", Cmd_TAS_Reward_Save);

	Cmd_AddCommand("tas_test_script", Cmd_TAS_Test_Script);
	Cmd_AddCommand("tas_test_run", Cmd_TAS_Test_Run);
	Cmd_AddCommand("tas_test_generate", Cmd_TAS_Test_Generate);

	Cmd_AddCommand("tas_ls", Cmd_TAS_LS);
	Cmd_AddCommand("tas_ss_clear", Cmd_TAS_SS_Clear);
	Cmd_AddCommand("tas_savestate", Cmd_TAS_Savestate);
	Cmd_AddCommand("tas_trace_edict", Cmd_TAS_Trace_Edict);
	Cvar_Register(&tas_playing);
	Cvar_Register(&tas_pause_onload);
	Cvar_Register(&tas_strafe);
	Cvar_Register(&tas_strafe_type);
	Cvar_Register(&tas_strafe_yaw);
	Cvar_Register(&tas_strafe_pitch);
	Cvar_Register(&tas_strafe_lgagst_speed);
	Cvar_Register(&tas_strafe_version);
	Cvar_Register(&tas_strafe_maxlength);
	Cvar_Register(&tas_view_pitch);
	Cvar_Register(&tas_view_yaw);
	Cvar_Register(&tas_anglespeed);
	Cvar_Register(&tas_edit_backups);
	Cvar_Register(&tas_edit_snap_threshold);
	Cvar_Register(&tas_freecam);
	Cvar_Register(&tas_freecam_speed);
	Cvar_Register(&tas_hud_frame);
	Cvar_Register(&tas_hud_block);
	Cvar_Register(&tas_hud_particles);
	Cvar_Register(&tas_hud_pos);
	Cvar_Register(&tas_hud_pos_inc);
	Cvar_Register(&tas_hud_pos_x);
	Cvar_Register(&tas_hud_pos_y);
	Cvar_Register(&tas_hud_rng);
	Cvar_Register(&tas_hud_vel);
	Cvar_Register(&tas_hud_vel2d);
	Cvar_Register(&tas_hud_vel3d);
	Cvar_Register(&tas_hud_velang);
	Cvar_Register(&tas_hud_angles);
	Cvar_Register(&tas_hud_pflags);
	Cvar_Register(&tas_hud_waterlevel);
	Cvar_Register(&tas_hud_state);
	Cvar_Register(&tas_hud_strafe);
	Cvar_Register(&tas_hud_strafeinfo);
	Cvar_Register(&tas_hud_movemessages);
	Cvar_Register(&tas_ipc);
	Cvar_Register(&tas_ipc_feedback);
	Cvar_Register(&tas_ipc_port);
	Cvar_Register(&tas_ipc_timeout);
	Cvar_Register(&tas_ipc_verbose);
	Cvar_Register(&tas_timescale);
	Cvar_Register(&tas_predict);
	Cvar_Register(&tas_predict_grenade);
	Cvar_Register(&tas_predict_per_frame);
	Cvar_Register(&tas_predict_amount);
	Cvar_Register(&tas_reward_display);
	Cvar_Register(&tas_reward_size);
	Cvar_Register(&tas_savestate_auto);
	Cvar_Register(&tas_savestate_enabled);

	IPC_Init();
}

void TAS_Set_Seed(unsigned int seed)
{
	seed_number = seed;
	set_seed = true;
}

qboolean Cmd_ExecuteString_Hook(const char* text)
{
	if (cls.demoplayback)
		return qfalse;

	return Script_Playback_Cmd_ExecuteString_Hook(text);
}

void IN_Move_Hook(usercmd_t* cmd)
{
	if (cls.demoplayback)
		return;

	Script_Playback_IN_Move_Hook(cmd);
}

void SCR_Draw_TAS_HUD_Hook(void)
{
	HUD_Draw_Hook();
}

void Draw_Lines_Hook(void)
{
	if (cls.demoplayback)
		return;

	Draw_Elements();
}

void Host_Connect_f_Hook()
{
	if (cls.demoplayback)
		return;

	if (tas_pause_onload.value)
	{
		AddAfterframes(0, "tas_afterframes 0 pause");
	}

	UnpauseAfterframes();
}

void CL_SignonReply_Hook()
{
	if (cls.demoplayback)
		return;

	if (cls.signon == 4 && tas_gamestate == loading)
		unpause_countdown = 4;
}

void IN_MouseMove_Hook(int mousex, int mousey)
{
	Camera_MouseMove_Hook(mousex, mousey);
}

void _Host_Frame_Hook()
{
	if (cls.demoplayback)
		return;

	if (unpause_countdown > 0)
	{
		--unpause_countdown;
		if (unpause_countdown == 0)
		{
			tas_gamestate = unpaused;
			unpause_countdown = -1;
			key_dest = key_game;
			Restore_Client();
		}
	}

	if (set_seed && tas_gamestate == unpaused)
	{
		set_seed = false;
		srand(seed_number);
	}
}

void _Host_Frame_After_FilterTime_Hook()
{
	Test_Host_Frame_Hook();
	Test_Runner_Frame_Hook();
	IPC_Loop();
	Bookmark_Frame_Hook();
	Simulate_Frame_Hook();
	Script_Playback_Host_Frame_Hook();

	char* queued = GetQueuedCommands();
	if (queued)
	{
		Cbuf_AddText(queued);
	}
}
