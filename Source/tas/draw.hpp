#include "cpp_quakedef.hpp"
#include <vector>

struct PathPoint
{
	Vector3f point;
	Vector3f color;
};

void AddCurve(std::vector<PathPoint>* points, const std::string& name);
void RemoveCurve(const std::string& name);
void Draw_Lines();