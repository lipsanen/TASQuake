#include <vector>

#include "cpp_quakedef.hpp"

#include "afterframes.hpp"

const int BUFFER_SIZE = 8192;

static std::vector<AfterFrames> afterframesQueue;
static char CmdBuffer[BUFFER_SIZE];
static int bufferIndex = 0;
static bool afterFramesPaused = false;
const unsigned int NoFilter = 0, Game = 1, Menu = 2, Unpaused = 4, Loading = 8;

static unsigned int GetCurrentFilter()
{
	unsigned int filter = 0;

	if (key_dest == key_game && cl.movemessages >= 2)
		filter |= Game;

	if (key_dest == key_menu)
		filter |= Menu;

	if (tas_gamestate == unpaused)
		filter |= Unpaused;

	if (tas_gamestate == loading)
		filter |= Loading;

	return filter;
}

bool FilterMatchesCurrentFrame(unsigned int filter)
{
	auto current = GetCurrentFilter();

	return filter == (filter & current);
}

void CopyToBuffer(const char* str)
{
	Con_DPrintf("Executing %s\n", str);
	int len = strlen(str);
	for (int i = 0; i < len; ++i)
	{
		CmdBuffer[bufferIndex++] = str[i];
	}
	CmdBuffer[bufferIndex++] = ';';
}

void AddAfterframes(int frames, const char* cmd, unsigned int filter)
{
	afterframesQueue.push_back(AfterFrames(frames, cmd, filter));
}

void AdvanceCommands()
{
	for (auto& entry : afterframesQueue)
	{
		if(entry.Active())
			--entry.frames;
	}
}

char* GetQueuedCommands()
{
	if (!afterFramesPaused)
	{
		AdvanceCommands();

		for (int i = afterframesQueue.size() - 1; i >= 0; --i)
		{
			auto& entry = afterframesQueue[i];
			if (entry.frames <= 0 && entry.Active())
			{
				CopyToBuffer(entry.command.c_str());
				afterframesQueue.erase(afterframesQueue.begin() + i);
			}
		}
	}

	if (bufferIndex == 0)
		return NULL;
	else
	{
		CmdBuffer[bufferIndex++] = '\0';
		bufferIndex = 0;
		return CmdBuffer;
	}
}

void PauseAfterframes()
{
	afterFramesPaused = true;
}

void UnpauseAfterframes()
{
	afterFramesPaused = false;
}

void ClearAfterframes()
{
	afterframesQueue.clear();
}

void Cmd_TAS_AfterFrames(void)
{
	if (Cmd_Argc() != 3)
	{
		Con_Printf("Usage: tas_afterframes <frames> <command>");
	}

	int frames = atoi(Cmd_Argv(1));
	char* cmd = Cmd_Argv(2);
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

AfterFrames::AfterFrames(int numFrames, const char* cmd, unsigned int f)
{
	frames = numFrames;
	command = cmd;
	filter = f;
}

AfterFrames::AfterFrames(int numFrames, const std::string& str, unsigned int f)
{
	frames = numFrames;
	command = str;
	filter = f;
}

AfterFrames::AfterFrames() {}

bool AfterFrames::Active()
{
	return FilterMatchesCurrentFrame(filter);
}
