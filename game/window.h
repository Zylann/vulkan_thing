#ifndef HEADER_WINDOW_H
#define HEADER_WINDOW_H

#include <GLFW/glfw3.h>
#include "core/math/vector2.h"
#include "core/vector.h"

class Window {
public:

    Window(Vector2i size, const char *title);
    ~Window();

    bool should_close() const;

    static void poll_events();
    static void get_required_vulkan_extensions(Vector<const char*> &out_required_extensions);

    Vector2i get_size() const { return _size; }

private:
    GLFWwindow* _window;
    Vector2i _size;
};

#endif // HEADER_WINDOW_H
