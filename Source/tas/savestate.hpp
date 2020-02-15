#include "cpp_quakedef.hpp"

int Savestate_Load_State(int frame);
void Savestate_Frame_Hook(int frame);
void Savestate_Script_Updated(int frame);
void Savestate_Playback_Started(int target_frame);

// desc: Make a manual savestate on current frame
void Cmd_TAS_Savestate(void);

// desc: When set to 1, use automatic savestates.
extern cvar_t tas_savestate_auto;
// desc: How often the game should automatically savestate
extern cvar_t tas_savestate_interval;
// desc: How many seconds prior to the skipped frame should the script start savestating?
extern cvar_t tas_savestate_prior;

void Cmd_TAS_LS(void);
void Cmd_TAS_SS(void);
