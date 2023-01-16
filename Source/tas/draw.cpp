#include <algorithm>
#include <map>

#include "cpp_quakedef.hpp"

#include "draw.hpp"
#include "ipc_prediction.hpp"
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

	Q_glBegin(GL_QUADS);
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

	Q_glEnd();
}

static void DrawLine(const std::vector<PathPoint>* line, uint32_t* pstart=nullptr, uint32_t* pend=nullptr) {
	Q_glBegin(GL_LINES);

	uint32_t start, end;
	if(pstart) {
		start = std::min(*pstart, line->size() - 2);
	} else {
		start = 0;
	}

	if(pend) {
		end = std::min(*pend, line->size()-1);
	} else {
		end = line->size()-1;
	}

	for (uint32_t i=start; i < end; ++i)
	{
		auto& vec1 = line->at(i);
		auto& vec2 = line->at(i + 1);

		glColor4fv(vec1.color.data());
		Q_glVertex3fv(vec1.point);
		Q_glVertex3fv(vec2.point);
	}

	Q_glEnd();
}

static void DrawRects(const std::vector<Rect>* rects, uint32_t* pstart=nullptr, uint32_t* pend=nullptr) {
	uint32_t start, end;
	if(pstart) {
		start = std::min(*pstart, rects->size() - 1);
	} else {
		start = 0;
	}

	if(pend) {
		end = std::min(*pend, rects->size());
	} else {
		end = rects->size();
	}

	auto startIt = rects->begin() + start;
	auto endIt = rects->begin() + end;

	for (auto it=startIt; it != endIt; ++it)
	{
		Draw_Rectangle(*it);
	}
}

void Draw_Elements()
{
	if (cl.intermission)
		return;

	glDisable(GL_TEXTURE_2D);
	Q_glEnable(GL_BLEND);

	if(IPC_Prediction_HasLine()) {
		uint32_t start, end;
		Get_Line_Endpoints(start, end);
		DrawLine(IPC_Prediction_GetPoints(), &start, &end);
		DrawRects(IPC_Prediction_GetRects(), &start, &end);
	}
	else if(GamePrediction_HasLine()) {
		uint32_t start, end;
		Get_Line_Endpoints(start, end);
		DrawLine(GamePrediction_GetPoints(), &start, &end);
		DrawRects(GamePrediction_GetRects(), &start, &end);
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
		Q_glBegin(GL_LINES);
		int elements = pair.second->size();
		for (int i = 0; i < elements - 1; ++i)
		{
			auto& vec1 = pair.second->at(i);
			auto& vec2 = pair.second->at(i + 1);

			glColor4fv(vec1.color.data());
			Q_glVertex3fv(vec1.point);
			Q_glVertex3fv(vec2.point);
		}
		Q_glEnd();
	}
	Q_glEnable(GL_TEXTURE_2D);
	Q_glDisable(GL_BLEND);
}
