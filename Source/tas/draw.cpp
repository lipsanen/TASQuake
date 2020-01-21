#include <algorithm>
#include <map>

#include "cpp_quakedef.hpp"

#include "draw.hpp"

#include "strafing.hpp"

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

#define DrawSide(vec)

void Draw_Elements()
{
	if (cl.intermission)
		return;

	glDisable(GL_TEXTURE_2D);
	for (auto& rect : rects)
	{
		auto corner1 = rect.center;
		auto corner2 = rect.center;
		corner1[0] -= rect.width / 2;
		corner1[1] -= rect.width / 2;
		corner1[2] -= rect.height / 2;
		corner2[0] += rect.width / 2;
		corner2[1] += rect.width / 2;
		corner2[2] += rect.height / 2;

		glBegin(GL_QUADS);
		glColor4fv(rect.color._Elems);
		glVertex3f(corner1[0], corner1[1], corner1[2]);
		glVertex3f(corner1[0], corner2[1], corner1[2]);
		glVertex3f(corner2[0], corner2[1], corner1[2]);
		glVertex3f(corner2[0], corner1[1], corner1[2]);

		glVertex3f(corner1[0], corner1[1], corner1[2]);
		glVertex3f(corner1[0], corner1[1], corner2[2]);
		glVertex3f(corner1[0], corner2[1], corner2[2]);
		glVertex3f(corner1[0], corner2[1], corner1[2]);

		glVertex3f(corner1[0], corner1[1], corner1[2]);
		glVertex3f(corner2[0], corner1[1], corner1[2]);
		glVertex3f(corner2[0], corner1[1], corner2[2]);
		glVertex3f(corner1[0], corner1[1], corner2[2]);

		glVertex3f(corner2[0], corner2[1], corner2[2]);
		glVertex3f(corner1[0], corner2[1], corner2[2]);
		glVertex3f(corner1[0], corner1[1], corner2[2]);
		glVertex3f(corner2[0], corner1[1], corner2[2]);

		glVertex3f(corner2[0], corner2[1], corner2[2]);
		glVertex3f(corner2[0], corner1[1], corner2[2]);
		glVertex3f(corner2[0], corner1[1], corner1[2]);
		glVertex3f(corner2[0], corner2[1], corner1[2]);

		glVertex3f(corner2[0], corner2[1], corner2[2]);
		glVertex3f(corner2[0], corner2[1], corner1[2]);
		glVertex3f(corner1[0], corner2[1], corner1[2]);
		glVertex3f(corner1[0], corner2[1], corner2[2]);

		glEnd();
	}

	for (auto pair : lines)
	{
		glBegin(GL_LINES);
		int elements = pair.second->size();
		for (int i = 0; i < elements - 1; ++i)
		{
			auto& vec1 = pair.second->at(i);
			auto& vec2 = pair.second->at(i + 1);

			glColor4fv(vec1.color._Elems);
			glVertex3fv(vec1.point._Elems);
			glVertex3fv(vec2.point._Elems);
		}
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
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
		center[i] = 0;
	for (int i = 0; i < 4; ++i)
		color[i] = 0;
}
