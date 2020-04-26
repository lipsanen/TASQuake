#pragma once
#include <string>

extern const unsigned int NoFilter, Game, Menu, Unpaused, Loading;

struct AfterFrames
{
	AfterFrames(int numFrames, const char* cmd, unsigned int f = 0);
	AfterFrames(int numFrames, const std::string& str, unsigned int f = Unpaused);
	AfterFrames();
	bool Active();

	std::string command;
	unsigned int filter;
	int frames;
};

void PauseAfterframes();
void UnpauseAfterframes();
void ClearAfterframes();
void AddAfterframes(int frames, const char* cmd, unsigned int f = Unpaused);
char* GetQueuedCommands();
bool FilterMatchesCurrentFrame(unsigned int filter);

void Cmd_TAS_AfterFrames();
void Cmd_TAS_AfterFrames_Await_Load(void);
void Cmd_TAS_AfterFrames_Clear(void);
