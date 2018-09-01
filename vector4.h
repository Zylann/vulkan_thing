#ifndef HEADER_VECTOR4_H
#define HEADER_VECTOR4_H

struct Vector4 {

    float x;
    float y;
    float z;
    float w;

    Vector4(): x(0), y(0), z(0), w(0) { }

    Vector4(float p_x, float p_y, float p_z, float p_w): x(p_x), y(p_y), z(p_z), w(p_w) { }

};

#endif // HEADER_VECTOR4_H
