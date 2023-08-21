#pragma once

#include "cpp_quakedef.hpp"

namespace TASQuake {
    void Log_Frame_Hook();

    // desc: Enables logging
    extern cvar_t tas_log;
    // desc: Log filters
    extern cvar_t tas_log_filter;
}
