#pragma once
#include <array>
#include <vector>

#include "cpp_quakedef.hpp"

#include "libtasquake/draw.hpp"

extern const int PREDICTION_ID;
extern const int REWARD_ID;
extern const int GRENADE_ID;
extern const int OPTIMIZER_ID;

void AddCurve(std::vector<TASQuake::PathPoint>* points, int id);
void AddRectangle(TASQuake::Rect rect);
void RemoveRectangles(int id);
void RemoveCurve(int id);
void Draw_Elements();

// desc: Draw hitbox of enemies
extern cvar_t tas_draw_entbbox;
