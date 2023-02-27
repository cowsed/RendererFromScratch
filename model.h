#pragma once
#include "gfx_math.h"
#include "gfx.h"
// Model
const int num_points = 4;
const int num_faces = 2;
const int num_materials = 1;
Vec3 points[num_points] = {{-1.000000, -1.000000, -0.000000}, {1.000000, -1.000000, -0.000000}, {-1.000000, 1.000000, 0.000000}, {1.000000, 1.000000, 0.000000}};
Tri faces[num_faces] = {{1, 2, 0, 0}, {1, 3, 2, 0}};
Material materials[num_materials] = {{.ambient = {1.000000, 1.000000, 1.000000}, .diffuse = {1.000000, 1.000000, 1.000000}, .specular = {1.000000, 1.000000, 1.000000}, .Ns = 1.000000}, };
Vec3 normals[num_faces] = {{0.000000, 0.000000, 4.000000}, {0.000000, -0.000000, 4.000000}};
