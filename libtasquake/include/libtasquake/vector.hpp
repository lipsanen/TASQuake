#pragma once

namespace TASQuake
{
    constexpr int PITCH_INDEX = 0;
    constexpr int YAW_INDEX = 1;
    constexpr int ROLL_INDEX = 2;

    struct Vector {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        Vector() = default;
        Vector(float x, float y, float z);
        float Distance(const Vector& rhs) const;
        float Norm() const;
        float& operator[](int index);
        const float& operator[](int index) const;
        Vector operator-(const Vector& rhs) const;
    };

    struct Trace {
        Vector m_vecStartPos;
        Vector m_vecAngles;
    };

    float DistanceFromPoint(const Trace& trace, const Vector& point);
    void AngleVectors (Vector angles, Vector& forward);
    Vector VecCrossProduct(Vector v1, Vector v2);
}
