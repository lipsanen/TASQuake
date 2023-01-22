#include "libtasquake/draw.hpp"
#include "libtasquake/prediction.hpp"
#include "script_playback.hpp"
#include "ipc2.hpp"
#include "ipc_prediction.hpp"
#include "optimizer_quake.hpp"
#include "prediction.hpp"
#include "simulate.hpp"
#include "hooks.h"

using namespace TASQuake;

static double last_request = 0;
static uint32_t last_request_id = 0;
static double current_line_time = 0;
static PredictionData data;
static bool has_data = false;

static void Request() {
    auto connection = TASQuake::Get_First_Session();
    data.m_vecFBdata.clear();
    data.m_vecPoints.clear();
    has_data = false;
    if(connection == -1)
        return; // No clients, give up
    
    last_request = Sys_DoubleTime();
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)TASQuake::IPCMessages::Predict;
    writer.WriteBytes(&type, 1);
    auto info = GetPlaybackInfo();

    int32_t target_frame;
    int32_t current_frame;
    TASQuake::Get_Prediction_Frames(current_frame, target_frame);
    writer.WriteBytes(&current_frame, sizeof(int32_t));
    writer.WriteBytes(&target_frame, sizeof(int32_t));
    writer.WriteBytes(&last_request_id, sizeof(int32_t));
    info->current_script.Write_To_Memory(writer);

    TASQuake::SV_StopMultiGameOpt();
    TASQuake::SV_SendMessage(connection, writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

void IPC_Prediction_Read_Response(ipc::Message& msg) {
    uint32_t request_id;
    auto reader = TASQuakeIO::BufferReadInterface::Init((std::uint8_t*)msg.address + 1, msg.length - 1);
    reader.Read(&request_id, sizeof(request_id));

    if(request_id != last_request_id)
        return;

    data.Load_From_Memory(reader);
    has_data = true;
    current_line_time = last_request;
}

static bool Should_Predict() {
    auto info = GetPlaybackInfo();
    bool has_line = IPC_Prediction_HasLine();
    bool has_been_edited = last_request < info->last_edited;
    if(Sys_DoubleTime() - info->last_edited <= 0.1 || tas_playing.value == 0 || tas_gamestate != paused) {
        return false; // game needs to be paused while playing a TAS
    } else if(has_been_edited) {
        return true; // Has been edited, need to generate new line
    } else if(current_line_time == last_request && !has_line) {
        return true; // We have received a response, but some issues with the line
    } else {
        return false;
    }
}

void IPC_Prediction_Frame_Hook() {
    if(Should_Predict()) {
        Request();
    } else if(IPC_Prediction_HasLine() && (tas_playing.value == 0 || tas_gamestate != paused)) {
        has_data = false;
    }
}

bool IPC_Prediction_HasLine() {
    int32_t start, end;
    TASQuake::Get_Prediction_Frames(start, end);
    auto info = GetPlaybackInfo();
    return current_line_time >= info->last_edited && end <= data.m_iEndFrame && start >= data.m_iStartFrame;
}

TASQuake::PredictionData* IPC_Get_PredictionData() {
    return &data;
}
