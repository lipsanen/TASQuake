#include "cpp_quakedef.hpp"
#include "libtasquake/prediction.hpp"
#include "libtasquake/script_playback.hpp"
#include <vector>
#include "hooks.h"
#include "draw.hpp"
#include "ipc2.hpp"
#include "prediction.hpp"
#include "real_prediction.hpp"
#include "savestate.hpp"
#include "script_playback.hpp"
#include "simulate.hpp"

using namespace TASQuake;

static PredictionData data;
cvar_t tas_predict_real = {"tas_predict_real", "0"};
static double last_edited_time = 0;
static bool ipc_request_finished = true;
static int32_t ipc_prediction_id = 0;

void GamePrediction_Receive_IPC(ipc::Message& msg) {
    auto iface = TASQuakeIO::BufferReadInterface::Init((uint8_t*)msg.address + 1, msg.length - 1);
    iface.Read(&data.m_iStartFrame);
    iface.Read(&data.m_iEndFrame);
    iface.Read(&ipc_prediction_id);
    data.m_vecFBdata.clear();
    data.m_vecPoints.clear();

    TASScript script;
    script.Load_From_Memory(iface);

    auto info = GetPlaybackInfo();
    int first_changed_frame = 0;
    info->current_script.ApplyChanges(&script, first_changed_frame);
    first_changed_frame = std::min(first_changed_frame, data.m_iStartFrame);
	Savestate_Script_Updated(first_changed_frame);
    info->last_edited = Sys_DoubleTime();
    Run_Script(data.m_iEndFrame, true);
    ipc_request_finished = false;
}

static void GamePredition_IPC_Respond() {
    ipc_request_finished = true;
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)TASQuake::IPCMessages::Predict;
    writer.WriteBytes(&type, sizeof(type));
    writer.Write(&ipc_prediction_id);
    data.Write_To_Memory(writer);
    TASQuake::CL_SendMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

void GamePrediction_Frame_Hook() {
    auto playback = GetPlaybackInfo();

	if(tas_playing.value != 0 && tas_gamestate == unpaused && (tas_predict_real.value != 0 || !ipc_request_finished)) {
        if(cls.state == ca_connected) {
            
            if(playback->current_frame >= data.m_iStartFrame)
            {
                TASQuake::Vector v;
                VectorCopy(sv_player->v.origin, v);
                data.m_vecPoints.push_back(v);
                const FrameBlock* block = playback->Get_Current_Block();

                if(block && block->frame == playback->current_frame) {
                    TASQuake::FrameBlockIndex fbindex;
                    fbindex.m_uFrame = block->frame;
                    fbindex.m_uBlockIndex = playback->GetBlockNumber();
                    data.m_vecFBdata.push_back(fbindex);
                }
            }
        }
    }

    if(!ipc_request_finished && playback->current_frame >= data.m_iEndFrame - 1) {
        GamePredition_IPC_Respond();
    }
}

