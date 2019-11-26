#include "cpp_quakedef.hpp"
#include "hooks.h"
#include "afterframes.hpp"
#include "strafing.hpp"

cvar_t	tas_pause_onload = { "tas_pause_onload", "0" };
static bool set_seed = false;

void Cmd_TAS_Set_Seed_Onload(void)
{
	set_seed = true;
}

void SV_Physics_Client_Hook()
{
	Strafe_Jump_Check();
}

void CL_SendMove_Hook(usercmd_t* cmd)
{
	if (tas_strafe.value )
	{
		Strafe(cmd);
	}
}

void TAS_Init()
{
	Cmd_AddCommand("tas_set_seed_onload", Cmd_TAS_Set_Seed_Onload);
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

void Host_Connect_f_Hook()
{
	if (tas_pause_onload.value)
	{
		AddAfterframes(0, "tas_afterframes 0 pause");
	}

	if (set_seed)
	{
		srand(0);
		set_seed = false;
	}

	UnpauseAfterframes();
}

void _Host_Frame_Hook()
{
	char* queued = GetQueuedCommands();
	if (queued)
	{
		Cbuf_AddText(queued);
	}
}