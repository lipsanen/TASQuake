#pragma once

void PauseAfterframes();
void UnpauseAfterframes();
void ClearAfterframes();
void AddAfterframes(int frames, char* cmd);
char* GetQueuedCommands();

extern "C"
{
	void Cmd_TAS_AfterFrames();
	void Cmd_TAS_AfterFrames_Await_Load(void);
	void Cmd_TAS_AfterFrames_Clear(void);
}
