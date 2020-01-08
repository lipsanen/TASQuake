#include "cpp_quakedef.hpp"

float* Vector3f::Get_Array()
{
	return &x;
}

float & Vector3f::operator[](int index)
{
	if(index == 0)
		return x;
	else if(index == 1)
		return y;
	else
		return z;
}
