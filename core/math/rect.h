#ifndef HEADER_RECT
#define HEADER_RECT

#include "vector2.h"

struct Rect {
	
	Vector2 position;
	Vector2 size;

	Rect() { }

	Rect(Vector2 p_position, Vector2 p_size): position(p_position), size(p_size) { }

	Rect(float x, float y, float w, float h): position(x, y), size(w, h) { }

	static Rect from_min_max(Vector2 min, Vector2 max) {
		return Rect(min, max - min);
	}

	bool contains(float x, float y) {
		return x >= position.x
			&& y >= position.y
			&& x < position.x + size.x
			&& y < position.y + size.y;
	}

	bool contains(Vector2 p) {
		return contains(p.x, p.y);
	}
};

#endif


