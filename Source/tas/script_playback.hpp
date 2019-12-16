#pragma once

#include "cpp_quakedef.hpp"
#include "script_parse.hpp"

struct PlaybackInfo 
{
	FrameBlock* current_block;
	FrameBlock* stacked;
	int current_frame;
	int last_frame;
};

void Script_Playback_Host_Frame_Hook();
qboolean Script_Playback_Cmd_ExecuteString_Hook(const char* text);
void Script_Playback_IN_Move_Hook(usercmd_t* cmd);
PlaybackInfo GetPlaybackInfo();

void Cmd_TAS_Script_Init(void);
void Cmd_TAS_Script_Load(void);
void Cmd_TAS_Script_Play(void);
void Cmd_TAS_Script_Stop(void);
void Cmd_TAS_Script_Skip(void);
void Cmd_TAS_Script_Skip_Block(void);
void Cmd_TAS_Script_Advance(void);
void Cmd_TAS_Script_Advance_Block(void);

extern cvar_t tas_edit_backups;
extern cvar_t tas_edit_snap_threshold;
extern cvar_t tas_edit_autosave;

void Cmd_TAS_Edit_Save(void);
void Cmd_TAS_Edit_Strafe(void);
void Cmd_TAS_Edit_Set_Pitch(void);
void Cmd_TAS_Edit_Set_Yaw(void);
void Cmd_TAS_Edit_Set_View(void);
void Cmd_TAS_Edit_Shrink(void);
void Cmd_TAS_Edit_Shift(void);
void Cmd_TAS_Edit_Add_Empty(void);
//void Cmd_TAS_Edit_Delete_Cmd(void);


void Cmd_TAS_Confirm(void);
void Cmd_TAS_Cancel(void);
void Cmd_TAS_Revert(void);
void Cmd_TAS_Reset(void);