#include "window.h"

int g_window_count = 0;

static void cb_framebuffer_resized(GLFWwindow* glfw_window, int width, int height) {

    InputEvent event;
    event.type = InputEvent::FRAMEBUFFER_RESIZED;
    event.size.x = width;
    event.size.y = height;

    Window * window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    window->push_event(event);
}

Window::Window(Vector2i size, const char *title) {

    if(g_window_count == 0) {
        glfwInit();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(size.x, size.y, title, nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);

    glfwSetFramebufferSizeCallback(_window, cb_framebuffer_resized);

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

bool Window::pop_event(InputEvent &out_event) {
    if (_events.size() != 0) {
        out_event = _events.back();
        _events.pop_back();
        return true;
    } else {
        return false;
    }
}

void Window::push_event(InputEvent event) {
    _events.push_front(event);
}

void Window::get_required_vulkan_extensions(Vector<const char*> &out_required_extensions) {

    uint32_t count = 0;
    const char ** exts = glfwGetRequiredInstanceExtensions(&count);

    for (uint32_t i = 0; i < count; ++i) {
        out_required_extensions.push_back(exts[i]);
    }
}

Vector2i Window::get_client_size() const {
    Vector2i size;
    glfwGetWindowSize(_window, &size.x, &size.y);
    return size;
}

Vector2i Window::get_framebuffer_size() const {
    Vector2i size;
    glfwGetFramebufferSize(_window, &size.x, &size.y);
    return size;
}



