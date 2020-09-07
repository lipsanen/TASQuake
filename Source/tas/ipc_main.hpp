#include "cpp_quakedef.hpp"
#include "json.hpp"

// desc: Turns on IPC.
extern cvar_t tas_ipc;
// desc: Turns on IPC feedback mode.
extern cvar_t tas_ipc_feedback;
// desc: IPC port
extern cvar_t tas_ipc_port;
// desc: IPC timeout in feedback mode
extern cvar_t tas_ipc_timeout;
// desc: Enables a bunch of debug messages in IPC
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