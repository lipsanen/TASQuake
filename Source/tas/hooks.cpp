#include "cpp_quakedef.hpp"
#include "hooks.h"
#include "afterframes.hpp"
#include "strafing.hpp"
#include "test.hpp"
#include "reset.hpp"

cvar_t	tas_pause_onload = { "tas_pause_onload", "0" };
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
	Cmd_AddCommand("tas_reset", Cmd_TAS_Reset_f);
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
	Cvar_Register(&tas_pause_onload);
	Cvar_Register(&tas_strafe);
	Cvar_Register(&tas_strafe_lgagst);
	Cvar_Register(&tas_strafe_yaw);
	Cvar_Register(&tas_strafe_yaw_offset);
}

void TAS_Set_Seed(unsigned int seed)
{
	seed_number = seed;
	set_seed = true;
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

	if (tas_gamestate == unpaused)
	{
		Test_Hook();

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