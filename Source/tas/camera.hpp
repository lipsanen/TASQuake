#include "cpp_quakedef.hpp"

void SendMovementCommand(const char* name);
void Camera_Init();
void Camera_MouseMove_Hook(int mousex, int mousey);
qboolean Camera_Refdef_Hook();

extern cvar_t tas_freecam;
extern cvar_t tas_freecam_speed;