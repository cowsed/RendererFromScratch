#pragma once
#include "gfx_math.h"
#include <stdint.h>

const Vec3 up_vec = {0, 1, 0};

struct IntColor
{

    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Color
{
    float r;
    float g;
    float b;
    IntColor toIntColor()
    {
        if (r > 1.0)
        {
            r = 1.0;
        }
        if (g > 1.0)
        {
            g = 1.0;
        }
        if (b > 1.0)
        {
            b = 1.0;
        }
        uint8_t nr = static_cast<uint8_t>(255.999 * r); // clamp(r, 0.0, 1.0));
        uint8_t ng = static_cast<uint8_t>(255.999 * g); // clamp(g, 0.0, 1.0));
        uint8_t nb = static_cast<uint8_t>(255.999 * b); // clamp(b, 0.0, 1.0));
        return {0, nr, ng, nb};
    }
};
Color Vec3ToColor(Vec3 v);

// ccw ordering
// normal calculation as such
struct Tri
{
    int v1_index;
    int v2_index;
    int v3_index;
    int matID;
};

Vec3 TriNormal(Vec3 v1, Vec3 v2, Vec3 v3);

struct Material
{
    Color ambient;
    Color diffuse;
    Color specular;
    float Ns; // 0-1000
};

/// @brief A camera represents the point that the scene is rendered from
struct Camera
{
    Vec3 pos;
    Vec3 target;

    Mat4 LookAtMatrix(Vec3 up);
};

Mat4 Frustum(float l, float r, float b, float t, float n, float f);

Mat4 ProjectionMatrix(float fovy_deg, float aspect, float near, float far);

/// @brief calculate the bounding box of 3 vertices in screen space
/// @param v0 one vertice of a tri
/// @param v1 another one
/// @param v2 another on
/// @return a rectangle representing the axis aligned bounding box of those vertices
Rect bounding_box2d(Vec2 v0, Vec2 v1, Vec2 v2);
