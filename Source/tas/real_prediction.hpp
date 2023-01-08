#pragma once

#include "draw.hpp"
#include <vector>

extern cvar_t tas_predict_real;

bool GamePrediction_HasLine();
void GamePrediction_Frame_Hook();
std::vector<PathPoint>* GamePrediction_GetPoints();
std::vector<Rect>* GamePrediction_GetRects();
