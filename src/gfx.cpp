#include "gfx.h"

Color Vec3ToColor(Vec3 v)
{
    return {.r = fabs(v.x), .g = fabs(v.y), .b = fabs(v.z)};
}

Vec3 TriNormal(Vec3 v1, Vec3 v2, Vec3 v3)
{
    Vec3 A = v2.Sub(v1);
    Vec3 B = v3.Sub(v1);
    float Nx = A.y * B.z - A.z * B.y;
    float Ny = A.z * B.x - A.x * B.z;
    float Nz = A.x * B.y - A.y * B.x;
    return {Nx, Ny, Nz};
}

Mat4 Camera::LookAtMatrix(Vec3 up)
    {
        Vec3 f = target.Sub(pos).Normalize();
        Vec3 s = f.Cross(up.Normalize()).Normalize();
        Vec3 u = s.Cross(f);

        std::cerr << "f: " << f << ". s: " << s << " u: " << u << std::endl;

        Mat4 M = Mat4
        {
            s.x, u.x, -f.x, 0,
            u.x, u.y, u.z, 0,
            s.z, u.z, -f.z, 0,
            0, 0, 0, 1,
        };

        Vec3 neg_eye = {-pos.x, -pos.y, -pos.z};

        std::cerr << "M:" << std::endl;
        std::cerr << M << std::endl;

        return M.Mul4x4(Translate3D(neg_eye));
    }


Mat4 Frustum(float l, float r, float b, float t, float n, float f) {
	float t1 = 2 * n;
	float t2 = r - l;
	float t3 = t - b;
	float t4 = f - n;
	return {
		t1 / t2, 0, (r + l) / t2, 0,
		0, t1 / t3, (t + b) / t3, 0,
		0, 0, (-f - n) / t4, (-t1 * f) / t4,
		0, 0, -1, 0};
}

Mat4 ProjectionMatrix(float fovy_deg, float aspect, float near, float far){
    float ymax = near * tan(fovy_deg*3.14159/360);
	float xmax = ymax * aspect;
	return Frustum(-xmax, xmax, -ymax, ymax, near, far);
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

bool edgeFunction(const Vec3 &a, const Vec3 &b, const Vec3 &c)
{
	return ((c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x) >= 0);
}

bool insideTri(Vec3 &v0, Vec3 &v1, Vec3 &v2, Vec3 &p)
{
	bool inside = true;
	inside &= edgeFunction(v0, v1, p);
	inside &= edgeFunction(v1, v2, p);
	inside &= edgeFunction(v2, v0, p);
	return inside;
}