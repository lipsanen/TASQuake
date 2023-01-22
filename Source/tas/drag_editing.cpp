#include "cpp_quakedef.hpp"
#include "camera.hpp"
#include "drag_editing.hpp"
#include "ipc_prediction.hpp"
#include "script_playback.hpp"

enum class DragType
{
    None, Single, Stack
};

static DragType dragType = DragType::None;
static double accum_yaw = 0;
static double accum_pitch = 0;
static int block_index = 0;

static int original_frame = 0;
static bool original_had_strafe_yaw = false;
static float original_strafeyaw = 0;

void TASQuake::Drag_Confirm()
{
    auto info = GetPlaybackInfo();
    float strafeyaw = info->current_script.blocks[block_index].convars["tas_strafe_yaw"];

    if(strafeyaw == original_strafeyaw)
    {
        info->current_script.blocks[block_index].convars.erase("tas_strafe_yaw");
    }

    dragType = DragType::None;
    SCR_CenterPrint("Drag edit confirmed");
}

void TASQuake::Drag_Cancel()
{
    auto info = GetPlaybackInfo();
    info->last_edited = Sys_DoubleTime();

    if(dragType == DragType::Single)
    {
        info->current_script.blocks[block_index].frame = original_frame;
    }
    else if(dragType == DragType::Stack)
    {
        int delta = original_frame - info->current_script.blocks[block_index].frame;
        info->current_script.ShiftBlocks(block_index, delta);
    }

    if(original_had_strafe_yaw)
    {
        info->current_script.AddCvar("tas_strafe_yaw", original_strafeyaw, original_frame);
    }
    else
    {
        info->current_script.RemoveCvarsFromRange("tas_strafe_yaw", original_frame, original_frame);
    }

    dragType = DragType::None;
    SCR_CenterPrint("Drag edit cancelled");
}

bool TASQuake::IsDragging()
{
    auto info = GetPlaybackInfo();
    return dragType != DragType::None && In_Freecam() && block_index < info->current_script.blocks.size();
}

static void ApplySingleDrag(int delta)
{
    auto info = GetPlaybackInfo();
    info->current_script.ShiftSingleBlock(block_index, delta);
}

static void ApplyStackDrag(int delta)
{
    auto info = GetPlaybackInfo();
    info->current_script.ShiftBlocks(block_index, delta);
}

void TASQuake::Drag_MouseMove_Hook(int mousex, int mousey)
{
    if(!IsDragging())
    {
        return;
    }

    accum_pitch -= mousey * m_pitch.value * sensitivity.value;
    accum_yaw -= mousex * m_yaw.value * sensitivity.value;

    const float PITCH_SENS = 20.0f;
    int pitchTicks = accum_pitch / PITCH_SENS;
    auto info = GetPlaybackInfo();
    bool changes = pitchTicks != 0 || accum_yaw != 0;
    
    if(pitchTicks != 0)
    {
        if(dragType == DragType::Single)
        {
            ApplySingleDrag(pitchTicks);
        }
        else if(dragType == DragType::Stack)
        {
            ApplyStackDrag(pitchTicks);
        }

        accum_pitch -= pitchTicks * PITCH_SENS;
    }

    if(accum_yaw != 0)
    {
        float yaw = info->current_script.blocks[block_index].convars["tas_strafe_yaw"] + accum_yaw;
        yaw = NormalizeDeg(yaw);
        info->current_script.blocks[block_index].convars["tas_strafe_yaw"] = yaw;
        accum_yaw = 0;
    }

    if(changes)
    {
        info->last_edited = Sys_DoubleTime();
    }
}


void TASQuake::StopDrag()
{
    dragType = DragType::None;
}

bool SelectDragIndex()
{
    if(!IPC_Prediction_HasLine())
    {
        return false;
    }

    auto data = IPC_Get_PredictionData();
    TASQuake::Trace trace;
    Get_Camera_PosAngles(trace.m_vecStartPos, trace.m_vecAngles);
    int index = data->FindFrameBlock(trace);

    if(index == -1)
    {
        return false;
    }
    else
    {
        auto info = GetPlaybackInfo();

        block_index = index;
        FrameBlock* block = &info->current_script.blocks[block_index];
        original_frame = block->frame;
        original_had_strafe_yaw = block->HasConvar("tas_strafe_yaw");

        if(original_had_strafe_yaw)
        {
            original_strafeyaw = block->convars["tas_strafe_yaw"];
        }
        else
        {
            FrameBlock stacked;

            for(size_t i=0; i < info->current_script.blocks.size(); ++i)
            {
                stacked.Stack(info->current_script.blocks[i]);
            }

            if(stacked.HasConvar("tas_strafe_yaw"))
            {
                original_had_strafe_yaw = stacked.convars["tas_strafe_yaw"];
            }
            else
            {
                original_strafeyaw = 0.0f;
            } 
        }

        return true;
    }
}

static bool TryDrag()
{
    if(!In_Freecam())
    {
        Con_Printf("Not in freecam mode, cannot start dragging\n");
        return false;
    }
    else if(!SelectDragIndex())
    {
        Con_Printf("Was unable to find a frameblock for dragging\n");
        return false;
    }
    
    return true;
}

void CMD_TAS_Drag_Single()
{
    if(!TryDrag())
        return;
    dragType = DragType::Single;
    SCR_CenterPrint("Single drag started");
}

void CMD_TAS_Drag_Stack()
{
    if(!TryDrag())
        return;
    dragType = DragType::Stack;
    SCR_CenterPrint("Stack drag started");
}
