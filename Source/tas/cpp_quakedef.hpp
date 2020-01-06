#pragma once
extern "C"
{
#include "..\quakedef.h"
}

struct Vector3f
{
	float x, y, z;
	float* Get_Array();
	float& operator[](int index);
};