#include <algorithm>

#include "cpp_quakedef.hpp"
#include "script_playback.hpp"
#include "script_parse.hpp"
#include "reset.hpp"
#include "afterframes.hpp"
#include "utils.hpp"
#include "hooks.h"
#include "strafing.hpp"

static PlaybackInfo playback;
const int LOWEST_FRAME = 8;
const int LOWEST_BLOCK = 2;
static char BUFFER[256];

enum class MouseState { Locked, Strafe, Yaw, Pitch, Mixed };
static float old_yaw = 0;
static float old_pitch = 0;
static MouseState m_state = MouseState::Locked;

cvar_t tas_edit_backups = { "tas_edit_backups", "100" };
cvar_t tas_edit_snap_threshold = { "tas_edit_snap_threshold", "5" };

static void Run_Script(int frame, bool skip = false)
{
	if (playback.Get_Number_Of_Blocks() == 0)
	{
		Con_Print("No script loaded\n");
		return;
	}

	playback.current_frame = 0;
	playback.stacked.Reset();

	if (frame >= 0)
	{
		playback.pause_frame = frame;
	}
	else
	{
		int frames_from_end = -1 - frame;
		auto& last_block = playback.current_script.blocks[playback.current_script.blocks.size() - 1];
		playback.pause_frame = last_block.frame - frames_from_end;
	}

	if (playback.pause_frame < LOWEST_FRAME)
	{
		Con_Printf("Cannot go to frame below %d.\n", LOWEST_FRAME);
		return;
	}

	Cmd_TAS_Cmd_Reset_f();
	AddAfterframes(0, "tas_playing 1");

	if (skip)
	{
		AddAfterframes(0, "tas_timescale 999999; r_norefresh 1");
		AddAfterframes(playback.pause_frame-1- playback.current_frame, "tas_timescale 1; r_norefresh 0");
	}

	playback.script_running = true;
}

static void Continue_Script(int frames)
{
	playback.pause_frame = playback.current_frame + frames;
	playback.script_running = true;
	playback.should_unpause = true;
}

static void Generic_Advance(int frame)
{
	if(frame > playback.current_frame)
		Continue_Script(frame - playback.current_frame);
	else if (frame < LOWEST_FRAME)
	{
		Con_Printf("Cannot skip to frame %d\n", frame);
	}
	else
	{
		Run_Script(frame, true);
	}
}

static bool CurrentFrameHasBlock()
{
	for (int i = 0; i < playback.current_script.blocks.size(); ++i)
	{
		if (playback.current_script.blocks[i].frame == playback.current_frame)
		{
			return true;
		}
	}

	return false;
}

static int AddBlock(int frame)
{
	FrameBlock block;
	block.frame = frame;
	block.parsed = true;

	if (playback.current_script.blocks.empty())
	{
		playback.current_script.blocks.push_back(block);
		return 0;
	}
	else
	{
		int i;

		for (i = 0; i < playback.current_script.blocks.size(); ++i)
		{
			if (playback.current_script.blocks[i].frame > frame)
			{
				playback.current_script.blocks.insert(playback.current_script.blocks.begin() + i, block);
				return i;
			}
		}

		playback.current_script.blocks.insert(playback.current_script.blocks.begin() + i, block);
		return i;
	}

}

static FrameBlock* GetBlockForFrame()
{
	if (playback.current_script.blocks.empty())
	{
		Con_Printf("Cannot add blocks to an empty script!\n");
		return nullptr;
	}

	if (CurrentFrameHasBlock())
	{
		return &playback.current_script.blocks[playback.GetCurrentBlockNumber()];
	}

	int block = AddBlock(playback.current_frame);
	return &playback.current_script.blocks[block];
}

static float Get_Stacked_Value(const char* name)
{
	if (playback.stacked.convars.find(name) != playback.stacked.convars.end())
	{
		return playback.stacked.convars[name];
	}
	else
		return Get_Default_Value(name);
}

static float Get_Existing_Value(const char* name)
{
	auto curblock = GetBlockForFrame();

	if (curblock->convars.find(name) != curblock->convars.end())
	{
		return curblock->convars[name];
	}
	else
		return Get_Stacked_Value(name);
}

static bool Get_Stacked_Toggle(const char* cmd_name)
{
	if (playback.stacked.toggles.find(cmd_name) != playback.stacked.toggles.end())
		return playback.stacked.toggles[cmd_name];
	else
		return false;
}

static bool Get_Existing_Toggle(const char* cmd_name)
{
	auto curblock = GetBlockForFrame();

	if (curblock->toggles.find(cmd_name) != curblock->toggles.end())
		return curblock->toggles[cmd_name];
	else
		return Get_Stacked_Toggle(cmd_name);
}

static void SetConvar(const char* name, float new_val, bool silent=false)
{
	float old_val = Get_Existing_Value(name);

	if (new_val == old_val)
	{
		if(!silent)
			Con_Printf("Value identical to old value.\n");
		return;
	}

	playback.last_edited = Sys_DoubleTime();
	auto block = GetBlockForFrame();
	if (!silent)
		sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "Block: Added %s %f", name, new_val);
	SCR_CenterPrint(BUFFER);

	if (Get_Stacked_Value(name) == new_val && block->convars.find(name) != block->convars.end())
	{
		block->convars.erase(name);
		return;
	}
	block->convars[name] = new_val;
}

void SetToggle(const char* cmd, bool new_value)
{
	playback.last_edited = Sys_DoubleTime();
	auto block = GetBlockForFrame();

	sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "Block: Added %c%s", new_value ? '+' : '-', cmd);
	SCR_CenterPrint(BUFFER);

	if (Get_Stacked_Toggle(cmd) == new_value && block->toggles.find(cmd) != block->toggles.end())
	{
		block->toggles.erase(cmd);
		return;
	}

	block->toggles[cmd] = new_value;
}


void Script_Playback_Host_Frame_Hook()
{
	if (playback.In_Edit_Mode() && m_state == MouseState::Strafe)
	{
		float yaw = Round(cl.viewangles[YAW], tas_edit_snap_threshold.value);
		SetConvar("tas_strafe_yaw", yaw, true);
	}

	if (tas_gamestate == paused && playback.should_unpause)
	{
		tas_gamestate = unpaused;
		playback.should_unpause = false;
	}

	if (tas_gamestate == unpaused)
		m_state = MouseState::Locked;

	if(tas_gamestate == paused || !playback.script_running)
		return;
	else if (playback.current_frame == playback.pause_frame)
	{
		tas_gamestate = paused;
		playback.script_running = false;
		old_yaw = cl.viewangles[YAW];
		old_pitch = cl.viewangles[PITCH];
		return;
	}

	int current_block = playback.GetCurrentBlockNumber();

	while (current_block < playback.current_script.blocks.size() && playback.current_script.blocks[current_block].frame <= playback.current_frame)
	{
		auto& block = playback.current_script.blocks[current_block];

		std::string cmd = block.GetCommand();
		playback.stacked.Stack(block);
		AddAfterframes(0, cmd.c_str());
		++current_block;
	}

	playback.last_edited = Sys_DoubleTime();
	++playback.current_frame;
}

void Script_Playback_IN_Move_Hook(usercmd_t * cmd)
{
	if (tas_playing.value && tas_gamestate == paused)
	{
		if(m_state == MouseState::Pitch || m_state == MouseState::Locked)
		{
			cl.viewangles[YAW] = old_yaw;
		}
		if (m_state == MouseState::Yaw || m_state == MouseState::Locked)
		{
			cl.viewangles[PITCH] = old_pitch;
		}
	}
}

static int Get_Current_Frame()
{
	if (!tas_playing.value)
		return 0;

	return playback.current_frame;
}

const PlaybackInfo& GetPlaybackInfo()
{
	return playback;
}

void Cmd_TAS_Script_Init(void)
{
	if (Cmd_Argc() < 4)
	{
		Con_Print("Usage: tas_script_init <filename> <map> <difficulty>\n");
		return;
	}
	
	char name[256];
	sprintf(name, "%s/tas/%s", com_gamedir, Cmd_Argv(1));
	COM_ForceExtension(name, ".qtas");

	playback.current_script = TASScript(name);
	FrameBlock b1;
	b1.frame = 1;
	b1.Add_Command("disconnect");
	b1.Add_Command("tas_set_seed");
	
	FrameBlock b2;
	b2.frame = 2;
	std::string map = Cmd_Argv(2);
	int difficulty = atoi(Cmd_Argv(3));
	b2.Add_Command("skill " + std::to_string(difficulty));
	b2.Add_Command("record demo " + map);
	
	FrameBlock b3;
	b3.frame = 8;

	playback.current_script.blocks.push_back(b1);
	playback.current_script.blocks.push_back(b2);
	playback.current_script.blocks.push_back(b3);
	playback.current_script.Write_To_File();
	Con_Printf("Initialized script into file %s, map %s and difficulty %d\n", name, map.c_str(), difficulty);
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

	Clear_Bookmarks();
	playback.current_script = TASScript(name);
	playback.current_script.Load_From_File();
	playback.last_edited = Sys_DoubleTime();
	Con_Printf("Script %s loaded with %u blocks.\n", Cmd_Argv(1), playback.current_script.blocks.size());
}

void Cmd_TAS_Script_Play(void)
{
	if (Cmd_Argc() > 1)
		Cmd_TAS_Script_Load();
	Run_Script(-1);
}

void Cmd_TAS_Script_Stop(void)
{
	playback.script_running = false;
	ClearAfterframes();
	Cmd_TAS_Cmd_Reset_f();
}

void Cmd_TAS_Script_Skip(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_script_skip <frame>\n");
		return;
	}

	int frame = atoi(Cmd_Argv(1));
	if (frame < 0)
		frame = playback.Get_Last_Frame() - frame - 1;
	Run_Script(frame, true);
}

void Skip_To_Block(int block)
{
	if (block >= playback.current_script.blocks.size() || block < LOWEST_BLOCK)
	{
		Con_Printf("Tried to move to block %d, which is out of bounds!\n", block);
		return;
	}

	int frame = playback.current_script.blocks[block].frame;
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
		block = playback.current_script.blocks.size() - blocks_from_end;
	}

	if (block >= playback.current_script.blocks.size() || block < LOWEST_BLOCK)
	{
		Con_Printf("Tried to move to block %d, which is out of bounds!\n", block);
		return;
	}
	Skip_To_Block(block);
}

void Cmd_TAS_Script_Advance(void)
{
	if (playback.script_running)
	{
		Con_Printf("Can't advance while script still running.\n");
		return;
	}

	int frames;

	if (Cmd_Argc() > 1)
		frames = atoi(Cmd_Argv(1));
	else
		frames = 1;

	int target_frame = playback.current_frame + frames;
	Generic_Advance(target_frame);
}

void Cmd_TAS_Script_Advance_Block(void)
{
	if (playback.script_running)
	{
		Con_Printf("Can't advance while script still running.\n");
		return;
	}

	int blocks;

	if (Cmd_Argc() > 1)
		blocks = atoi(Cmd_Argv(1));
	else
		blocks = 1;

	int target_block = playback.GetCurrentBlockNumber() + blocks;

	if (target_block < LOWEST_BLOCK || target_block >= playback.current_script.blocks.size())
	{
		Con_Printf("Tried to move to block %d, which is out of bounds!\n", target_block);
		return;
	}

	auto& block = playback.current_script.blocks[target_block];
	int frame = block.frame;
	Generic_Advance(frame);
}

void Cmd_TAS_Edit_Prune(void)
{
	for(int i= playback.current_script.blocks.size() - 1; i >= LOWEST_FRAME; --i)
	{
		auto element = &playback.current_script.blocks[i];
		if(element->convars.empty() && element->commands.empty() && element->toggles.empty())
			playback.current_script.blocks.erase(playback.current_script.blocks.begin() + i);
	}

}

void Cmd_TAS_Edit_Save(void)
{
	if(Cmd_Argc() > 1)
		playback.current_script.file_name = Cmd_Argv(1);
	playback.current_script.Write_To_File();
}

void Cmd_TAS_Edit_Strafe(void)
{
	SetConvar("tas_strafe", 1, true);
	m_state = MouseState::Strafe;
}

void Cmd_TAS_Edit_Set_Pitch(void)
{
	m_state = MouseState::Pitch;
}

void Cmd_TAS_Edit_Set_Yaw(void)
{
	m_state = MouseState::Yaw;
}

void Cmd_TAS_Edit_Set_View(void)
{
	m_state = MouseState::Mixed;
}

void Cmd_TAS_Edit_Shrink(void)
{
	int current_block = playback.GetCurrentBlockNumber();
	if (current_block >= playback.current_script.blocks.size())
	{
		Con_Printf("Already beyond last block.\n");
	}
	else
	{
		int new_size;

		if(CurrentFrameHasBlock())
			new_size = current_block + 1;
		else
			new_size = current_block;

		if (new_size < playback.current_script.blocks.size())
		{
			if (new_size <= LOWEST_BLOCK)
			{
				Con_Printf("Cannot delete all blocks!\n");
				return;
			}
			else
				playback.current_script.blocks.resize(new_size);
		}

	}

}

void Cmd_TAS_Edit_Delete(void)
{
	int current_block = playback.GetCurrentBlockNumber();
	if (current_block <= LOWEST_BLOCK)
	{
		Con_Print("Cannot delete first block.\n");
		return;
	}
	else if (!CurrentFrameHasBlock())
	{
		Con_Print("Current frame has no block to delete.\n");
		return;
	}

	SCR_CenterPrint("Block deleted.\n");
	playback.current_script.blocks.erase(playback.current_script.blocks.begin() + current_block);
}

void Shift_Block(const FrameBlock& new_block)
{
	bool set = false;

	for (auto i = 0; i < playback.current_script.blocks.size(); ++i)
	{
		auto& block = playback.current_script.blocks[i];

		if (block.frame == new_block.frame)
		{
			Con_Printf("Cannot shift to an existing block!\n");
			return;
		}
		else if (block.frame > new_block.frame)
		{
			playback.current_script.blocks.insert(playback.current_script.blocks.begin() + i, new_block);
			set = true;
			break;
		}
	}

	if (!set)
	{
		playback.current_script.blocks.push_back(new_block);
	}
}

void Cmd_TAS_Edit_Shift(void)
{
	if (!CurrentFrameHasBlock())
	{
		Con_Printf("No block found on current frame, nothing to shift!\n");
		return;
	}
	else if (Cmd_Argc() == 1)
	{
		Con_Printf("Usage: tas_edit_shift <frames>\n");
		return;
	}

	int frames = atoi(Cmd_Argv(1));
	int new_frame = playback.current_frame + frames;

	if (new_frame < LOWEST_FRAME)
	{
		Con_Printf("Cannot move frame past %d\n", LOWEST_FRAME);
		return;
	}

	int current_block = playback.GetCurrentBlockNumber();
	auto block = playback.current_script.blocks[current_block];
	block.frame = new_frame;
	playback.current_script.blocks.erase(playback.current_script.blocks.begin() + current_block);
	Shift_Block(block);

	Generic_Advance(new_frame);
}

void Cmd_TAS_Edit_Shift_Stack(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Printf("Usage: tas_edit_shift <frames>\n");
		return;
	}

	int frames = atoi(Cmd_Argv(1));
	int current_block = playback.GetCurrentBlockNumber();

	if (current_block == playback.current_script.blocks.size() - 1)
	{
		if(frames > 0)
			AddBlock(playback.current_frame + 1);
		else
		{
			Con_Printf("Cannot shift stack backwards, nothing to shift.\n");
			return;
		}
	}

	std::vector<FrameBlock> insert_blocks;

	// Frame check
	for (int i = playback.current_script.blocks.size() - 1; i >= LOWEST_BLOCK && playback.current_script.blocks[i].frame > playback.current_frame; --i)
	{
		auto& block = playback.current_script.blocks[i];
		int new_frame = block.frame + frames;

		if (new_frame < LOWEST_FRAME || new_frame <= playback.current_frame)
		{
			Con_Printf("Cannot move frame to position %d\n", new_frame);
			return;
		}
	}

	for (int i = playback.current_script.blocks.size() - 1; i >= LOWEST_BLOCK && playback.current_script.blocks[i].frame > playback.current_frame; --i)
	{
		auto block = playback.current_script.blocks[i];
		block.frame += frames;
		playback.current_script.blocks.pop_back();
		insert_blocks.push_back(block);
	}

	for(auto& block : insert_blocks)
		Shift_Block(block);
}

void Cmd_TAS_Edit_Add_Empty(void)
{
	GetBlockForFrame();
}

void Cmd_TAS_Confirm(void)
{
	if (m_state == MouseState::Locked)
		return;

	if (m_state == MouseState::Strafe)
	{
		float yaw = Round(cl.viewangles[YAW], tas_edit_snap_threshold.value);
		SetConvar("tas_strafe", 1);
		SetConvar("tas_strafe_yaw", yaw);
	}
	else if (m_state == MouseState::Mixed || m_state == MouseState::Yaw)
	{
		SetConvar("tas_view_yaw", cl.viewangles[YAW]);
	}
	else if (m_state == MouseState::Mixed || m_state == MouseState::Pitch)
	{
		SetConvar("tas_view_pitch", cl.viewangles[PITCH]);
	}

	m_state = MouseState::Locked;
}

void Cmd_TAS_Cancel(void)
{
	if (m_state == MouseState::Locked)
		return;

	m_state = MouseState::Locked;
	SetConvar("tas_strafe", Get_Stacked_Value("tas_strafe"));
	SetConvar("tas_strafe_yaw", Get_Stacked_Value("tas_strafe_yaw"));
}

void Cmd_TAS_Revert(void)
{
	if (m_state == MouseState::Locked)
		return;

	if (m_state == MouseState::Strafe)
	{
		SetConvar("tas_strafe_yaw", Get_Stacked_Value("tas_strafe_yaw"));
		SetConvar("tas_strafe", Get_Stacked_Value("tas_strafe"));
	}
	else if (m_state == MouseState::Mixed || m_state == MouseState::Yaw)
	{
		SetConvar("tas_view_yaw", Get_Stacked_Value("tas_view_yaw"));
	}
	else if (m_state == MouseState::Mixed || m_state == MouseState::Pitch)
	{
		SetConvar("tas_view_pitch", Get_Stacked_Value("tas_view_pitch"));
	}
	m_state = MouseState::Locked;
}

void Cmd_TAS_Reset(void)
{
	if (m_state == MouseState::Locked)
		return;

	if (m_state == MouseState::Strafe)
	{
		SetConvar("tas_strafe", 0);
	}
	else if (m_state == MouseState::Mixed || m_state == MouseState::Yaw)
	{
		SetConvar("tas_view_yaw", INVALID_ANGLE);
	}
	else if (m_state == MouseState::Mixed || m_state == MouseState::Pitch)
	{
		SetConvar("tas_view_pitch", -1);
	}
	m_state = MouseState::Locked;
}

void Clear_Bookmarks()
{
	playback.current_script.bookmarks.clear();
}

void Cmd_TAS_Bookmark_Frame(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_bookmark_frame <bookmark name>\n");
		return;
	}

	std::string name = Cmd_Argv(1);
	playback.current_script.bookmarks[name] = Bookmark(playback.current_frame, true);
}

void Cmd_TAS_Bookmark_Block(void)
{
	int current_block = playback.GetCurrentBlockNumber();

	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_bookmark_block <bookmark name>\n");
		return;
	}
	else if (!CurrentFrameHasBlock())
	{
		Con_Print("Current frame has no block.\n");
		return;
	}

	std::string name = Cmd_Argv(1);
	playback.current_script.bookmarks[name] = Bookmark(current_block, false);
}

void Cmd_TAS_Bookmark_Skip(void)
{
	if (Cmd_Argc() == 1)
	{
		Con_Print("Usage: tas_bookmark_block <bookmark name>\n");
		return;
	}

	std::string name = Cmd_Argv(1);
	if (playback.current_script.bookmarks.find(name) == playback.current_script.bookmarks.end())
	{
		Con_Printf("Usage: No bookmark with name %s\n", Cmd_Argv(1));
		return;
	}

	auto& bookmark = playback.current_script.bookmarks[name];
	if (bookmark.frame)
	{
		Run_Script(bookmark.index, true);
	}
	else
	{
		Skip_To_Block(bookmark.index);
	}
}

qboolean Script_Playback_Cmd_ExecuteString_Hook(const char * text)
{
	if (!playback.In_Edit_Mode())
		return qfalse;

	char* name = Cmd_Argv(0);

	if (IsGameplayCvar(name))
	{
		if (Cmd_Argc() == 1)
		{
			Con_Printf("No argument for cvar given.\n");
			return qfalse;
		}

		float new_val = atof(Cmd_Argv(1));
		SetConvar(name, new_val);

		return qtrue;
	}
	else if (IsDownCmd(name))
	{
		auto cmd = name + 1;
			
		bool old_value = Get_Existing_Toggle(cmd);
		bool new_value = !old_value;
		SetToggle(cmd, new_value);

		return qtrue;
	}
	else if (strstr(name, "impulse") == name)
	{
		playback.last_edited = Sys_DoubleTime();
		auto block = GetBlockForFrame();
		sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "Block: Added %s %s", name, Cmd_Argv(1));
		SCR_CenterPrint(BUFFER);
		block->commands.clear();
		sprintf_s(BUFFER, ARRAYSIZE(BUFFER), "Selected weapon %s", Cmd_Argv(1));
		block->Add_Command(BUFFER);

		return qtrue;
	}

	return qfalse;
}

PlaybackInfo::PlaybackInfo()
{
	pause_frame = -1;
	current_frame = -1;
	script_running = false;
	should_unpause = false;
}

const FrameBlock* PlaybackInfo::Get_Current_Block(int frame) const
{
	int blck = GetCurrentBlockNumber(frame);

	if(blck >= playback.current_script.blocks.size())
		return nullptr;
	else
		return &playback.current_script.blocks[blck];
}

const FrameBlock* PlaybackInfo::Get_Stacked_Block() const
{
	return &stacked;
}


int PlaybackInfo::GetCurrentBlockNumber(int frame) const
{
	if(frame == -1)
		frame = current_frame;

	for (int i = 0; i < current_script.blocks.size(); ++i)
	{
		if (current_script.blocks[i].frame >= frame)
		{
			return i;
		}
	}

	return current_script.blocks.size() - 1;
}

int PlaybackInfo::Get_Number_Of_Blocks() const
{
	return current_script.blocks.size();
}

int PlaybackInfo::Get_Last_Frame() const
{
	if (playback.current_script.blocks.empty())
		return 0;
	else
	{
		return playback.current_script.blocks[playback.current_script.blocks.size() - 1].frame;
	}
}

bool PlaybackInfo::In_Edit_Mode() const
{
	return !script_running && tas_gamestate == paused && tas_playing.value
		&& !current_script.blocks.empty();
}
