#ifndef HEADER_VECTOR2_H
#define HEADER_VECTOR2_H

#include "../string.h"

struct Vector2 {

    union {
        struct {
            float x;
            float y;
        };

        float coord[2];
    };

    Vector2(): x(0), y(0) { }
};

struct Vector2i {

    union {
        struct {
            int x;
            int y;
        };

        int coord[2];
    };

    Vector2i(): x(0), y(0) { }
    Vector2i(int p_x, int p_y): x(p_x), y(p_y) { }
};

inline bool operator!=(Vector2i a, Vector2i b) {
    return a.x != b.x || a.y != b.y;
}

inline void to_string(String &dst, Vector2i v) {
    to_string(dst, L'(');
    to_string(dst, v.x);
    to_string(dst, L', ');
    to_string(dst, v.y);
    to_string(dst, L')');
}

#endif // HEADER_VECTOR2_H
