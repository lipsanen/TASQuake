#include "cpp_quakedef.hpp"

float* Vector3f::Get_Array()
{
	return &x;
}

float & Vector3f::operator[](int index)
{
	return Get_Array()[index];
}
