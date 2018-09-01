#ifndef HEADER_MATH_FUNCS_H
#define HEADER_MATH_FUNCS_H

#include <cmath>
#include "../types.h"

namespace Math {

const float ROUNDING_ERROR_F32 = 0.000001f; // Float epsilon
const double ROUNDING_ERROR_F64 = 0.00000001; // Double epsilon

/// Constant for PI.
const float PI = 3.14159265359f;

#ifdef PI64 // make sure we don't collide with a define
#undef PI64
#endif

/// Constant for 64bit PI.
const double PI64 = 3.1415926535897932384626433832795028841971693993751;

/// 32bit Constant for converting from degrees to radians
const float DEG2RAD = PI / 180.0f;

/// 32bit constant for converting from radians to degrees
const float RAD2DEG = 180.0f / PI;

/// 64bit constant for converting from degrees to radians
const double DEG2RAD64 = PI64 / 180.0;

/// 64bit constant for converting from radians to degrees
const double RAD2DEG64 = 180.0 / PI64;

inline float absf(float g) {

    union {
        float f;
        uint32_t i;
    } u;

    u.f = g;
    u.i &= 2147483647u;
    return u.f;
}

inline double absd(double g) {

    union {
        double d;
        uint64_t i;
    } u;

    u.d = g;
    u.i &= (uint64_t)9223372036854775807ll;
    return u.d;
}

inline int absi(int x) {
    return x < 0 ? -x : x;
}

inline float sqrt(float x) { return sqrtf(x); }
inline float sqrt(double x) { return sqrt(x); }

/// returns if a equals b, taking possible rounding errors into account
inline bool equals(const double a, const double b, const double tolerance = ROUNDING_ERROR_F64) {
    return (a + tolerance >= b) && (a - tolerance <= b);
}

/// returns if a equals b, taking possible rounding errors into account
inline bool equals(const float a, const float b, const float tolerance = ROUNDING_ERROR_F32) {
    return (a + tolerance >= b) && (a - tolerance <= b);
}

/// returns if a equals zero, taking rounding errors into account
inline bool is_zero(const double a, const double tolerance = ROUNDING_ERROR_F64) {
    return absd(a) <= tolerance;
}

/// returns if a equals zero, taking rounding errors into account
inline bool is_zero(const float a, const float tolerance = ROUNDING_ERROR_F32) {
    return absf(a) <= tolerance;
}

/// \brief Clamps x to the given interval. If x is greater or lesser than
/// min or max, it will be backed to min or max.
/// \param x : value to clamp.
/// \param min
/// \param max
/// \return clamped value
template<class T>
inline const T & clamp(const T & x, const T & min, const T & max) {
    if(x > max)
        return max;
    else if(x < min)
        return min;
    return x;
}

} // namespace Math

#endif // HEADER_MATH_FUNCS_H
