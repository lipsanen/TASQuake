#pragma once

#include <array>

namespace TASQuake {
    struct PathPoint
    {
        PathPoint();
        float point[3];
        std::array<float, 4> color;
    };

    struct Rect
    {
        Rect();
        std::array<float, 4> color;
        float mins[3];
        float maxs[3];
        int id;
        Rect(const std::array<float, 4>& _color, float _mins[3], float _maxs[3], int _id);
        static Rect Get_Rect(const std::array<float, 4>& _color, float center[3], float width, float height, int id);
        static Rect Get_Rect(const std::array<float, 4>& _color, float center[3], float angles[3], float length, float width, float height, int id);
    };
}
