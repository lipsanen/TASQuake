#include "camera.hpp"
#include "drag_editing.hpp"
#include "libtasquake/utils.hpp"

cvar_t tas_freecam = {"tas_freecam", "0"};
cvar_t tas_freecam_speed = {"tas_freecam_speed", "320"};
static vec3_t camera_origin;
static vec3_t camera_angles;
static vec3_t buttons;

bool In_Freecam()
{
	return tas_freecam.value != 0 && tas_gamestate == paused;
}

void Get_Camera_PosAngles(TASQuake::Vector& pos, TASQuake::Vector& angles)
{
	VectorCopy(camera_origin, pos);
	VectorCopy(camera_angles, angles);
}

void Camera_MouseMove_Hook(int mousex, int mousey)
{
	if (!In_Freecam() || TASQuake::IsDragging())
	{
		return;
	}

	camera_angles[0] = bound(-90, camera_angles[0] + mousey * m_pitch.value, 90);
	camera_angles[1] = ToQuakeAngle(camera_angles[1] - mousex * m_yaw.value);
	vec3_t fwd, right;
	AngleVectors(camera_angles, fwd, right, NULL);

	float fwdscale = static_cast<float>(buttons[0] * tas_freecam_speed.value * host_frametime);
	float rightscale = static_cast<float>(buttons[1] * tas_freecam_speed.value * host_frametime);
	float upscale = buttons[2] * tas_freecam_speed.value * host_frametime;

	VectorScaledAdd(camera_origin, fwd, fwdscale, camera_origin);
	VectorScaledAdd(camera_origin, right,  rightscale, camera_origin);
	camera_origin[2] += upscale;
}

qboolean Camera_Refdef_Hook()
{
	if (!In_Freecam())
	{
		if (tas_gamestate == unpaused)
		{
			VectorCopy(r_refdef.vieworg, camera_origin);
			VectorCopy(r_refdef.viewangles, camera_angles);
		}

		return qtrue;
	}

	VectorCopy(camera_origin, r_refdef.vieworg);
	VectorCopy(camera_angles, r_refdef.viewangles);

	return qfalse;
}

void SendMovementCommand(const char* cvar)
{
#define CommandCheck(name, button, value) 	if (strstr(cvar, "+" #name) == cvar) \
											{ \
												buttons[button] = value; \
											} \
											else if (strstr(cvar, "-" #name) == cvar) \
											{ \
												buttons[button] = 0; \
											}
	CommandCheck(forward, 0, 1);
	CommandCheck(back, 0, -1);
	CommandCheck(moveright, 1, 1);
	CommandCheck(moveleft, 1, -1);
	CommandCheck(moveup, 2, 1);
	CommandCheck(movedown, 2, -1);
}

void Camera_Init()
{
	VectorCopy(vec3_origin, camera_origin);
	VectorCopy(vec3_origin, camera_angles);
	VectorCopy(vec3_origin, buttons);
}
