#pragma once

#include "draw.hpp"
#include "libtasquake/draw.hpp"
#include "libtasquake/boost_ipc.hpp"
#include <vector>

extern cvar_t tas_predict_real;

void GamePrediction_Frame_Hook();
void GamePrediction_Receive_IPC(ipc::Message& msg);
