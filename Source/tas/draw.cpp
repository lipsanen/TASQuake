#include "cpp_quakedef.hpp"
#include "draw.hpp"
#include <map>
#include "strafing.hpp"

static std::map<std::string, std::vector<PathPoint>*> lines;
static byte	color_green[4] = { 0, 255, 0, 0 };

void AddCurve(std::vector<PathPoint>* points, const std::string & name)
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
		for (int i=0; i < pair.second->size() - 1; ++i)
		{
			auto& vec1 = pair.second->at(i);
			auto& vec2 = pair.second->at(i+1);

			glColor3fv(vec1.color.Get_Array());
			glVertex3fv(vec1.point.Get_Array());
			glVertex3fv(vec2.point.Get_Array());
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
	}
}
