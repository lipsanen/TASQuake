#pragma once

#include "draw.hpp"
#include <vector>

extern bool path_assigned;
extern bool grenade_assigned;

bool Calculate_Prediction_Line(bool canPredict);
void Calculate_Grenade_Line(bool canPredict);

bool Prediction_HasLine();
std::vector<PathPoint>* Prediction_GetPoints();
std::vector<Rect>* Prediction_GetRects();
