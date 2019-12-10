#pragma once

#include "cpp_quakedef.hpp"

extern cvar_t tas_strafe;
extern cvar_t tas_strafe_yaw;
extern cvar_t tas_strafe_yaw_offset;
enum class StrafeType { None = 0, MaxAccel, Straight };

void Strafe(usercmd_t* cmd);
void Strafe_Jump_Check();
void IN_TAS_Jump_Down(void);
void IN_TAS_Jump_Up(void);
void IN_TAS_Lgagst_Down(void);
void IN_TAS_Lgagst_Up(void);
void Cmd_Print_Vel(void);
void Cmd_Print_Origin(void);
void Cmd_Print_Moves(void);