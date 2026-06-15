#pragma once
#include <iostream>

struct Vector4 {
    /* XYZW */
    float x, y, z, w;

    /* Constructors */
    Vector4() = default;
    Vector4(float xyzw) : x(xyzw), y(xyzw), z(xyzw), w(xyzw) {}
    Vector4(const Vector4&) = default;
    Vector4& operator=(const Vector4&) = default;
    
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