#pragma once

#include "cpp_quakedef.hpp"

void Script_Playback_Host_Frame_Hook();

void Cmd_TAS_Script_Load(void);
void Cmd_TAS_Script_Play(void);
void Cmd_TAS_Script_Stop(void);
void Cmd_TAS_Script_Skip(void);
void Cmd_TAS_Script_Skip_Block(void);
void Cmd_TAS_Script_Advance(void);
void Cmd_TAS_Script_Advance_Block(void);

extern cvar_t tas_edit_backups;
extern cvar_t tas_edit_snap_treshold;

void Cmd_TAS_Edit_Strafe(void);
void Cmd_TAS_Edit_Set_Pitch(void);
void Cmd_TAS_Edit_Set_Yaw(void);
void Cmd_TAS_Edit_Set_View(void);
void Cmd_TAS_Edit_Delete_Cmd(void);
void Cmd_TAS_Edit_Shrink(void);
void Cmd_TAS_Edit_Save(void);
