#pragma once

#include "cpp_quakedef.hpp"
#include "libtasquake/json.hpp"

nlohmann::json Dump_SV();
nlohmann::json Dump_Test();
void Cmd_TAS_Dump_SV();