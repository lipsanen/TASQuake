#include "libtasquake/utils.hpp"
#include "libtasquake/vector.hpp"
#include <cmath>

using namespace TASQuake;

float Vector::Distance(const Vector& rhs) const
{
	return std::sqrt((x - rhs.x) * (x - rhs.x) + (y - rhs.y) * (y - rhs.y) + (z - rhs.z) * (z - rhs.z));
}

float Vector::Norm() const
{
	return std::sqrt(x * x + y * y + z * z);
}

float TASQuake::DistanceFromPoint(const Trace& trace, const Vector& point)
{
	Vector diff = (point - trace.m_vecStartPos);
	Vector dir;
	AngleVectors(trace.m_vecAngles, dir);
	Vector cross = TASQuake::VecCrossProduct(diff, dir);

    return cross.Norm() / dir.Norm();
}

float& Vector::operator[](int index) {
    if(index == 0) {
        return x;
    } else if(index == 1) {
        return y;
    } else {
        return z;
    }
}
const float& Vector::operator[](int index) const {
    if(index == 0) {
        return x;
    } else if(index == 1) {
        return y;
    } else {
        return z;
    }
}

Vector Vector::operator-(const Vector& rhs) const
{
	return TASQuake::Vector(x - rhs.x, y - rhs.y, z - rhs.z);
}

Vector TASQuake::VecCrossProduct(Vector v1, Vector v2)	
{
    Vector v;
    v.x = (v1)[1] * (v2)[2] - (v1)[2] * (v2)[1];
    v.y = (v1)[2] * (v2)[0] - (v1)[0] * (v2)[2];
    v.z = (v1)[0] * (v2)[1] - (v1)[1] * (v2)[0];
    return v;
}

Vector::Vector(float x, float y, float z) : x(x), y(y), z(z)
{

}

void TASQuake::AngleVectors (Vector angles, Vector& forward)
{
	float	angle, sr, sp, sy, cr, cp, cy, temp;

	if (angles[YAW_INDEX])
	{
		angle = M_DEG2RAD * angles[YAW_INDEX];
		sy = sin(angle);
		cy = cos(angle);
	}
	else
	{
		sy = 0;
		cy = 1;
	}

	if (angles[PITCH_INDEX])
	{
		angle = M_DEG2RAD * angles[PITCH_INDEX];
		sp = sin(angle);
		cp = cos(angle);
	}
	else
	{
		sp = 0;
		cp = 1;
	}


    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;
}
