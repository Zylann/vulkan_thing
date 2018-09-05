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

VkResult Window::create_vulkan_surface(VkInstance vulkan_instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR *out_surface) const {
    return glfwCreateWindowSurface(vulkan_instance, _window, allocator, out_surface);
}

bool Window::should_close() const {
    return glfwWindowShouldClose(_window);
}

void Window::poll_events() {
    glfwPollEvents();
}

void Window::get_required_vulkan_extensions(Vector<const char*> &out_required_extensions) {

    uint32_t count = 0;
    const char ** exts = glfwGetRequiredInstanceExtensions(&count);

    for (uint32_t i = 0; i < count; ++i) {
        out_required_extensions.push_back(exts[i]);
    }
}
