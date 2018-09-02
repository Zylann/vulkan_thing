#ifndef HEADER_BOX_H
#define HEADER_BOX_H

#include "vector3.h"

struct Box {

    Vector3 position;
    Vector3 size;

    Box() { }

    Box(Vector3 p_position, Vector3 p_size): position(p_position), size(p_size) { }

    Box(float x, float y, float z, float w, float h, float d): position(x, y, z), size(w, h, d) { }

    static Box from_min_max(Vector3 min, Vector3 max) {
        return Rect(min, max - min);
    }

    bool contains(float x, float y, float z) {
        return x >= position.x
            && y >= position.y
            && z >= position.z
            && x < position.x + size.x
            && y < position.y + size.y
            && z < position.z + size.z;
    }

    bool contains(Vector3 p) {
        return contains(p.x, p.y, p.z);
    }
};

#endif // HEADER_BOX_H
