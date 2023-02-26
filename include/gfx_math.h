#pragma once
#include <iostream>
#include "math.h"

struct Vec2;
struct Vec3;
struct Vec4;

struct Vec2
{
	float x;
	float y;
};

struct Rect
{
	Vec2 min;
	Vec2 max;
};

struct Vec3
{
	constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	constexpr Vec3() : x(0.0), y(0.0), z(0.0) {}
	float x;
	float y;
	float z;
	Vec2 toVec2();
	float length();

	Vec3 operator-(const Vec3 b) const;
	Vec3 operator+(const Vec3 b) const;
	Vec3 operator/(const float s) const;
	Vec3 operator*(const float s) const;

	Vec3 RotateY(float radians) const;
	Vec3 RotateZ(float radians) const;

	Vec3 Normalize();
	float Dot(const Vec3 b) const;
	Vec4 toVec4(const float w) const;
};
struct Vec4
{
	float x;
	float y;
	float z;
	float w;
	Vec3 const toVec3();
};

std::ostream &operator<<(std::ostream &os, const Vec3 m);

struct Mat4
{
	float X00, X01, X02, X03;
	float X10, X11, X12, X13;
	float X20, X21, X22, X23;
	float X30, X31, X32, X33;

	Vec3 Mul4xV3(Vec3 v);
	Vec4 Mul4xV4(const Vec4 v) const;
	Mat4 Mul4x4(const Mat4 other) const;
	Mat4 operator*(const Mat4 other) const;
};
Mat4 Mat4Identity();
Mat4 Translate3D(const Vec3 v);
Mat4 RotateX(const float rad);
Mat4 RotateY(const float rad);
Mat4 RotateZ(const float rad);

std::ostream &operator<<(std::ostream &os, const Mat4 &m);

float min(float a, float b);
float max(float a, float b);
float clamp(float v, float min, float max);