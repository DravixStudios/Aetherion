#pragma once
#include <iostream>

struct Vector4 {
    /* XYZW */
    float x, y, z, w;

    /* Constructors */
    Vector4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
    Vector4(const Vector4& v);
    Vector4(float x, float y, float z, float w);

    /* Operator overloadings */
    Vector4 operator+(const Vector4& v);
    Vector4 operator-(const Vector4& v);
    Vector4 operator*(const Vector4& v);
    Vector4 operator/(const Vector4& v);

    Vector4 operator+(float value);
    Vector4 operator-(float value);
    Vector4 operator*(float value);
    Vector4 operator/(float value);
};