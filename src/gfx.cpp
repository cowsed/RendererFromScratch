#include "gfx.h"

Color Vec3ToColor(Vec3 v)
{
    return {.r = fabs(v.x), .g = fabs(v.y), .b = fabs(v.z)};
}

Vec3 TriNormal(Vec3 v1, Vec3 v2, Vec3 v3)
{
    Vec3 A = v2 - v1;
    Vec3 B = v3- v1;
    float Nx = A.y * B.z - A.z * B.y;
    float Ny = A.z * B.x - A.x * B.z;
    float Nz = A.x * B.y - A.y * B.x;
    return {Nx, Ny, Nz};
}




Rect bounding_box2d(Vec2 v0, Vec2 v1, Vec2 v2){
	float minx = min(min(v0.x, v1.x), v2.x);
	float miny = min(min(v0.y, v1.y), v2.y);

	float maxx = max(max(v0.x, v1.x), v2.x);
	float maxy = max(max(v0.y, v1.y), v2.y);
	return {
		.min = {minx, miny},
		.max = {maxx, maxy}
	};
}

