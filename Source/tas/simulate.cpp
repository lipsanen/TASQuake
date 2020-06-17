#include "simulate.hpp"

#include "draw.hpp"
#include "strafing.hpp"
#include "utils.hpp"
#include "afterframes.hpp"

trace_t SV_Move_Proxy(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t* passedict)
{
	return SV_Move(start, mins, maxs, end, type, sv_player);
}

void SV_Impact(edict_t* e1, edict_t* e2)
{
	// Hopefully this does nothing important
}

#define STOP_EPSILON 0.1

int ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float backoff, change;
	int i, blocked;

	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1; // floor
	if (!normal[2])
		blocked |= 2; // step

	backoff = DotProduct(in, normal) * overbounce;

	if (backoff > 0)
	{
		int a = 0;
	}

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}

trace_t SV_PushEntity(edict_t* ent, vec3_t push)
{
	trace_t trace;
	vec3_t end;

	VectorAdd(ent->v.origin, push, end);

	if (ent->v.movetype == MOVETYPE_FLYMISSILE)
		trace = SV_Move_Proxy(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_MISSILE, ent);
	else if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
		// only clip against bmodels
		trace = SV_Move_Proxy(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NOMONSTERS, ent);
	else
		trace = SV_Move_Proxy(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

	VectorCopy(trace.endpos, ent->v.origin);
	// SV_LinkEdict(ent, true);

	if (trace.ent)
		SV_Impact(ent, trace.ent);

	return trace;
}

int SV_FlyMove(edict_t* ent, float time, trace_t* steptrace, SimulationInfo& info);

int SV_TryUnstick(edict_t* ent, vec3_t oldvel, SimulationInfo& info)
{
	int i, clip;
	vec3_t oldorg, dir;
	trace_t steptrace;

	VectorCopy(ent->v.origin, oldorg);
	VectorCopy(vec3_origin, dir);

	for (i = 0; i < 8; i++)
	{
		// try pushing a little in an axial direction
		switch (i)
		{
		case 0:
			dir[0] = 2;
			dir[1] = 0;
			break;
		case 1:
			dir[0] = 0;
			dir[1] = 2;
			break;
		case 2:
			dir[0] = -2;
			dir[1] = 0;
			break;
		case 3:
			dir[0] = 0;
			dir[1] = -2;
			break;
		case 4:
			dir[0] = 2;
			dir[1] = 2;
			break;
		case 5:
			dir[0] = -2;
			dir[1] = 2;
			break;
		case 6:
			dir[0] = 2;
			dir[1] = -2;
			break;
		case 7:
			dir[0] = -2;
			dir[1] = -2;
			break;
		}

		SV_PushEntity(ent, dir);

		// retry the original move
		ent->v.velocity[0] = oldvel[0];
		ent->v.velocity[1] = oldvel[1];
		ent->v.velocity[2] = 0;
		clip = SV_FlyMove(ent, 0.1, &steptrace, info);

		if (fabs(oldorg[1] - ent->v.origin[1]) > 4 || fabs(oldorg[0] - ent->v.origin[0]) > 4)
		{
			//Con_DPrintf ("unstuck!\n");
			return clip;
		}

		// go back to the original pos and try again
		VectorCopy(oldorg, ent->v.origin);
	}

	VectorCopy(vec3_origin, ent->v.velocity);
	return 7; // still not moving
}

void SV_WallFriction(edict_t* ent, trace_t* trace)
{
	float d, i;
	vec3_t forward, right, up, into, side;

	AngleVectors(ent->v.v_angle, forward, right, up);
	d = DotProduct(trace->plane.normal, forward);

	d += 0.5;
	if (d >= 0)
		return;

	// cut the tangential velocity
	i = DotProduct(trace->plane.normal, ent->v.velocity);
	VectorScale(trace->plane.normal, i, into);
	VectorSubtract(ent->v.velocity, into, side);

	ent->v.velocity[0] = side[0] * (1 + d);
	ent->v.velocity[1] = side[1] * (1 + d);
}

#define MAX_CLIP_PLANES 5
int SV_FlyMove(edict_t* ent, float time, trace_t* steptrace, SimulationInfo& info)
{
	int i, j, bumpcount, numbumps, numplanes, blocked;
	float d, time_left;
	vec3_t dir, planes[MAX_CLIP_PLANES], primal_velocity, original_velocity, new_velocity, end;
	trace_t trace;

	numbumps = 4;

	blocked = 0;
	VectorCopy(ent->v.velocity, original_velocity);
	VectorCopy(ent->v.velocity, primal_velocity);
	numplanes = 0;

	time_left = time;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (!ent->v.velocity[0] && !ent->v.velocity[1] && !ent->v.velocity[2])
			break;

		for (i = 0; i < 3; i++)
			end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

		trace = SV_Move_Proxy(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

		if (trace.allsolid)
		{ // entity is trapped in another solid
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{ // actually covered some distance
			VectorCopy(trace.endpos, ent->v.origin);
			VectorCopy(ent->v.velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break; // moved the entire distance

		if (!trace.ent)
			Sys_Error("SV_FlyMove: !trace.ent");

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1; // floor
			if (trace.ent->v.solid == SOLID_BSP)
			{
				ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
				ent->v.groundentity = EDICT_TO_PROG(trace.ent);
			}
		}
		else
		{
			info.collision = true;
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2; // step
			if (steptrace)
				*steptrace = trace; // save for player extrafriction
		}

		// run the impact function
		SV_Impact(ent, trace.ent);
		if (ent->free)
			break; // removed by the impact function

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{ // this shouldn't really happen
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		for (i = 0; i < numplanes; i++)
		{
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
						break; // not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{ // go along this plane
			VectorCopy(new_velocity, ent->v.velocity);
		}
		else
		{ // go along the crease
			if (numplanes != 2)
			{
				//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy(vec3_origin, ent->v.velocity);
				return 7;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->v.velocity);
			VectorScale(dir, d, ent->v.velocity);
		}

		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		if (DotProduct(ent->v.velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, ent->v.velocity);
			return blocked;
		}
	}

	return blocked;
}

#define STEPSIZE 18
void SV_WalkMove(edict_t* ent, double hfr, SimulationInfo& info, bool nostep = false)
{
	int clip, oldonground;
	vec3_t upmove, downmove, oldorg, oldvel, nosteporg, nostepvel;
	trace_t steptrace, downtrace;
	bool oldcollision = info.collision;

	// do a regular slide move unless it looks like you ran into a step
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	VectorCopy(ent->v.origin, oldorg);
	VectorCopy(ent->v.velocity, oldvel);

	clip = SV_FlyMove(ent, hfr, &steptrace, info);

	if (!(clip & 2))
		return; // move didn't block on a step

	if (!oldonground && ent->v.waterlevel == 0)
		return; // don't stair up while jumping

	if (ent->v.movetype != MOVETYPE_WALK)
		return; // gibbed by a trigger

	if (nostep)
		return;

	if ((int)ent->v.flags & FL_WATERJUMP)
		return;

	if (!oldcollision && info.collision)
		info.collision = false;
	VectorCopy(ent->v.origin, nosteporg);
	VectorCopy(ent->v.velocity, nostepvel);

	// try moving up and forward to go up a step
	VectorCopy(oldorg, ent->v.origin); // back to start pos

	VectorCopy(vec3_origin, upmove);
	VectorCopy(vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2] * hfr;

	// move up
	SV_PushEntity(ent, upmove); // FIXME: don't link?

	// move forward
	ent->v.velocity[0] = oldvel[0];
	ent->v.velocity[1] = oldvel[1];
	ent->v.velocity[2] = 0;
	clip = SV_FlyMove(ent, hfr, &steptrace, info);

	// check for stuckness, possibly due to the limited precision of floats
	// in the clipping hulls
	if (clip)
	{
		if (fabs(oldorg[1] - ent->v.origin[1]) < 0.03125 && fabs(oldorg[0] - ent->v.origin[0]) < 0.03125)
		{ // stepping up didn't make any progress
			clip = SV_TryUnstick(ent, oldvel, info);
		}
	}

	// extra friction based on view angle
	if (clip & 2)
		SV_WallFriction(ent, &steptrace);

	// move down
	downtrace = SV_PushEntity(ent, downmove); // FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ent->v.solid == SOLID_BSP)
		{
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = EDICT_TO_PROG(downtrace.ent);
		}
	}
	else
	{
		// if the push down didn't end up on good ground, use the move without
		// the step up.  This happens near wall / slope combinations, and can
		// cause the player to hop up higher on a slope too steep to climb
		VectorCopy(nosteporg, ent->v.origin);
		VectorCopy(nostepvel, ent->v.velocity);
	}
}

bool SV_CheckWater(edict_t* ent)
{
	int cont;
	vec3_t point;

	point[0] = ent->v.origin[0];
	point[1] = ent->v.origin[1];
	point[2] = ent->v.origin[2] + ent->v.mins[2] + 1;

	ent->v.waterlevel = 0;
	ent->v.watertype = CONTENTS_EMPTY;
	cont = SV_PointContents(point);
	if (cont <= CONTENTS_WATER)
	{
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		point[2] = ent->v.origin[2] + (ent->v.mins[2] + ent->v.maxs[2]) * 0.5;
		cont = SV_PointContents(point);
		if (cont <= CONTENTS_WATER)
		{
			ent->v.waterlevel = 2;
			point[2] = ent->v.origin[2] + ent->v.view_ofs[2];
			cont = SV_PointContents(point);
			if (cont <= CONTENTS_WATER)
				ent->v.waterlevel = 3;
		}
	}

	return ent->v.waterlevel > 1;
}

void SV_AddGravity(edict_t* ent, double hfr)
{
	float ent_gravity;

	eval_t* val;

	val = GETEDICTFIELDVALUE(ent, eval_gravity);
	if (val && val->_float)
		ent_gravity = val->_float;
	else
		ent_gravity = 1.0;
	ent->v.velocity[2] -= ent_gravity * sv_gravity.value * hfr;
}

edict_t* Simulate_SV_TestEntityPosition(edict_t* ent)
{
	trace_t trace;

	trace = SV_Move_Proxy(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, ent);

	if (trace.startsolid)
		return sv.edicts;

	return NULL;
}

void SV_CheckStuck(edict_t* ent)
{
	int i, j;
	int z;
	vec3_t org;

	if (!Simulate_SV_TestEntityPosition(ent))
	{
		VectorCopy(ent->v.origin, ent->v.oldorigin);
		return;
	}

	VectorCopy(ent->v.origin, org);
	VectorCopy(ent->v.oldorigin, ent->v.origin);
	if (!Simulate_SV_TestEntityPosition(ent))
	{
		return;
	}

	for (z = 0; z < 18; z++)
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
			{
				ent->v.origin[0] = org[0] + i;
				ent->v.origin[1] = org[1] + j;
				ent->v.origin[2] = org[2] + z;
				if (!Simulate_SV_TestEntityPosition(ent))
				{
					return;
				}
			}

	VectorCopy(org, ent->v.origin);
}

void PlayerPhysics(SimulationInfo& info)
{
	if (!SV_CheckWater(&info.ent) && !((int)info.ent.v.flags & FL_WATERJUMP))
		SV_AddGravity(&info.ent, info.host_frametime);
	SV_CheckStuck(&info.ent);
	SV_WalkMove(&info.ent, info.host_frametime, info);
}

void Simulate_SV_ClientThink(SimulationInfo& info)
{
	for (int i = 0; i < 3; ++i)
	{
		info.ent.v.v_angle[i] = AngleModDeg(info.viewangles[i]);
	}

	info.time += info.host_frametime;

	client_t t;
	t.cmd.forwardmove = info.fmove;
	t.cmd.sidemove = info.smove;
	t.cmd.upmove = info.upmove;

	server_t backup = sv;
	sv.time = info.time;
	sv.paused = qfalse;

	edict_t* ply = sv_player;
	client_t* host = host_client;

	host_client = &t;
	sv_player = &info.ent;

	SV_ClientThink();

	sv_player = ply;
	host_client = host;
	sv = backup;
}

#define READ_KEY(name) \
	if ((in_##name.state & 1) != 0) \
		info.key_##name.state = 1; \
	else \
		info.key_##name.state = 0;
#define READ_CVAR(name) info.##name = ##name.value

SimulationInfo Get_Sim_Info()
{
	SimulationInfo info;
	info.host_frametime = host_frametime;
	info.ent = *sv_player;
	info.time = sv.time;
	info.jumpflag = sv_player->v.velocity[2];
	info.smove = 0;
	info.fmove = 0;
	info.upmove = 0;
	info.tas_jump = Is_TAS_Jump_Down();
	info.tas_lgagst = Is_TAS_Lgagst_Down();
	info.collision = false;

	VectorCopy(cl.prev_viewangles, info.viewangles);
	for (int i = 0; i < 3; ++i)
		info.ent.v.v_angle[i] = AngleModDeg(cl.prev_viewangles[i]);

	READ_CVAR(cl_forwardspeed);
	READ_CVAR(cl_movespeedkey);
	READ_CVAR(cl_sidespeed);
	READ_CVAR(cl_upspeed);

	READ_KEY(forward);
	READ_KEY(back);
	READ_KEY(moveleft);
	READ_KEY(moveright);
	READ_KEY(up);
	READ_KEY(down);
	READ_KEY(speed);
	READ_KEY(jump);
	if (Gonna_Jump())
	{
		info.key_jump.state = 1;
	}
	info.vars = Get_Current_Vars();

	return info;
}

#define CHECK_INPUT(name, member_name) \
	if (block->toggles.find(#name) != block->toggles.end()) \
	{ \
		if (info.##member_name.state == 0 && block->toggles.at(#name)) \
			info.##member_name.state = 0.5; \
		else if (info.##member_name.state > 0 && !block->toggles.at(#name)) \
			info.##member_name.state = 0; \
	}

#define TOGGLE_BOOL(name) block->toggles.find(#name) != block->toggles.end()

static void ApplyToggles(SimulationInfo& info, const FrameBlock* block)
{
	CHECK_INPUT(forward, key_forward);
	CHECK_INPUT(back, key_back);
	CHECK_INPUT(moveleft, key_moveleft);
	CHECK_INPUT(moveright, key_moveright);
	CHECK_INPUT(moveup, key_up);
	CHECK_INPUT(movedown, key_down);
	CHECK_INPUT(speed, key_speed);
	CHECK_INPUT(jump, key_jump);

	if (TOGGLE_BOOL(tas_lgagst))
		info.tas_lgagst = block->toggles.at("tas_lgagst");

	if (TOGGLE_BOOL(tas_jump))
		info.tas_jump = block->toggles.at("tas_jump");
}

#define CHECK_CVAR(name, member_name) \
	if (block->convars.find(#name) != block->convars.end()) \
	{ \
		info.##member_name = block->convars.at(#name); \
	}

static void ApplyCvars(SimulationInfo& info, const FrameBlock* block)
{
	CHECK_CVAR(cl_forwardspeed, cl_forwardspeed);
	CHECK_CVAR(cl_sidespeed, cl_sidespeed);
	CHECK_CVAR(cl_upspeed, cl_upspeed);
	CHECK_CVAR(cl_movespeedkey, cl_movespeedkey);
	CHECK_CVAR(tas_view_yaw, vars.tas_view_yaw);
	CHECK_CVAR(tas_view_pitch, vars.tas_view_pitch);
	CHECK_CVAR(tas_strafe, vars.tas_strafe);
	CHECK_CVAR(tas_strafe_yaw, vars.tas_strafe_yaw);
	CHECK_CVAR(tas_strafe_pitch, vars.tas_strafe_pitch);
	CHECK_CVAR(tas_strafe_version, vars.tas_strafe_version);
	CHECK_CVAR(tas_anglespeed, vars.tas_anglespeed);

	if (block->convars.find("cl_maxfps") != block->convars.end())
	{
		info.host_frametime = 1.0 / block->convars.at("cl_maxfps");
		info.vars.host_frametime = info.host_frametime;
	}

	if (block->convars.find("tas_strafe_type") != block->convars.end())
	{
		int number = static_cast<int>(block->convars.at("tas_strafe_type"));
		info.vars.tas_strafe_type = static_cast<StrafeType>(number);
	}
}

#define CHECK_HALF_PRESS(member_name) \
	if (info.##member_name.state == 0.5) \
	info.##member_name.state = 1

static void CheckHalfPresses(SimulationInfo& info)
{
	CHECK_HALF_PRESS(key_forward);
	CHECK_HALF_PRESS(key_back);
	CHECK_HALF_PRESS(key_moveleft);
	CHECK_HALF_PRESS(key_moveright);
	CHECK_HALF_PRESS(key_up);
	CHECK_HALF_PRESS(key_down);
	CHECK_HALF_PRESS(key_speed);
	CHECK_HALF_PRESS(key_jump);
}

static void CalculateMoves(SimulationInfo& info)
{
	float factor = info.key_speed.state > 0 ? info.cl_movespeedkey : 1;
	info.fmove = 0;
	info.smove = 0;
	info.upmove = 0;

	info.fmove += info.key_forward.state * info.cl_forwardspeed * factor;
	info.fmove -= info.key_back.state * info.cl_forwardspeed * factor;
	info.smove += info.key_moveright.state * info.cl_sidespeed * factor;
	info.smove -= info.key_moveleft.state * info.cl_sidespeed * factor;
	info.upmove += info.key_up.state * info.cl_upspeed * factor;
	info.upmove -= info.key_down.state * info.cl_upspeed * factor;
}

static void SetStrafe(SimulationInfo& info)
{
	if (info.time <= 1.425)
	{
		info.fmove = 0;
		info.smove = 0;
		info.upmove = 0;
		return;
	}

	usercmd_t cmd;
	cmd.forwardmove = 0;
	cmd.upmove = 0;
	cmd.sidemove = 0;

	StrafeSim(&cmd, &info.ent, &info.viewangles[YAW], &info.viewangles[PITCH], info.vars);

	if (info.vars.tas_strafe)
	{
		info.fmove = cmd.forwardmove;
		info.smove = cmd.sidemove;
		info.upmove = cmd.upmove;
		info.viewangles[ROLL] = 0;
	}
	else
	{
		CalculateMoves(info);
	}
}

void ApplyFrameblock(SimulationInfo& info, const FrameBlock* block)
{
	ApplyToggles(info, block);
	ApplyCvars(info, block);
}

void WaterMove(SimulationInfo& info)
{
	if (!info.ent.v.waterlevel)
	{
		if (((int)info.ent.v.flags & FL_INWATER) != 0)
			info.ent.v.flags = (int)info.ent.v.flags - FL_INWATER;
		return;
	}

	if (((int)info.ent.v.flags & FL_INWATER) == 0)
	{
		info.ent.v.flags = (int)info.ent.v.flags + FL_INWATER;
	}

	if (((int)info.ent.v.flags & FL_WATERJUMP) == 0)
	{
		vec3_t sub;
		VectorCopy(info.ent.v.velocity, sub);
		VectorScale(sub, -0.8 * info.ent.v.waterlevel * info.host_frametime, sub);
		VectorAdd(info.ent.v.velocity, sub, info.ent.v.velocity);
	}
}

void CheckWaterJump(SimulationInfo& info)
{
	vec3_t start, end, forward;
	trace_t trace;
	AngleVectors(info.ent.v.v_angle, forward, NULL, NULL);

	VectorCopy(info.ent.v.origin, start);
	start[2] += 8;
	forward[2] = 0;
	VectorNormalize(forward);
	VectorScale(forward, 24, forward);
	VectorAdd(start, forward, end);
	trace = SV_Move_Proxy(start, vec3_origin, vec3_origin, end, MOVE_NOMONSTERS, sv_player);

	if (trace.fraction < 1)
	{ // solid at waist
		start[2] += info.ent.v.maxs[2] - 8;
		VectorAdd(start, forward, end);
		VectorCopy(trace.plane.normal, info.ent.v.movedir);
		VectorScale(info.ent.v.movedir, -50, info.ent.v.movedir);

		trace = SV_Move_Proxy(start, vec3_origin, vec3_origin, end, MOVE_NOMONSTERS, sv_player);
		if (trace.fraction == 1)
		{
			info.ent.v.flags = (int)info.ent.v.flags | FL_WATERJUMP;
			info.ent.v.velocity[2] = 225;
			info.ent.v.flags -= (int)info.ent.v.flags & FL_JUMPRELEASED;
			info.ent.v.teleport_time += 2;
		}
	}
}

void PlayerJump(SimulationInfo& info)
{
	int flags = info.ent.v.flags;

	if ((flags & FL_WATERJUMP) != 0)
		return;

	if (info.ent.v.waterlevel >= 2)
	{
		if (info.ent.v.watertype == CONTENTS_WATER)
			info.ent.v.velocity[2] = 100;
		else if (info.ent.v.watertype == CONTENTS_SLIME)
			info.ent.v.velocity[2] = 80;
		else
			info.ent.v.velocity[2] = 50;

		return;
	}

	if ((flags & FL_ONGROUND) == 0 || (flags & FL_JUMPRELEASED) == 0)
		return;
	info.ent.v.flags -= flags & FL_JUMPRELEASED;
	info.ent.v.flags -= FL_ONGROUND;
	info.ent.v.velocity[2] += 270;
	info.collision = false;
}

void PlayerPreThink(SimulationInfo& info)
{
	WaterMove(info);

	if (info.ent.v.waterlevel == 2)
	{
		CheckWaterJump(info);
	}

	if (info.key_jump.state > 0)
	{
		PlayerJump(info);
	}
	else
	{
		info.ent.v.flags = (int)info.ent.v.flags | FL_JUMPRELEASED;
	}
}

void PlayerPostThink(SimulationInfo& info) {}

bool Should_Jump(const SimulationInfo& info)
{
	if (!info.tas_lgagst && !info.tas_jump)
		return false;

	auto data = GetPlayerData(const_cast<edict_t*>(&info.ent), info.vars);

	if (!data.onGround)
		return false;

	if (info.tas_jump)
		return true;

	if (info.vars.tas_strafe_version <= 1)
	{
		float lgagst = tas_strafe_lgagst_speed.value;
		return data.vel2d > lgagst && info.vars.tas_strafe && info.vars.tas_strafe_type == StrafeType::MaxAccel;
	}
	else
	{
		auto jump = info;
		auto nojump = info;
		jump.key_jump.state = 1;
		nojump.key_jump.state = 0;

		SimulateWithStrafe(jump);
		SimulateWithStrafe(jump);
		SimulateWithStrafe(nojump);
		SimulateWithStrafe(nojump);

		float jumpSpeed = VectorLength2D(jump.ent.v.velocity);
		float nojumpSpeed = VectorLength2D(nojump.ent.v.velocity);

		return jumpSpeed >= nojumpSpeed;
	}
}

static void SimulateWithStrafePlusJump(SimulationInfo& info)
{
	SetStrafe(info);
	SimulateFrame(info);

	bool jump = Should_Jump(info);
	if (!jump && info.key_jump.state > 0 && (info.tas_jump || info.tas_lgagst))
	{
		info.key_jump.state = 0;
	}
	else if (jump && info.key_jump.state == 0)
	{
		info.key_jump.state = 1;
	}
}

void SimulateFrame(SimulationInfo& info)
{
	simulating = qtrue;
	Simulate_SV_ClientThink(info);
	PlayerPreThink(info);
	PlayerPhysics(info);
	PlayerPostThink(info);
	CheckHalfPresses(info);
	simulating = qfalse;
}

void SimulateWithStrafe(SimulationInfo& info)
{
	SetStrafe(info);
	SimulateFrame(info);
}

// desc: How long the prediction algorithm should run per frame. High values will kill your fps.
cvar_t tas_predict_per_frame{"tas_predict_per_frame", "0.01"};
// desc: Display position prediction while paused in a TAS.
cvar_t tas_predict{"tas_predict", "1"};
// desc: Amount of time to predict
cvar_t tas_predict_amount{"tas_predict_amount", "3"};

void Simulate_Frame_Hook()
{
	static double last_updated = 0;
	static bool path_assigned = false;
	const std::string pathName = "predict";
	static std::vector<PathPoint> points;
	//static std::vector<SimulationInfo> infos;
	static int startFrame = 0;

	if (cls.state != ca_connected)
		return;
	auto& playback = GetPlaybackInfo();
	if (!playback.In_Edit_Mode() || !tas_predict.value)
	{
		/*
		int index = playback.current_frame - startFrame;

		if (index < points.size() && index >= 0)
		{
			vec3_t diff;
			VectorCopy(sv_player->v.origin, diff);
			VectorSubtract(diff, points[index].point, diff);
			float len = VectorLength(diff);
			//auto& player = infos[index];
			auto info = Get_Sim_Info();
			SetStrafe(info);
			if (len > 0.001)
			{
				Con_Printf("Prediction error: %f\n", len);		
			}
		}*/

		RemoveCurve(PREDICTION_ID);
		RemoveRectangles(PREDICTION_ID);
		path_assigned = false;
		return;
	}

	double currentTime = Sys_DoubleTime();
	static int frame = 0;
	static double last_sim_time = 0;
	static SimulationInfo info;
	const std::array<float, 4> color = {0, 0, 1, 0.5};

	if (last_updated < playback.last_edited)
	{
		RemoveRectangles(PREDICTION_ID);
		points.clear();
		//infos.clear();
		last_updated = currentTime;
		startFrame = playback.current_frame;
		frame = playback.current_frame;
		info = Get_Sim_Info();
		info.vars.simulated = true;
		last_sim_time = tas_predict_amount.value + sv.time;

		int frames = static_cast<int>(std::ceil((last_sim_time - info.time) * 72));
		if (frames > 0)
			points.reserve(frames);
	}

	double realTimeStart = Sys_DoubleTime();

	for (; Sys_DoubleTime() - realTimeStart < tas_predict_per_frame.value && info.time < last_sim_time; ++frame)
	{
		PathPoint vec;
		vec.color[3] = 1;
		if (info.collision)
		{
			vec.color[0] = 1;
		}
		else
		{
			vec.color[1] = 1;
		}
		VectorCopy(info.ent.v.origin, vec.point);
		points.push_back(vec);
		//infos.push_back(info);

		auto block = playback.Get_Current_Block(frame);
		if (block && block->frame == frame)
		{
			ApplyFrameblock(info, block);
			Rect rect = Rect::Get_Rect(color, info.ent.v.origin, 3, 3, PREDICTION_ID);
			AddRectangle(rect);
		}

		SimulateWithStrafePlusJump(info);
	}

	if (!path_assigned)
	{
		AddCurve(&points, PREDICTION_ID);
		path_assigned = true;
	}
}
