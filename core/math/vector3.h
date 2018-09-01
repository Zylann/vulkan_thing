#ifndef HEADER_VECTOR3_H
#define HEADER_VECTOR3_H

#include <cassert>
#include "math_funcs.h"

struct Vector3 {

    union {
        struct {
            float x;
            float y;
            float z;
        };

        float coord[3];
    };

    Vector3(): x(0), y(0), z(0) { }

    Vector3(float p_x, float p_y, float p_z): x(p_x), y(p_y), z(p_z) { }

    Vector3(const Vector3 &other): x(other.x), y(other.y), z(other.z) { }

    inline const float &operator[](int p_axis) const {
        return coord[p_axis];
    }

    inline float &operator[](int p_axis) {
        return coord[p_axis];
    }

    inline void operator *=(float k) {
        x *= k;
        y *= k;
        z *= k;
    }

    inline void operator /=(float k) {
        assert(k != 0);
        x /= k;
        y /= k;
        z /= k;
    }

    float length() const {

        float x2 = x * x;
        float y2 = y * y;
        float z2 = z * z;

        return Math::sqrt(x2 + y2 + z2);
    }

    float length_squared() const {

        float x2 = x * x;
        float y2 = y * y;
        float z2 = z * z;

        return x2 + y2 + z2;
    }

    void normalize() {

        float l = length();
        if (l == 0) {
            x = y = z = 0;
        } else {
            x /= l;
            y /= l;
            z /= l;
        }
    }

    Vector3 normalized() const {

        Vector3 v = *this;
        v.normalize();
        return v;
    }

    Vector3 cross(const Vector3 &p_b) const {

        Vector3 ret(
                (y * p_b.z) - (z * p_b.y),
                (z * p_b.x) - (x * p_b.z),
                (x * p_b.y) - (y * p_b.x));

        return ret;
    }

    float dot(const Vector3 &p_b) const {

        return x * p_b.x + y * p_b.y + z * p_b.z;
    }
};

inline Vector3 operator+(Vector3 a, Vector3 b) {
    return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vector3 operator-(Vector3 a, Vector3 b) {
    return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
}

#endif // HEADER_VECTOR3_H
