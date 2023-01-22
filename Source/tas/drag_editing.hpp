#pragma once

namespace TASQuake
{
    void Drag_Confirm();
    void Drag_Cancel();
    void Drag_MouseMove_Hook(int mousex, int mousey);
    bool IsDragging();
    int Selected_Block();
    void StopDrag();
}

void CMD_TAS_Drag_Single();
void CMD_TAS_Drag_Stack();
