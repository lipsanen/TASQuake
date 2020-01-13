#include "cpp_quakedef.hpp"
#include <vector>

#define PREDICTION_ID 1

struct PathPoint
{
	PathPoint();
	Vector3f point;
	Vector4f color;
};

struct Rect
{
	Rect();
	Vector4f color;
	Vector3f center;
	float width;
	float height;
	int id;
};

void AddCurve(std::vector<PathPoint>* points, int id);
void AddRectangle(Rect rect);
void RemoveRectangles(int id);
void RemoveCurve(int id);
void Draw_Elements();