#include "cpp_quakedef.hpp"
#include "camera.hpp"
#include "drag_editing.hpp"
#include "script_playback.hpp"

enum class DragType
{
    None, Single, Stack
};

static DragType dragType = DragType::None;
static double accum_yaw = 0;
static double accum_pitch = 0;
static int block_index = 0;

static float view_yaw = 0.0f;
static float view_pitch = 0.0f;

static int original_frame = 0;
static bool original_had_strafe_yaw = false;
static float original_strafeyaw = 0;

void TASQuake::Drag_Confirm()
{
    dragType = DragType::None;
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
}

bool TASQuake::IsDragging()
{
    return dragType != DragType::None && tas_freecam.value != 0;
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

void TASQuake::Drag_Editing_Hook()
{
    if(!IsDragging())
    {
        return;
    }

    accum_pitch += cl.viewangles[PITCH] - view_pitch;
    accum_yaw += cl.viewangles[YAW] - view_yaw;
    cl.viewangles[PITCH] = view_pitch;
    cl.viewangles[YAW] = view_yaw;

    const float PITCH_SENS = 5.0f;
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

bool SelectDragIndex();

void CMD_TAS_Drag_Single()
{
    if(tas_freecam.value == 0 || !SelectDragIndex())
        return;
}

void CMD_TAS_Drag_Stack()
{
    if(tas_freecam.value == 0 || !SelectDragIndex())
        return;
}
