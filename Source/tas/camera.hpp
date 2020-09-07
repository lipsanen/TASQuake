#include "cpp_quakedef.hpp"

void SendMovementCommand(const char* name);
void Camera_Init();
void Camera_MouseMove_Hook(int mousex, int mousey);
qboolean Camera_Refdef_Hook();

// desc: Turns on freecam mode while paused in a TAS
extern cvar_t tas_freecam;
// desc: Camera speed while freecamming
extern cvar_t tas_freecam_speed;