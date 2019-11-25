#include "cpp_quakedef.hpp"
#include "afterframes.hpp"
#include <vector>

const int BUFFER_SIZE = 8192;

struct AfterFrames
{
	AfterFrames(int numFrames, char* cmd)
	{
		command = cmd;
		this->frames = numFrames;
	}

	std::string command;
	int frames;
};

static std::vector<AfterFrames> afterframesQueue;
static char CmdBuffer[BUFFER_SIZE];
static int bufferIndex;
static bool afterFramesPaused = false;


void AddAfterframes(int frames, char* cmd)
{
	afterframesQueue.push_back(AfterFrames(frames, cmd));
}

void AdvanceCommands()
{
	for (auto& entry : afterframesQueue)
	{
		--entry.frames;
	}
}

void CopyToBuffer(const char* str)
{
	int len = strlen(str);
	for (int i = 0; i < len; ++i)
	{
		CmdBuffer[bufferIndex++] = str[i];
	}
	CmdBuffer[bufferIndex++] = ';';
}

char* GetQueuedCommands()
{
	bufferIndex = 0;

	if (!afterFramesPaused)
	{
		AdvanceCommands();

		for (int i = afterframesQueue.size() - 1; i >= 0; --i)
		{
			auto& entry = afterframesQueue[i];
			if (entry.frames <= 0)
			{
				CopyToBuffer(entry.command.c_str());
				afterframesQueue.erase(afterframesQueue.begin() + i);
			}
		}
	}

	if(bufferIndex == 0)
		return NULL;
	else
	{
		CmdBuffer[bufferIndex++] = '\0';
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