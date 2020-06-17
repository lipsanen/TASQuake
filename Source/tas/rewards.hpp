#pragma once

#include "cpp_quakedef.hpp"

extern cvar_t tas_reward_size;
extern cvar_t tas_reward_display;

void Cmd_TAS_Reward_Gate();
void Cmd_TAS_Reward_Pop();
void Cmd_TAS_Reward_Delete_All();
void Cmd_TAS_Reward_Intermission();
void Cmd_TAS_Reward_Save();
void Cmd_TAS_Reward_Load();
void Cmd_TAS_Reward_Dump();
