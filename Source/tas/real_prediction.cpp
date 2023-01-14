#include "cpp_quakedef.hpp"
#include "hooks.h"
#include "draw.hpp"
#include "libtasquake/script_playback.hpp"
#include "script_playback.hpp"
#include "simulate.hpp"
#include <vector>

using namespace TASQuake;

static std::vector<PathPoint> real_line;
static std::vector<Rect> real_rects;
const std::array<float, 4> collision_path = { 0, 1, 0, 1 };
const std::array<float, 4> normal_path = { 0, 1, 0, 1 };
const std::array<float, 4> box_color = { 0, 0, 1, 0.5 };
cvar_t tas_predict_real = {"tas_predict_real", "0"};
static double last_edited_time = 0;

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
	if(tas_playing.value != 0 && tas_gamestate == unpaused && tas_predict_real.value != 0) {
        auto playback = GetPlaybackInfo();
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
}

