#include "gfx.h"


Vec3 Color::toVec3()const {
	return {r, g, b};
}

Color Vec3ToColor(const Vec3 v)
{
    return {.r = fabs(v.x), .g = fabs(v.y), .b = fabs(v.z)};
}

Vec3 TriNormal(const Vec3 &v1, const Vec3 &v2, const Vec3 &v3)
{
    Vec3 A = v2 - v1;
    Vec3 B = v3- v1;
    float Nx = A.y * B.z - A.z * B.y;
    float Ny = A.z * B.x - A.x * B.z;
    float Nz = A.x * B.y - A.y * B.x;
    return {Nx, Ny, Nz};
}



Rect bounding_box2d(const Vec2 &v0, const Vec2 &v1, const Vec2 &v2){
	float minx = min(min(v0.x, v1.x), v2.x);
	float miny = min(min(v0.y, v1.y), v2.y);

	float maxx = max(max(v0.x, v1.x), v2.x);
	float maxy = max(max(v0.y, v1.y), v2.y);
	return {
		.min = {minx, miny},
		.max = {maxx, maxy}
	};
}




