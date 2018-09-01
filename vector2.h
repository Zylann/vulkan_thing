#ifndef HEADER_VECTOR2_H
#define HEADER_VECTOR2_H

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

#endif // HEADER_VECTOR2_H
