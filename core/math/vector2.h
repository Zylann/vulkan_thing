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

#endif // HEADER_VECTOR2_H
