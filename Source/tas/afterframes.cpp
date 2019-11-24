#include "afterframes.h"
#include <vector>
#include <string>

const int BUFFER_SIZE = 1024;

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