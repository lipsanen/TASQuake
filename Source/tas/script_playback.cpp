#include "script_playback.hpp"
#include "script_parse.hpp"
#include "reset.hpp"
#include "afterframes.hpp"

static TASScript current_script;
static int current_frame = 0;
static int last_frame = 0;
static int current_block = 0;
static bool script_running = false;
static bool should_unpause = false;
const int LOWEST_FRAME = 2;
const int LOWEST_BLOCK = 3;


void Run_Script(int frame, bool skip = false)
{
	if (current_script.blocks.size() == 0)
	{
		Con_Print("No script loaded\n");
		return;
	}

	current_frame = 0;
	current_block = 0;

	if (frame >= 0)
	{
		last_frame = frame;
	}
	else
	{
		int frames_from_end = -1 - frame;
		auto& last_block = current_script.blocks[current_script.blocks.size() - 1];
		last_frame = last_block.frame - frames_from_end;
	}

	if (last_frame < LOWEST_FRAME)
	{
		Con_Printf("Cannot go to frame below 2.\n");
		return;
	}

	Cmd_TAS_Full_Reset_f();
	AddAfterframes(0, "tas_playing 1");

	if (skip)
	{
		AddAfterframes(0, "cl_maxfps 0");
		AddAfterframes(frame, "cl_maxfps 72");
	}

	script_running = true;
}

void Continue_Script(int frames)
{
	last_frame = current_frame + frames;
	script_running = true;
	should_unpause = true;
}

void Script_Playback_Host_Frame_Hook()
{
	if (tas_gamestate == paused && should_unpause)
	{
		tas_gamestate = unpaused;
		should_unpause = false;
	}

	if(tas_gamestate == paused || !script_running)
		return;
	else if (current_frame == last_frame)
	{
		tas_gamestate = paused;
		script_running = false;
		return;
	}
		

	while (current_block < current_script.blocks.size() && current_script.blocks[current_block].frame <= current_frame)
	{
		auto& block = current_script.blocks[current_block];
		std::string cmd = block.GetCommand();
		AddAfterframes(0, cmd.c_str());
		++current_block;
	}

	++current_frame;
}

void Cmd_TAS_Script_Load(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_script_load <filename>\n");
		return;
	}

	char name[256];
	sprintf(name, "%s/tas/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension(name, ".qtas");

	current_script = TASScript(name);
	current_script.Load_From_File();
	Con_Printf("Script %s loaded with %u blocks.\n", Cmd_Argv(1), current_script.blocks.size());
}

void Cmd_TAS_Script_Play(void)
{
	if (Cmd_Argc() > 1)
		Cmd_TAS_Script_Load();
	Run_Script(-1);
}

void Cmd_TAS_Script_Stop(void)
{
	script_running = false;
	ClearAfterframes();
	Cmd_TAS_Full_Reset_f();
}

void Cmd_TAS_Script_Skip(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_script_skip <frame>\n");
		return;
	}

	int frame = atoi(Cmd_Argv(1));
	Run_Script(frame, true);
}

void Cmd_TAS_Script_Skip_Block(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_script_skip_block <block>\n");
		return;
	}

	int block = atoi(Cmd_Argv(1));
	if (block < 0)
	{
		int blocks_from_end = 0 - block;
		block = current_script.blocks.size() - blocks_from_end;
	}

	if (block >= current_script.blocks.size() || block < LOWEST_BLOCK)
	{
		Con_Printf("Tried to move to block %d, which is out of bounds!\n", block);
		return;
	}

	int frame = current_script.blocks[block].frame;
	Run_Script(frame, true);
}

void Cmd_TAS_Script_Advance(void)
{
	if (script_running)
	{
		Con_Printf("Can't advance while script still running.\n");
		return;
	}

	int frames;

	if (Cmd_Argc() > 1)
		frames = atoi(Cmd_Argv(1));
	else
		frames = 1;

	if (frames > 0)
	{
		Continue_Script(frames);
	}
	else if (frames < 0)
	{
		int target_frame = current_frame + frames;
		if (target_frame < LOWEST_FRAME)
		{
			Con_Printf("Cannot go back beyond frame %d.\n", LOWEST_FRAME);
			return;
		}
		Run_Script(target_frame, true);
	}
	else
	{
		Con_Print("Can't advance by 0 frames.\n");
	}
}

void Cmd_TAS_Script_Advance_Block(void)
{
	if (script_running)
	{
		Con_Printf("Can't advance while script still running.\n");
		return;
	}

	int blocks;

	if (Cmd_Argc() > 1)
		blocks = atoi(Cmd_Argv(1));
	else
		blocks = 1;

	int target_block = current_block + blocks;

	if (target_block < LOWEST_BLOCK || target_block >= current_script.blocks.size())
	{
		Con_Printf("Tried to move to block %d, which is out of bounds!\n", target_block);
		return;
	}

	auto& block = current_script.blocks[target_block];
	int frame = block.frame;

	if (frame > current_frame)
	{
		int diff = frame - current_frame;
		Continue_Script(diff);
	}
	else
	{
		Run_Script(frame, true);
	}
}

