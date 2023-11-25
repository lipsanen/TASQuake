#pragma once

#include "draw.hpp"
#include "libtasquake/draw.hpp"
#include <vector>

extern bool path_assigned;
extern bool grenade_assigned;

extern cvar_t tas_predict_endoffset;
extern cvar_t tas_predict_maxlength;
extern cvar_t tas_predict_entity;

bool Calculate_Prediction_Line(bool canPredict);
void Calculate_Grenade_Line(bool canPredict);

bool Prediction_HasLine();
std::vector<TASQuake::PathPoint>* Prediction_GetPoints();
std::vector<TASQuake::Rect>* Prediction_GetRects();
void Get_Line_Endpoints(uint32_t& start, uint32_t& end);
namespace TASQuake {
    void Get_Prediction_Frames(int32_t& start_frame, int32_t& end_frame);
}

