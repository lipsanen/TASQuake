#pragma once
#include <array>
#include <vector>

#include "libtasquake/draw.hpp"
#include "cpp_quakedef.hpp"

extern const int PREDICTION_ID;
extern const int REWARD_ID;
extern const int GRENADE_ID;
extern const int OPTIMIZER_ID;

void AddCurve(std::vector<TASQuake::PathPoint>* points, int id);
void AddRectangle(TASQuake::Rect rect);
void RemoveRectangles(int id);
void RemoveCurve(int id);
void Draw_Elements();
