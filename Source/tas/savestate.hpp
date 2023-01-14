#include "cpp_quakedef.hpp"

int Savestate_Load_State(int frame);
void Savestate_Frame_Hook(int frame, int target_frame);
void Savestate_Script_Updated(int frame);
void Savestate_Playback_Started(int target_frame);

// desc: Make a manual savestate on current frame
void Cmd_TAS_Savestate(void);

// desc: When set to 1, use automatic savestates in level transitions.
extern cvar_t tas_savestate_auto;
// desc: Enable/disable savestates in TASes.
extern cvar_t tas_savestate_enabled;
// desc: Assign a prefix to savestate names
extern cvar_t tas_savestate_prefix;

// desc: Load savestate. Probably don't use this.
void Cmd_TAS_LS(void);
// desc: Clear savestates
void Cmd_TAS_SS_Clear(void);
void Restore_Client();
void Savestate_Init();