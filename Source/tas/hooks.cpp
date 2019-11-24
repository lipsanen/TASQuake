#include "cpp_quakedef.hpp"
#include "hooks.h"
#include "afterframes.hpp"

cvar_t	tas_pause_onload = { "tas_pause_onload", "0" };

void TAS_Init()
{
	Cmd_AddCommand("tas_afterframes", Cmd_TAS_AfterFrames);
	Cmd_AddCommand("tas_afterframes_await_load", Cmd_TAS_AfterFrames_Await_Load);
	Cmd_AddCommand("tas_afterframes_clear", Cmd_TAS_AfterFrames_Clear);
	Cvar_Register(&tas_pause_onload);
}


void Host_Connect_f_Hook()
{
	if (tas_pause_onload.value)
	{
		AddAfterframes(0, "tas_afterframes 0 pause");
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