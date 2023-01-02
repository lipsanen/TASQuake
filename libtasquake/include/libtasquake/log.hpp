#pragma once

namespace TASQuake {
    typedef void(*LogFunc)(char* text);

    void SetLog(LogFunc func);
    void Log(const char* fmt, ...);
}
