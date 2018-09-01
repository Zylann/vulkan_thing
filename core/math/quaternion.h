#ifndef HEADER_QUATERNION_H
#define HEADER_QUATERNION_H

#include "vector3.h"
#include "math_funcs.h"

struct Quaternion {

    // Real part
    float w;

    // Imaginary part
    float x;
    float y;
    float z;

    Quaternion(): w(1), x(0), y(0), z(0) {}

    Quaternion(float p_w, float p_x, float p_y, float p_z): w(p_w), x(p_x), y(p_y), z(p_z) {}

    Quaternion(const Quaternion &other):
        w(other.w),
        x(other.x),
        y(other.y),
        z(other.z)
    { }

    Quaternion(float degrees_x, float degrees_y, float degrees_z) {
        set_from_euler(degrees_x, degrees_y, degrees_z);
    }

    Quaternion(Vector3 euler_degrees) {
        set_from_euler(euler_degrees.x, euler_degrees.y, euler_degrees.z);
    }

    inline bool operator==(const Quaternion other) const {
        return w == other.w
            && x == other.x
            && y == other.y
            && z == other.z;
    }

    inline bool operator!=(const Quaternion other) const {
        return !((*this) == other);
    }

    inline Quaternion &operator=(const Quaternion other) {
        w = other.w;
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    inline Quaternion operator+(const Quaternion other) const {
        return Quaternion(
            w + other.w,
            x + other.x,
            y + other.y,
            z + other.z
        );
    }

    inline Quaternion operator*(float s) const {
        return Quaternion(w * s, x * s, y * s, z * s);
    }

    inline Quaternion operator*(const Quaternion other) const {

        Quaternion res;

        const Quaternion & lhs = *this;
        const Quaternion & rhs = other;

        // Irrlicht

        //res.m_w = (other.m_w * m_w) - (other.m_x * m_x) - (other.m_y * m_y) - (other.m_z * m_z);
        //res.m_x = (other.m_w * m_x) + (other.m_x * m_w) + (other.m_y * m_z) - (other.m_z * m_y);
        //res.m_y = (other.m_w * m_y) + (other.m_y * m_w) + (other.m_z * m_x) - (other.m_x * m_z);
        //res.m_z = (other.m_w * m_z) + (other.m_z * m_w) + (other.m_x * m_y) - (other.m_y * m_x);

        // http://www.cprogramming.com/tutorial/3d/quaternions.html

        res.w = (lhs.w * rhs.w) - (lhs.x * rhs.x) - (lhs.y * rhs.y) - (lhs.z * rhs.z);
        res.x = (lhs.w * rhs.x) + (lhs.x * rhs.w) + (lhs.y * rhs.z) - (lhs.z * rhs.y);
        res.y = (lhs.w * rhs.y) - (lhs.x * rhs.z) + (lhs.y * rhs.w) + (lhs.z * rhs.x);
        res.z = (lhs.w * rhs.z) + (lhs.x * rhs.y) - (lhs.y * rhs.x) + (lhs.z * rhs.w);

        return res;
    }

    inline Quaternion & operator*=(float s) {
        w *= s;
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    inline Vector3 operator*(const Vector3 & v) const {
        // nVidia SDK implementation

        Vector3 uv, uuv;
        Vector3 qvec(x, y, z);
        uv = qvec.cross(v);
        uuv = qvec.cross(uv);
        uv *= (2.0f * w);
        uuv *= 2.0f;

        return v + uv + uuv;
    }

    inline Quaternion & operator*=(const Quaternion & other) {
        return (*this = other * (*this));
    }

    inline void load_identity() {
        w = 1;
        x = 0;
        y = 0;
        z = 0;
    }

    inline Quaternion get_inverse() {
        return Quaternion(w, -x, -y, -z);
    }

    inline void invert() {
        x = -x;
        y = -y;
        z = -z;
    }

    inline Quaternion & normalize() {
        const float n = x * x + y * y + z * z + w * w;

        if (n == 1)
            return *this;

        //n = 1.0f / sqrtf(n);
        return (*this *= (1.f / sqrtf(n)));
    }

    inline float dot_product(const Quaternion other) const {
        return (x * other.x)
             + (y * other.y)
             + (z * other.z)
             + (w * other.w);
    }

    inline void set_from_euler(float degrees_x, float degrees_y, float degrees_z) {

        double angle;

        angle = (degrees_x * Math::DEG2RAD) * 0.5;
        const double sr = sin(angle);
        const double cr = cos(angle);

        angle = (degrees_y * Math::DEG2RAD) * 0.5;
        const double sp = sin(angle);
        const double cp = cos(angle);

        angle = (degrees_z * Math::DEG2RAD) * 0.5;
        const double sy = sin(angle);
        const double cy = cos(angle);

        const double cpcy = cp * cy;
        const double spcy = sp * cy;
        const double cpsy = cp * sy;
        const double spsy = sp * sy;

        x = (float)(sr * cpcy - cr * spsy);
        y = (float)(cr * spcy + sr * cpsy);
        z = (float)(cr * cpsy - sr * spcy);
        w = (float)(cr * cpcy + sr * spsy);

        normalize();
    }

    inline Vector3 get_euler_angles() const {

        const double sqw = w * w;
        const double sqx = x * x;
        const double sqy = y * y;
        const double sqz = z * z;
        const double test = 2.0 * (y * w - x * z);

        Vector3 euler;

        if (Math::equals(test, 1.0, 0.000001)) {

            // heading = rotation about z-axis
            euler.z = (float)(-2.0 * atan2(x, w));
            // bank = rotation about x-axis
            euler.x = 0;
            // attitude = rotation about y-axis
            euler.y = (float)(Math::PI64 / 2.0);

        } else if (Math::equals(test, -1.0, 0.000001)) {

            // heading = rotation about z-axis
            euler.z = (float)(2.0 * atan2(x, w));
            // bank = rotation about x-axis
            euler.x = 0;
            // attitude = rotation about y-axis
            euler.y = (float)(Math::PI64 / -2.0);

        } else {

            // heading = rotation about z-axis
            euler.z = (float)atan2(2.0 * (x * y + z * w), (sqx - sqy - sqz + sqw));
            // bank = rotation about x-axis
            euler.x = (float)atan2(2.0 * (y * z + x * w), (-sqx - sqy + sqz + sqw));
            // attitude = rotation about y-axis
            euler.y = (float)asin(Math::clamp(test, -1.0, 1.0));
        }

        euler.x *= Math::RAD2DEG;
        euler.y *= Math::RAD2DEG;
        euler.z *= Math::RAD2DEG;

        return euler;
    }

    static inline Quaternion lerp(const Quaternion q1, const Quaternion q2, float t) {
        const float scale = 1.0f - t;
        return (q1*scale) + (q2*t);
    }

    static inline Quaternion slerp(Quaternion q1, const Quaternion q2, float t, float threshold) {
        float angle = q1.dot_product(q2);

        // make sure we use the short rotation
        if (angle < 0.0f) {
            q1 *= -1.0f;
            angle *= -1.0f;
        }

        if (angle <= (1 - threshold)) // spherical interpolation
        {
            const float theta = acosf(angle);
            const float invsintheta = 1.0f / (sinf(theta));
            const float scale = sinf(theta * (1.0f - t)) * invsintheta;
            const float invscale = sinf(theta * t) * invsintheta;
            return (q1*scale) + (q2*invscale);
        }
        else // linear interploation
            return lerp(q1, q2, t);
    }

};

#endif // HEADER_QUATERNION_H
