#include "window.h"

int g_window_count = 0;

Window::Window(Vector2i size, const char *title) {

    if(g_window_count == 0) {
        glfwInit();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
    _size = size;

    ++g_window_count;
}

Window::~Window() {
    glfwDestroyWindow(_window);
    --g_window_count;

    if(g_window_count == 0) {
        glfwTerminate();
    }
}

bool Window::should_close() const {
    return glfwWindowShouldClose(_window);
}

void Window::poll_events() {
    glfwPollEvents();
}

const char **Window::get_required_vulkan_extensions(uint32_t &out_extension_count) {
    return glfwGetRequiredInstanceExtensions(&out_extension_count);
}
