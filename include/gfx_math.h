#pragma once
#include <iostream>
#include "math.h"

struct Vec2;
struct Vec3;
struct Vec4;

struct Vec2{
	float x;
	float y;
};

struct Rect{
	Vec2 min;
	Vec2 max;
};

struct Vec3{
	float x;
	float y;
	float z;
	Vec2 toVec2();
	Vec4 toVec4(float w);

	float length();
	Vec3 Div(float s);
	Vec3 Sub(Vec3 b);
	Vec3 Cross(Vec3 b);
	Vec3 Normalize();
};

std::ostream& operator<<(std::ostream& os, const Vec3 m);

struct Vec4{
	float x;
	float y;
	float z;
    float w;
	Vec3 toVec3();
};
std::ostream& operator<<(std::ostream& os, const Vec4 m);


struct Mat4{
	float X00, X01, X02, X03;
	float X10, X11, X12, X13;
    float X20, X21, X22, X23;
    float X30, X31, X32, X33;

	Vec3 Mul4xV3(Vec3 v);
    Vec4 Mul4xV4(Vec4 v);
	Mat4 Mul4x4(Mat4 other);
};
Mat4 Mat4Identity();
Mat4 Translate3D(Vec3 v);


std::ostream& operator<<(std::ostream& os, const Mat4& m);

float min(float a, float b);
float max(float a, float b);
float clamp(float v, float min, float max);