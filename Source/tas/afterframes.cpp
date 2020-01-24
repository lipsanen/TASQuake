#include "afterframes.hpp"

const int BUFFER_SIZE = 8192;

static std::vector<AfterFrames> afterframesQueue;
static char CmdBuffer[BUFFER_SIZE];
static int bufferIndex = 0;
static bool afterFramesPaused = false;

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

void AddAfterframes(int frames, const char* cmd)
{
	if (frames <= 1)
		CopyToBuffer(cmd);
	else
		afterframesQueue.push_back(AfterFrames(frames, cmd));
}

void AdvanceCommands()
{
	for (auto& entry : afterframesQueue)
	{
		--entry.frames;
	}
}

char* GetQueuedCommands()
{
	if (!afterFramesPaused && tas_gamestate == unpaused)
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

AfterFrames::AfterFrames(int numFrames, const char* cmd)
{
	frames = numFrames;
	command = cmd;
}

AfterFrames::AfterFrames(int numFrames, const std::string& str)
{
	frames = numFrames;
	command = str;
}

AfterFrames::AfterFrames() {}
