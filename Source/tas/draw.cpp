#include <algorithm>
#include <map>

#include "cpp_quakedef.hpp"

#include "draw.hpp"
#include "real_prediction.hpp"
#include "prediction.hpp"

#include "strafing.hpp"
#include "libtasquake/draw.hpp"
#include "libtasquake/utils.hpp"

const int PREDICTION_ID = 1;
const int REWARD_ID = 2;
const int GRENADE_ID = 3;
const int OPTIMIZER_ID = 4;

using namespace TASQuake;

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

static void DrawLine(const std::vector<PathPoint>* line) {
	glBegin(GL_LINES);
	int elements = line->size();
	for (int i = 0; i < elements - 1; ++i)
	{
		auto& vec1 = line->at(i);
		auto& vec2 = line->at(i + 1);

		glColor4fv(vec1.color.data());
		glVertex3fv(vec1.point);
		glVertex3fv(vec2.point);
	}
	glEnd();
}

static void DrawRects(const std::vector<Rect>* rects) {
	for (auto it=rects->begin(); it != rects->end(); ++it)
	{
		Draw_Rectangle(*it);
	}
}

void Draw_Elements()
{
	if (cl.intermission)
		return;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	if(GamePrediction_HasLine()) {
		DrawLine(GamePrediction_GetPoints());
		DrawRects(GamePrediction_GetRects());
	} else if(Prediction_HasLine()) {
		DrawLine(Prediction_GetPoints());
		DrawRects(Prediction_GetRects());
	}

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
