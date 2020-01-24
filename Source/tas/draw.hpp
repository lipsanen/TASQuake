#define PREDICTION_ID 1

#include "cpp_quakedef.hpp"

struct PathPoint
{
	PathPoint();
	std::array<float, 3> point;
	std::array<float, 4> color;
};

struct Rect
{
	Rect();
	std::array<float, 4> color;
	std::array<float, 3> center;
	float width;
	float height;
	int id;
};

void AddCurve(std::vector<PathPoint>* points, int id);
void AddRectangle(Rect rect);
void RemoveRectangles(int id);
void RemoveCurve(int id);
void Draw_Elements();
