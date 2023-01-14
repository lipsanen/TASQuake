#include "cpp_quakedef.hpp"
#include "libtasquake/script_playback.hpp"
#include <vector>
#include "hooks.h"
#include "draw.hpp"
#include "ipc2.hpp"
#include "real_prediction.hpp"
#include "savestate.hpp"
#include "script_playback.hpp"
#include "simulate.hpp"

using namespace TASQuake;

static std::vector<PathPoint> real_line;
static std::vector<Rect> real_rects;
const std::array<float, 4> collision_path = { 0, 1, 0, 1 };
const std::array<float, 4> normal_path = { 0, 1, 0, 1 };
const std::array<float, 4> box_color = { 0, 0, 1, 0.5 };
cvar_t tas_predict_real = {"tas_predict_real", "0"};
static double last_edited_time = 0;
static bool ipc_request_finished = true;
static int32_t ipc_target_frame = 0;
static int32_t ipc_start_frame = 0;
static int32_t ipc_simulation_id = 0;

void GamePrediction_Receive_IPC(ipc::Message& msg) {
    auto iface = TASQuakeIO::BufferReadInterface::Init((uint8_t*)msg.address + 1, msg.length - 1);
    iface.Read(&ipc_start_frame, sizeof(ipc_start_frame));
    iface.Read(&ipc_target_frame, sizeof(ipc_target_frame));
    iface.Read(&ipc_simulation_id, sizeof(ipc_simulation_id));

    TASScript script;
    script.Load_From_Memory(iface);

    auto info = GetPlaybackInfo();
    int first_changed_frame = 0;
    info->current_script.ApplyChanges(&script, first_changed_frame);
	Savestate_Script_Updated(first_changed_frame);
    info->last_edited = Sys_DoubleTime();
    Run_Script(ipc_target_frame, true);
    ipc_request_finished = false;
}

static void GamePredition_IPC_Respond() {
    ipc_request_finished = true;
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)TASQuake::IPCMessages::Predict;
    writer.WriteBytes(&type, sizeof(type));
    writer.WriteBytes(&ipc_simulation_id, sizeof(ipc_simulation_id));
    writer.WritePODVec(real_line);
    writer.WritePODVec(real_rects);
    TASQuake::CL_SendMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

std::vector<PathPoint>* GamePrediction_GetPoints() {
    return &real_line;
}

std::vector<Rect>* GamePrediction_GetRects() {
    return &real_rects;
}

bool GamePrediction_HasLine() {
    int frames = 72 * tas_predict_amount.value;
    auto playback = GetPlaybackInfo();
    int totalFrames = playback->current_frame + frames;
    return playback->last_edited < last_edited_time && totalFrames < real_line.size() && tas_predict_real.value != 0;
}

void GamePrediction_Frame_Hook() {
    auto playback = GetPlaybackInfo();

	if(tas_playing.value != 0 && tas_gamestate == unpaused && (tas_predict_real.value != 0 || !ipc_request_finished)) {
        if(cls.state == ca_connected) {
            // Resize so that current time matches index
            if(real_line.size() > playback->current_frame && last_edited_time > playback->last_edited) {
                return;
            } else if(real_line.size() != playback->current_frame) {
                real_line.resize(playback->current_frame);
            }

            PathPoint p;
            p.color = normal_path;
            VectorCopy(sv_player->v.origin, p.point);
            const FrameBlock* block = playback->Get_Current_Block();
            real_line.push_back(p);
            last_edited_time = Sys_DoubleTime();

            if(block && block->frame == playback->current_frame) {
                auto count = playback->GetBlockNumber();
                if(count != real_rects.size()) {
                    real_rects.resize(count);
                }
                real_rects.push_back(Rect::Get_Rect(box_color, sv_player->v.origin, 3, 3, PREDICTION_ID));
            }
        }
    }

    if(!ipc_request_finished && playback->current_frame == ipc_target_frame) {
        GamePredition_IPC_Respond();
    }
}

