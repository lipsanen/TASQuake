#include "libtasquake/draw.hpp"
#include "libtasquake/utils.hpp"
#include <cmath>
#include <cstring>

using namespace TASQuake;

#define VectorCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])

PathPoint::PathPoint()
{
	for (int i = 0; i < 3; ++i)
		point[i] = 0;
	for (int i = 0; i < 4; ++i)
		color[i] = 0;
}

Rect::Rect()
{
	for (int i = 0; i < 3; ++i)
		mins[i] = 0;
	for (int i = 0; i < 3; ++i)
		maxs[i] = 0;
	for (int i = 0; i < 4; ++i)
		color[i] = 0;
}

Rect::Rect(const std::array<float, 4>& _color, float _mins[3], float _maxs[3], int _id)
{
	VectorCopy(_mins, mins);
	VectorCopy(_maxs, maxs);
	color = _color;
	id = _id;
}

Rect Rect::Get_Rect(const std::array<float, 4>& _color, float center[3], float width, float height, int id)
{
	Rect r;
	r.color = _color;
	r.id = id;
	VectorCopy(center, r.mins);
	r.mins[0] -= width;
	r.mins[1] -= width;
	r.mins[2] -= height;

	VectorCopy(center, r.maxs);
	r.maxs[0] += width;
	r.maxs[1] += width;
	r.maxs[2] += height;

	return r;
}

typedef float vec3_t[3];
static const int PITCH = 0;
static const int YAW = 1;
static const int ROLL = 2;

static void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float	angle, sr, sp, sy, cr, cp, cy, temp;

	if (angles[YAW])
	{
		angle = M_DEG2RAD * angles[YAW];
		sy = sin(angle);
		cy = cos(angle);
	}
	else
	{
		sy = 0;
		cy = 1;
	}

	if (angles[PITCH])
	{
		angle = M_DEG2RAD * angles[PITCH];
		sp = sin(angle);
		cp = cos(angle);
	}
	else
	{
		sp = 0;
		cp = 1;
	}

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if (right || up)
	{
		if (angles[ROLL])
		{
			angle = M_DEG2RAD * angles[ROLL];
			sr = sin(angle);
			cr = cos(angle);

			if (right)
			{
				temp = sr * sp;
				right[0] = -1 * temp * cy + cr * sy;
				right[1] = -1 * temp * sy - cr * cy;
				right[2] = -1 * sr * cp;
			}

			if (up)
			{
				temp = cr * sp;
				up[0] = (temp * cy + sr * sy);
				up[1] = (temp * sy - sr * cy);
				up[2] = cr * cp;
			}
		}
		else
		{
			if (right)
			{
				right[0] = sy;
				right[1] = -cy;
				right[2] = 0;
			}

			if (up)
			{
				up[0] = sp * cy ;
				up[1] = sp * sy;
				up[2] = cp;
			}
		}
	}
}

Rect Rect::Get_Rect(const std::array<float, 4>& _color, float center[3], float angles[3], float length, float width, float height, int id)
{
	float fwd[3], right[3], up[3];
	float corner1[3], corner2[3];
	AngleVectors(angles, fwd, right, up);
	Rect r;
	r.color = _color;
	r.id = id;

	VectorCopy(center, corner1);
	VectorCopy(center, corner2);
	VectorScaledAdd(corner1, fwd, length / 2, corner1);
	VectorScaledAdd(corner2, fwd, -length / 2, corner2);
	VectorScaledAdd(corner1, right, width / 2, corner1);
	VectorScaledAdd(corner2, right, -width / 2, corner2);
	VectorScaledAdd(corner1, up, height / 2, corner1);
	VectorScaledAdd(corner2, up, -height / 2, corner2);

	r.mins[0] = fmin(corner1[0], corner2[0]);
	r.mins[1] = fmin(corner1[1], corner2[1]);
	r.mins[2] = fmin(corner1[2], corner2[2]);

	r.maxs[0] = fmax(corner1[0], corner2[0]);
	r.maxs[1] = fmax(corner1[1], corner2[1]);
	r.maxs[2] = fmax(corner1[2], corner2[2]);

	return r;
}
