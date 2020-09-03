#include "cpp_quakedef.hpp"
#include "json.hpp"

extern cvar_t tas_ipc;
extern cvar_t tas_ipc_feedback;
extern cvar_t tas_ipc_port;
extern cvar_t tas_ipc_timeout;
extern cvar_t tas_ipc_verbose;

void IPC_Init();
bool IPC_Active();
void IPC_Loop();
void IPC_Send(const nlohmann::json& msg);
void Cmd_TAS_IPC_Simulate();
void Cmd_TAS_IPC_Print_Posvel();
void Cmd_TAS_IPC_Print_Seed();
void Cmd_TAS_IPC_Print_Playback();
void Cmd_TAS_IPC_Condition();
void Cmd_TAS_IPC_Condition_Disable();