#include <array>
#include <vector>

#include "cpp_quakedef.hpp"

const int PREDICTION_ID = 1;
const int REWARD_ID = 2;
const int GRENADE_ID = 3;

struct PathPoint
{
	PathPoint();
	vec3_t point;
	std::array<float, 4> color;
};

struct Rect
{
	Rect();
	std::array<float, 4> color;
	vec3_t mins;
	vec3_t maxs;
	Rect(const std::array<float, 4>& _color, vec3_t _mins, vec3_t _maxs, int id);
	static Rect Get_Rect(const std::array<float, 4>& _color, vec3_t center, float width, float height, int id);
	static Rect Get_Rect(const std::array<float, 4>& _color, vec3_t center, vec3_t angles, float length, float width, float height, int id);
	int id;
};

void AddCurve(std::vector<PathPoint>* points, int id);
void AddRectangle(Rect rect);
void RemoveRectangles(int id);
void RemoveCurve(int id);
void Draw_Elements();
