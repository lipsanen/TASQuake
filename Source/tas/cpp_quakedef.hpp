#pragma once
#pragma warning(disable : 4010)
extern "C"
{
#include "../quakedef.h"
}

#ifdef __linux__
#undef max
#undef min
#undef rand
#endif

#include "libtasquake/json.hpp"
#include "libtasquake/utils.hpp"

#ifdef __linux__
#define rand tas_rand
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif