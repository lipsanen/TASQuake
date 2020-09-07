#pragma once

#include "cpp_quakedef.hpp"

// desc: Controls the reward gate size
extern cvar_t tas_reward_size;
// desc: Displays rewards
extern cvar_t tas_reward_display;

// desc: Creates a reward gate
void Cmd_TAS_Reward_Gate();
// desc: Delete reward gate
void Cmd_TAS_Reward_Pop();
// desc: Deletes all gates
void Cmd_TAS_Reward_Delete_All();
// desc: Add intermission gate
void Cmd_TAS_Reward_Intermission();
// desc: Save rewards into file
void Cmd_TAS_Reward_Save();
// desc: Load rewards from file
void Cmd_TAS_Reward_Load();
// desc: Dumps rewards to IPC.
void Cmd_TAS_Reward_Dump();
