#pragma once

#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/draw.hpp"
#include "libtasquake/prediction.hpp"

void IPC_Prediction_Frame_Hook();
void IPC_Prediction_Read_Response(ipc::Message& msg);
bool IPC_Prediction_HasLine();
TASQuake::PredictionData* IPC_Get_PredictionData();
