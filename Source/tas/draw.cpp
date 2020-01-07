#include "cpp_quakedef.hpp"
#include "draw.hpp"
#include <map>
#include "strafing.hpp"

static std::map<std::string, std::vector<Vector3f>*> lines;
static byte	color_green[4] = { 0, 255, 0, 0 };

void AddCurve(std::vector<Vector3f>* points, const std::string & name)
{
	lines[name] = points;
}

void RemoveCurve(const std::string & name)
{
	if(lines.find(name) != lines.end())
		lines.erase(name);
}

void Draw_Lines()
{
	for (auto pair : lines)
	{
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		for (int i=0; i < pair.second->size(); ++i)
		{
			auto& point = pair.second->at(i);
			glColor3f(0, 1.f, 0);
			glVertex3fv(point.Get_Array());
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
	}
}
