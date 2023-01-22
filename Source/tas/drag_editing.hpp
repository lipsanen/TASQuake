#pragma once

namespace TASQuake
{
    void Drag_Confirm();
    void Drag_Cancel();
    void Drag_Editing_Hook();
    bool IsDragging();
    int Selected_Block();
}

void CMD_TAS_Drag_Single();
void CMD_TAS_Drag_Stack();
