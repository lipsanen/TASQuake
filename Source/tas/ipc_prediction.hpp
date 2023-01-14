#pragma once

#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/draw.hpp"

void IPC_Prediction_Frame_Hook();
void IPC_Prediction_Read_Response(ipc::Message& msg);
bool IPC_Prediction_HasLine();
std::vector<TASQuake::PathPoint>* IPC_Prediction_GetPoints();
std::vector<TASQuake::Rect>* IPC_Prediction_GetRects();
