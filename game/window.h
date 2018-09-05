#ifndef HEADER_WINDOW_H
#define HEADER_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//#include <vulkan/vulkan.h>
#include "core/math/vector2.h"
#include "core/vector.h"

class Window {
public:

    Window(Vector2i size, const char *title);
    ~Window();

    VkResult create_vulkan_surface(VkInstance vulkan_instance, const VkAllocationCallbacks *allocator, VkSurfaceKHR *out_surface) const;

    bool should_close() const;

    static void poll_events();
    static void get_required_vulkan_extensions(Vector<const char*> &out_required_extensions);

    Vector2i get_size() const { return _size; }

private:
    GLFWwindow* _window;
    Vector2i _size;
};

#endif // HEADER_WINDOW_H
