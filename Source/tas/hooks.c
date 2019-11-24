#include "hooks.h"
#include "afterframes.h"
#include "..\quakedef.h"

cvar_t	tas_pause_onload = { "tas_pause_onload", "0" };

void Cmd_TAS_AfterFrames(void)
{
	if (Cmd_Argc() != 3)
	{
		Con_Printf("Usage: tas_afterframes <frames> <command>");
	}

	int frames = atoi(Cmd_Argv(1));
	char* cmd = Cmd_Argv(2);

	if (developer.value)
		Con_Printf("Adding command %s with delay %d\n", cmd, frames);
	AddAfterframes(frames, cmd);
}

void Cmd_TAS_AfterFrames_Await_Load(void)
{
	PauseAfterframes();
}

void Cmd_TAS_AfterFrames_Clear(void)
{
	ClearAfterframes();
}

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