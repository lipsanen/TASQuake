#pragma once
#include "cpp_quakedef.hpp"

void Cmd_TAS_Full_Reset_f(void);
void Cmd_TAS_Reset_Movement(void);
float Get_Default_Value(const char* name);
bool IsGameplayCvar(cvar_t* var);
bool IsGameplayCvar(const char* text);
bool IsUpCmd(const char* text);
bool IsUpCmd(cmd_function_t* func);
bool IsDownCmd(const char* text);
bool IsDownCmd(cmd_function_t* func);