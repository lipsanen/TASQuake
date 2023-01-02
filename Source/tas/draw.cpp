#include <algorithm>
#include <map>

#include "cpp_quakedef.hpp"

#include "draw.hpp"

#include "strafing.hpp"
#include "libtasquake/utils.hpp"

static std::map<int, std::vector<PathPoint>*> lines;
static std::vector<Rect> rects;
static byte color_green[4] = {0, 255, 0, 0};

void AddCurve(std::vector<PathPoint>* points, int id)
{
	lines[id] = points;
}

void AddRectangle(Rect rect)
{
	rects.push_back(rect);
}

void RemoveRectangles(int id)
{
	for (int i = rects.size() - 1; i >= 0; --i)
	{
		if (rects[i].id == id)
			rects.erase(rects.begin() + i);
	}
}

void RemoveCurve(int id)
{
	if (lines.find(id) != lines.end())
		lines.erase(id);
}

static void Draw_Rectangle(const Rect& rect)
{

	glBegin(GL_QUADS);
	glColor4fv(rect.color.data());
	glVertex3f(rect.mins[0], rect.mins[1], rect.mins[2]);
	glVertex3f(rect.mins[0], rect.maxs[1], rect.mins[2]);
	glVertex3f(rect.maxs[0], rect.maxs[1], rect.mins[2]);
	glVertex3f(rect.maxs[0], rect.mins[1], rect.mins[2]);

	glVertex3f(rect.mins[0], rect.mins[1], rect.mins[2]);
	glVertex3f(rect.mins[0], rect.mins[1], rect.maxs[2]);
	glVertex3f(rect.mins[0], rect.maxs[1], rect.maxs[2]);
	glVertex3f(rect.mins[0], rect.maxs[1], rect.mins[2]);

	glVertex3f(rect.mins[0], rect.mins[1], rect.mins[2]);
	glVertex3f(rect.maxs[0], rect.mins[1], rect.mins[2]);
	glVertex3f(rect.maxs[0], rect.mins[1], rect.maxs[2]);
	glVertex3f(rect.mins[0], rect.mins[1], rect.maxs[2]);

	glVertex3f(rect.maxs[0], rect.maxs[1], rect.maxs[2]);
	glVertex3f(rect.mins[0], rect.maxs[1], rect.maxs[2]);
	glVertex3f(rect.mins[0], rect.mins[1], rect.maxs[2]);
	glVertex3f(rect.maxs[0], rect.mins[1], rect.maxs[2]);

	glVertex3f(rect.maxs[0], rect.maxs[1], rect.maxs[2]);
	glVertex3f(rect.maxs[0], rect.mins[1], rect.maxs[2]);
	glVertex3f(rect.maxs[0], rect.mins[1], rect.mins[2]);
	glVertex3f(rect.maxs[0], rect.maxs[1], rect.mins[2]);

	glVertex3f(rect.maxs[0], rect.maxs[1], rect.maxs[2]);
	glVertex3f(rect.maxs[0], rect.maxs[1], rect.mins[2]);
	glVertex3f(rect.mins[0], rect.maxs[1], rect.mins[2]);
	glVertex3f(rect.mins[0], rect.maxs[1], rect.maxs[2]);

	glEnd();
}

void Draw_Elements()
{
	if (cl.intermission)
		return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	for (auto& rect : rects)
	{
		Draw_Rectangle(rect);
	}

	for (auto pair : lines)
	{
		glBegin(GL_LINES);
		int elements = pair.second->size();
		for (int i = 0; i < elements - 1; ++i)
		{
			auto& vec1 = pair.second->at(i);
			auto& vec2 = pair.second->at(i + 1);

			glColor4fv(vec1.color.data());
			glVertex3fv(vec1.point);
			glVertex3fv(vec2.point);
		}
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

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

Rect::Rect(const std::array<float, 4>& _color, vec3_t _mins, vec3_t _maxs, int _id)
{
	VectorCopy(_mins, mins);
	VectorCopy(_maxs, maxs);
	color = _color;
	id = _id;
}

Rect Rect::Get_Rect(const std::array<float, 4>& _color, vec3_t center, float width, float height, int id)
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

Rect Rect::Get_Rect(const std::array<float, 4>& _color, vec3_t center, vec3_t angles, float length, float width, float height, int id)
{
	vec3_t fwd, right, up;
	vec3_t corner1, corner2;
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
