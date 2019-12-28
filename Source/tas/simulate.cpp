#include "simulate.hpp"

trace_t SV_Move_Proxy(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t *passedict)
{
	return SV_Move(start, mins, maxs, end, type, sv_player);
}

void SV_Impact(edict_t *e1, edict_t *e2)
{
	// Hopefully this does nothing important
}

#define	STOP_EPSILON	0.1

int ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff, change;
	int	i, blocked;

	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}

trace_t SV_PushEntity(edict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	end;

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

int SV_TryUnstick(edict_t *ent, vec3_t oldvel)
{
	int	i, clip;
	vec3_t	oldorg, dir;
	trace_t	steptrace;

	VectorCopy(ent->v.origin, oldorg);
	VectorCopy(vec3_origin, dir);

	for (i = 0; i < 8; i++)
	{
		// try pushing a little in an axial direction
		switch (i)
		{
		case 0:	dir[0] = 2; dir[1] = 0; break;
		case 1:	dir[0] = 0; dir[1] = 2; break;
		case 2:	dir[0] = -2; dir[1] = 0; break;
		case 3:	dir[0] = 0; dir[1] = -2; break;
		case 4:	dir[0] = 2; dir[1] = 2; break;
		case 5:	dir[0] = -2; dir[1] = 2; break;
		case 6:	dir[0] = 2; dir[1] = -2; break;
		case 7:	dir[0] = -2; dir[1] = -2; break;
		}

		SV_PushEntity(ent, dir);

		// retry the original move
		ent->v.velocity[0] = oldvel[0];
		ent->v.velocity[1] = oldvel[1];
		ent->v.velocity[2] = 0;
		clip = SV_FlyMove(ent, 0.1, &steptrace);

		if (fabs(oldorg[1] - ent->v.origin[1]) > 4 || fabs(oldorg[0] - ent->v.origin[0]) > 4)
		{
			//Con_DPrintf ("unstuck!\n");
			return clip;
		}

		// go back to the original pos and try again
		VectorCopy(oldorg, ent->v.origin);
	}

	VectorCopy(vec3_origin, ent->v.velocity);
	return 7;		// still not moving
}

void SV_WallFriction(edict_t *ent, trace_t *trace)
{
	float	d, i;
	vec3_t	forward, right, up, into, side;

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

#define	MAX_CLIP_PLANES	5
int SV_FlyMove(edict_t *ent, float time, trace_t *steptrace)
{
	int			i, j, bumpcount, numbumps, numplanes, blocked;
	float		d, time_left;
	vec3_t		dir, planes[MAX_CLIP_PLANES], primal_velocity, original_velocity, new_velocity, end;
	trace_t		trace;

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
		{	// entity is trapped in another solid
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, ent->v.origin);
			VectorCopy(ent->v.velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

		if (!trace.ent)
			Sys_Error("SV_FlyMove: !trace.ent");

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (trace.ent->v.solid == SOLID_BSP)
			{
				ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
				ent->v.groundentity = EDICT_TO_PROG(trace.ent);
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

		// run the impact function
		SV_Impact(ent, trace.ent);
		if (ent->free)
			break;		// removed by the impact function

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
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
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{	// go along this plane
			VectorCopy(new_velocity, ent->v.velocity);
		}
		else
		{	// go along the crease
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

#define	STEPSIZE	18
void SV_WalkMove(edict_t *ent, double hfr, bool nostep=false)
{
	int		clip, oldonground;
	vec3_t		upmove, downmove, oldorg, oldvel, nosteporg, nostepvel;
	trace_t		steptrace, downtrace;

	// do a regular slide move unless it looks like you ran into a step
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	VectorCopy(ent->v.origin, oldorg);
	VectorCopy(ent->v.velocity, oldvel);

	clip = SV_FlyMove(ent, hfr, &steptrace);

	if (!(clip & 2))
		return;		// move didn't block on a step

	if (!oldonground && ent->v.waterlevel == 0)
		return;		// don't stair up while jumping

	if (ent->v.movetype != MOVETYPE_WALK)
		return;		// gibbed by a trigger

	if (nostep)
		return;

	if ((int)ent->v.flags & FL_WATERJUMP)
		return;

	VectorCopy(ent->v.origin, nosteporg);
	VectorCopy(ent->v.velocity, nostepvel);

	// try moving up and forward to go up a step
	VectorCopy(oldorg, ent->v.origin);	// back to start pos

	VectorCopy(vec3_origin, upmove);
	VectorCopy(vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2] * hfr;

	// move up
	SV_PushEntity(ent, upmove);	// FIXME: don't link?

// move forward
	ent->v.velocity[0] = oldvel[0];
	ent->v.velocity[1] = oldvel[1];
	ent->v.velocity[2] = 0;
	clip = SV_FlyMove(ent, hfr, &steptrace);

	// check for stuckness, possibly due to the limited precision of floats
	// in the clipping hulls
	if (clip)
	{
		if (fabs(oldorg[1] - ent->v.origin[1]) < 0.03125 && fabs(oldorg[0] - ent->v.origin[0]) < 0.03125)
		{	// stepping up didn't make any progress
			clip = SV_TryUnstick(ent, oldvel);
		}
	}

	// extra friction based on view angle
	if (clip & 2)
		SV_WallFriction(ent, &steptrace);

	// move down
	downtrace = SV_PushEntity(ent, downmove);	// FIXME: don't link?

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

bool SV_CheckWater(edict_t *ent)
{
	int	cont;
	vec3_t	point;

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
		point[2] = ent->v.origin[2] + (ent->v.mins[2] + ent->v.maxs[2])*0.5;
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

void SV_AddGravity(edict_t *ent, double hfr)
{
	float	ent_gravity;

	eval_t	*val;

	val = GETEDICTFIELDVALUE(ent, eval_gravity);
	if (val && val->_float)
		ent_gravity = val->_float;
	else
		ent_gravity = 1.0;
	ent->v.velocity[2] -= ent_gravity * sv_gravity.value * hfr;
}


void SV_CheckStuck(edict_t *ent)
{
	int		i, j;
	int		z;
	vec3_t	org;

	if (!SV_TestEntityPosition(ent))
	{
		VectorCopy(ent->v.origin, ent->v.oldorigin);
		return;
	}

	VectorCopy(ent->v.origin, org);
	VectorCopy(ent->v.oldorigin, ent->v.origin);
	if (!SV_TestEntityPosition(ent))
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
				if (!SV_TestEntityPosition(ent))
				{
					return;
				}
			}

	VectorCopy(org, ent->v.origin);
}


void Simulate_SV_WaterJump(edict_t* ent)
{
	if (sv.time > ent->v.teleport_time || !ent->v.waterlevel)
	{
		ent->v.flags = (int)ent->v.flags & ~FL_WATERJUMP;
		ent->v.teleport_time = 0;
	}
	ent->v.velocity[0] = ent->v.movedir[0];
	ent->v.velocity[1] = ent->v.movedir[1];
}

void SV_WaterMove(edict_t* ent, const usercmd_t& cmd, double hfr)
{
	int	i;
	vec3_t	wishvel, forward, right, up;
	float	speed, newspeed, wishspeed, addspeed, accelspeed;

	// user intentions
	AngleVectors(ent->v.v_angle, forward, right, up);

	for (i = 0; i < 3; i++)
		wishvel[i] = forward[i] * cmd.forwardmove + right[i] * cmd.sidemove;

	if (!cmd.forwardmove && !cmd.sidemove && !cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += cmd.upmove;

	wishspeed = VectorLength(wishvel);
	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}
	wishspeed *= 0.7;

	// water friction
	speed = VectorLength(ent->v.velocity);
	if (speed)
	{
		newspeed = speed - hfr * speed * sv_friction.value;
		if (newspeed < 0)
			newspeed = 0;
		VectorScale(ent->v.velocity, newspeed / speed, ent->v.velocity);
	}
	else
	{
		newspeed = 0;
	}

	// water acceleration
	if (!wishspeed)
		return;

	addspeed = wishspeed - newspeed;
	if (addspeed <= 0)
		return;

	VectorNormalize(wishvel);
	accelspeed = sv_accelerate.value * wishspeed * hfr;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		ent->v.velocity[i] += accelspeed * wishvel[i];
}

void SV_UserFriction(edict_t* ent, double hfr)
{
	float	*vel, speed, newspeed, control, friction;
	vec3_t	start, stop;
	trace_t	trace;

	vel = ent->v.velocity;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	if (!speed)
		return;

	// if the leading edge is over a dropoff, increase friction
	start[0] = stop[0] = ent->v.origin[0] + vel[0] / speed * 16;
	start[1] = stop[1] = ent->v.origin[1] + vel[1] / speed * 16;
	start[2] = ent->v.origin[2] + ent->v.mins[2];
	stop[2] = start[2] - 34;

	trace = SV_Move_Proxy(start, vec3_origin, vec3_origin, stop, true, ent);

	if (trace.fraction == 1.0)
		friction = sv_friction.value * sv_edgefriction.value;
	else
		friction = sv_friction.value;

	// apply friction	
	control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
	newspeed = speed - hfr * control * friction;

	if (newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

void SV_Accelerate(edict_t* ent, vec3_t wishdir, double wishspeed, double hfr)
{
	int	i;
	float	addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(ent->v.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = sv_accelerate.value*hfr*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		ent->v.velocity[i] += accelspeed * wishdir[i];
}


void SV_AirAccelerate(edict_t* ent, vec3_t wishveloc, double wishspeed, double hfr)
{
	int	i;
	float	addspeed, wishspd, accelspeed, currentspeed;

	wishspd = VectorNormalize(wishveloc);
	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct(ent->v.velocity, wishveloc);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	//	accelspeed = sv_accelerate.value * host_frametime;
	accelspeed = sv_accelerate.value*wishspeed * hfr;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		ent->v.velocity[i] += accelspeed * wishveloc[i];
}



void SV_AirMove(edict_t* ent, const usercmd_t& cmd, double hfr, double time)
{
	int	i;
	vec3_t	wishvel, wishdir, forward, right, up;
	float	fmove, smove;
	float wishspeed;

	AngleVectors(ent->v.angles, forward, right, up);

	fmove = cmd.forwardmove;
	smove = cmd.sidemove;

	// hack to not let you back into teleporter
	if (time < ent->v.teleport_time && fmove < 0)
		fmove = 0;

	for (i = 0; i < 3; i++)
		wishvel[i] = forward[i] * fmove + right[i] * smove;

	if ((int)ent->v.movetype != MOVETYPE_WALK)
		wishvel[2] = cmd.upmove;
	else
		wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}

	if (ent->v.movetype == MOVETYPE_NOCLIP)
	{	// noclip
		VectorCopy(wishvel, ent->v.velocity);
	}
	else if ((int)ent->v.flags & FL_ONGROUND)
	{
		SV_UserFriction(ent, hfr);
		SV_Accelerate(ent, wishdir, wishspeed, hfr);
	}
	else
	{	// not on ground, so little effect on velocity
		SV_AirAccelerate(ent, wishvel, wishspeed, hfr);
	}
}

void PlayerPhysics(SimulationInfo& info)
{
	if (!SV_CheckWater(&info.ent) && !((int)info.ent.v.flags & FL_WATERJUMP))
		SV_AddGravity(&info.ent, info.hfr);
	SV_CheckStuck(&info.ent);
	SV_WalkMove(&info.ent, info.hfr);
}

void Simulate_SV_ClientThink(SimulationInfo& info)
{
	vec3_t v_angle;
	vec3_t origin;
	bool onground;
	usercmd_t cmd;
	float angles[3];

	info.ent.v.v_angle[ROLL] = 0;
	info.ent.v.v_angle[PITCH] = info.pitch;
	info.ent.v.v_angle[YAW] = info.yaw;
	cmd.forwardmove = info.fmove;
	cmd.sidemove = info.smove;
	cmd.upmove = info.upmove;

	if ((int)info.ent.v.flags & FL_WATERJUMP)
	{
		Simulate_SV_WaterJump(&info.ent);
		return;
	}
	// walk
	if ((info.ent.v.waterlevel >= 2) && (info.ent.v.movetype != MOVETYPE_NOCLIP))
	{
		SV_WaterMove(&info.ent, cmd, info.hfr);
		return;
	}

	SV_AirMove(&info.ent, cmd, info.hfr, info.time);
}

SimulationInfo GetCurrentStatus()
{
	SimulationInfo info;
	info.hfr = host_frametime;
	info.ent = *sv_player;
	info.time = sv.time;
	return info;
}

void SetStrafe(SimulationInfo& info)
{

}

void SimulateFrame(SimulationInfo & info)
{
	info.time += info.hfr;
	Simulate_SV_ClientThink(info);
	PlayerPhysics(info);
}
