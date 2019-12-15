#pragma once
#include <string>


struct AfterFrames
{
	AfterFrames(int numFrames, const char* cmd);
	AfterFrames(int numFrames, const std::string& str);
	AfterFrames();

	std::string command;
	int frames;
};


void PauseAfterframes();
void UnpauseAfterframes();
void ClearAfterframes();
void AddAfterframes(int frames, const char* cmd);
void AddAfterframes(const AfterFrames& af);
char* GetQueuedCommands();

extern "C"
{
	void Cmd_TAS_AfterFrames();
	void Cmd_TAS_AfterFrames_Await_Load(void);
	void Cmd_TAS_AfterFrames_Clear(void);
}
