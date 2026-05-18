#include "Math/Vector4.h"

Vector4::Vector4(const Vector4& v) {
    this->x = v.x;
    this->y = v.y;
    this->z= v.z;
    this->w = v.w;
}

Vector4::Vector4(float x, float y, float z, float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

Vector4
Vector4::operator+(const Vector4& v) {
    return Vector4{ x + v.x, y + v.y, z + v.z, w + v.w };
}

Vector4
Vector4::operator-(const Vector4& v) {
    return Vector4{ x - v.x, y - v.y, z - v.z, w - v.w };
}

Vector4
Vector4::operator*(const Vector4& v) {
    return Vector4{ x * v.x, y * v.y, z * v.z, w * v.w };
}

Vector4
Vector4::operator/(const Vector4& v) {
    return Vector4{ x / v.x, y / v.y, z / v.z, w / v.w };
}

Vector4
Vector4::operator+(float value) {
    return Vector4{ x + value, y + value, z + value, w + value };
}

Vector4
Vector4::operator-(float value) {
    return Vector4{ x - value, y - value, z - value, w - value };
}

Vector4
Vector4::operator*(float value) {
    return Vector4{ x * value, y * value, z * value, w * value };
}

Vector4
Vector4::operator/(float value) {
    return Vector4{ x / value, y / value, z / value, w / value };
}

