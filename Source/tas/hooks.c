#include "hooks.h"
#include "afterframes.h"
#include "..\quakedef.h"


void Cmd_TAS_AfterFrames(void)
{
	if (Cmd_Argc() != 3)
	{
		Con_Printf("Usage: tas_afterframes <frames> <command>");
	}

	int frames = atoi(Cmd_Argv(1));
	char* cmd = Cmd_Argv(2);

	Con_Printf("adding command %s with delay %d\n", cmd, frames);
	AddAfterframes(frames, cmd);
}


void TAS_Init()
{
	Cmd_AddCommand("tas_afterframes", Cmd_TAS_AfterFrames);
}

void _Host_Frame_Hook()
{
	char* queued = GetQueuedCommands();
	if (queued)
	{
		Cbuf_AddText(queued);
	}
}