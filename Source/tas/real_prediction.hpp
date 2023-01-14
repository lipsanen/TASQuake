#pragma once

#include "draw.hpp"
#include "libtasquake/draw.hpp"
#include <vector>

extern cvar_t tas_predict_real;

bool GamePrediction_HasLine();
void GamePrediction_Frame_Hook();
std::vector<TASQuake::PathPoint>* GamePrediction_GetPoints();
std::vector<TASQuake::Rect>* GamePrediction_GetRects();
