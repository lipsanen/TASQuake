#pragma once
extern "C"
{
#include "..\quakedef.h"
}

struct Vector4f
{
	float x, y, z, w;
	float* Get_Array();
	float& operator[](int index);
};

struct Vector3f
{
	float x, y, z;
	float* Get_Array();
	float& operator[](int index);
};