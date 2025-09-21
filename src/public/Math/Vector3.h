#pragma once
#include <iostream>

/* Our Vector3 structure */
struct Vector3 {
	/* XYZ */
	float x, y, z;

	/* Constructors */
	Vector3(const Vector3& v);
	Vector3(float x, float y, float z);

	/* Operator overloadings */
	Vector3 operator+(const Vector3& v);
	Vector3 operator-(const Vector3& v);
	Vector3 operator*(const Vector3& v);
	Vector3 operator/(const Vector3& v);

	Vector3 operator+(float value);
	Vector3 operator-(float value);
	Vector3 operator*(float value);
	Vector3 operator/(float value);
};