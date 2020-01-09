#include <map>
#include <algorithm>

#include "cpp_quakedef.hpp"
#include "draw.hpp"
#include "strafing.hpp"

static std::map<int, std::vector<PathPoint>*> lines;
static std::vector<Rect> rects;
static byte	color_green[4] = { 0, 255, 0, 0 };

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
		if(rects[i].id == id)
			rects.erase(rects.begin() + i);
	}
}

void RemoveCurve(int id)
{
	if(lines.find(id) != lines.end())
		lines.erase(id);
}

#define DrawSide(vec)

void Draw_Elements()
{
	glDisable(GL_TEXTURE_2D);
	for (auto& rect : rects)
	{
		Vector3f corner1 = rect.center;
		Vector3f corner2 = rect.center;
		corner1[0] -= rect.width / 2;
		corner1[1] -= rect.width / 2;
		corner1[2] -= rect.height / 2;
		corner2[0] += rect.width / 2;
		corner2[1] += rect.width / 2;
		corner2[2] += rect.height / 2;


		glBegin(GL_QUADS);
		glColor3fv(rect.color.Get_Array());
		glVertex3f(corner1.x, corner1.y, corner1.z);
		glVertex3f(corner1.x, corner2.y, corner1.z);
		glVertex3f(corner2.x, corner2.y, corner1.z);
		glVertex3f(corner2.x, corner1.y, corner1.z);

		glVertex3f(corner1.x, corner1.y, corner1.z);
		glVertex3f(corner1.x, corner1.y, corner2.z);
		glVertex3f(corner1.x, corner2.y, corner2.z);
		glVertex3f(corner1.x, corner2.y, corner1.z);

		glVertex3f(corner1.x, corner1.y, corner1.z);
		glVertex3f(corner2.x, corner1.y, corner1.z);
		glVertex3f(corner2.x, corner1.y, corner2.z);
		glVertex3f(corner1.x, corner1.y, corner2.z);

		glVertex3f(corner2.x, corner2.y, corner2.z);
		glVertex3f(corner1.x, corner2.y, corner2.z);
		glVertex3f(corner1.x, corner1.y, corner2.z);
		glVertex3f(corner2.x, corner1.y, corner2.z);

		glVertex3f(corner2.x, corner2.y, corner2.z);
		glVertex3f(corner2.x, corner1.y, corner2.z);
		glVertex3f(corner2.x, corner1.y, corner1.z);
		glVertex3f(corner2.x, corner2.y, corner1.z);

		glVertex3f(corner2.x, corner2.y, corner2.z);
		glVertex3f(corner2.x, corner2.y, corner1.z);
		glVertex3f(corner1.x, corner2.y, corner1.z);
		glVertex3f(corner1.x, corner2.y, corner2.z);

		glEnd();
	}

	for (auto pair : lines)
	{	
		glBegin(GL_LINES);
		for (int i=0; i < pair.second->size() - 1; ++i)
		{
			auto& vec1 = pair.second->at(i);
			auto& vec2 = pair.second->at(i+1);

			glColor3fv(vec1.color.Get_Array());
			glVertex3fv(vec1.point.Get_Array());
			glVertex3fv(vec2.point.Get_Array());
		}
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
}
