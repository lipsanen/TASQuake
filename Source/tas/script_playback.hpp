#pragma once

#include "cpp_quakedef.hpp"

#include "script_parse.hpp"

struct PlaybackInfo
{
	PlaybackInfo();

	int pause_frame;
	int current_frame;
	bool script_running;
	bool should_unpause;
	double last_edited;

	FrameBlock stacked;
	TASScript current_script;

	const FrameBlock* Get_Stacked_Block() const;
	const FrameBlock* Get_Current_Block(int frame = -1) const;
	int GetBlockNumber(int frame = -1) const;
	int Get_Number_Of_Blocks() const;
	int Get_Last_Frame() const;
	bool In_Edit_Mode() const;
};

void Script_Playback_Host_Frame_Hook();
qboolean Script_Playback_Cmd_ExecuteString_Hook(const char* text);
void Script_Playback_IN_Move_Hook(usercmd_t* cmd);
PlaybackInfo* GetPlaybackInfo();
bool TAS_Script_Load(const char* name);
void Run_Script(int frame, bool skip = false, bool ss=true);
bool CurrentFrameHasBlock(int frame = -1);
void Skip_To_Block(int block);

// desc: Usage: tas_script_init <filename> <map> <difficulty>. Initializes a TAS script.
void Cmd_TAS_Script_Init(void);
// desc: Usage: tas_script_load <filename>. Loads a TAS script from file.
void Cmd_TAS_Script_Load(void);
// desc: Plays a script.
void Cmd_TAS_Script_Play(void);
// desc: Stop a script from playing. This is the "reset everything that the game is doing" command.
void Cmd_TAS_Script_Stop(void);
// desc: Usage: tas_script_skip <frame>. Skips to the frame number given as parameter. Use with negative values \
to skip to the end, e.g. -1 skips to last frame, -2 skips to second last and so on.
void Cmd_TAS_Script_Skip(void);
// desc: Usage: tas_script_skip_block <block>. Skips to this number of block. Works with negative numbers similarly to \
regular skip
void Cmd_TAS_Script_Skip_Block(void);
// desc: Usage: tas_script_advance <frames>. Advances script by number of frames given as argument. Use negative values \
to go backwards.
void Cmd_TAS_Script_Advance(void);
// desc: Usage: tas_script_advance <frames>. Advances script by number of blocks given as argument. Use negative values \
to go backwards.
void Cmd_TAS_Script_Advance_Block(void);

extern cvar_t tas_edit_backups;
extern cvar_t tas_edit_snap_threshold;

// desc: Removes all blocks with no content
void Cmd_TAS_Edit_Prune(void);
// desc: Usage: tas_edit_save [savename]. Saves the script to file \
, if used without arguments saves to the same filename where the script was loaded from
void Cmd_TAS_Edit_Save(void);
// desc: Enters strafe edit mode
void Cmd_TAS_Edit_Strafe(void);
// desc: Enters swim edit mode
void Cmd_TAS_Edit_Swim(void);
// desc: Enters pitch editing mode
void Cmd_TAS_Edit_Set_Pitch(void);
// desc: Enters yaw editing mode
void Cmd_TAS_Edit_Set_Yaw(void);
// desc: Enters pitch/yaw editing mode
void Cmd_TAS_Edit_Set_View(void);
// desc: Removes all frameblocks after current one
void Cmd_TAS_Edit_Shrink(void);
// desc: Delete current block
void Cmd_TAS_Edit_Delete(void);
// desc: Usage: tas_edit_shift <frames>. Shift current frameblock by frames. Use with a negative argument \
to shift backwards
void Cmd_TAS_Edit_Shift(void);
// desc: Shifts all frameblocks after current one
void Cmd_TAS_Edit_Shift_Stack(void);
// desc: Adds empty frameblock
void Cmd_TAS_Edit_Add_Empty(void);
//void Cmd_TAS_Edit_Delete_Cmd(void);
// desc: Add a number of shots to an interval
void Cmd_TAS_Edit_Shots(void);

// desc: Usage: tas_edit_random_toggle <command> <min> <max>. Add a toggle at a random point in the script.
void Cmd_TAS_Edit_Random_Toggle(void);

// desc: Confirms change in editing mode
void Cmd_TAS_Confirm(void);
// desc: Cancel change in editing mode
void Cmd_TAS_Cancel(void);
// desc: Revert changes to current editing mode to pre-frame values
void Cmd_TAS_Revert(void);
