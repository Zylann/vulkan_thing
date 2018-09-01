#ifndef HEADER_FOV_H
#define HEADER_FOV_H

#include "math_funcs.h"

/// \brief Field of view defined by four half-angle tangent values
struct Fov {

    /// \brief Tangent of the angle between the left edge of the view and the forward vector
    float tan_left;
    /// \brief Tangent of the angle between the right edge of the view and the forward vector
    float tan_right;
    /// \brief Tangent of the angle between the top edge of the view and the forward vector
    float tan_up;
    /// \brief Tangent of the angle between the bottom edge of the view and the forward vector
    float tan_down;

    /// \brief Creates a default Fov with 90 degrees of horizontal and vertical field
    Fov() :
        tan_left(1),
        tan_right(1),
        tan_up(1),
        tan_down(1)
    {}

    /// \brief Creates a Fov from raw tangent values
    Fov(float _tan_left, float _tan_right, float _tan_up, float _tan_down) :
        tan_left(_tan_left),
        tan_right(_tan_right),
        tan_up(_tan_up),
        tan_down(_tan_down)
    {}

    /// \brief Creates a Fov from four half-angles in degrees
    static Fov from_degrees(float degrees_left, float degrees_right, float degrees_up, float degrees_down) {
        return {
            tan(degrees_left * Math::DEG2RAD),
            tan(degrees_right * Math::DEG2RAD),
            tan(degrees_up * Math::DEG2RAD),
            tan(degrees_down * Math::DEG2RAD)
        };
    }

    /// \brief Creates a Fov from a full vertical angle and an aspect ratio.
    static Fov from_degrees_ratio(float degrees, float aspect_ratio) {
        float t = tan(0.5f * degrees * Math::DEG2RAD);
        return {
            // Horizontal
            t * aspect_ratio,
            t * aspect_ratio,
            // Vertical
            t,
            t
        };
    }
};

#endif // HEADER_FOV_H
