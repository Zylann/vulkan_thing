#ifndef HEADER_WINDOW_H
#define HEADER_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//#include <vulkan/vulkan.h>
#include "core/math/vector2.h"
#include "core/vector.h"

struct InputEvent {

    enum Type {
        FRAMEBUFFER_RESIZED
    };

    Type type;
    Vector2i size;
};

class Window {
public:

    Window(Vector2i size, const char *title);
    ~Window();

    VkResult create_vulkan_surface(VkInstance vulkan_instance, const VkAllocationCallbacks *allocator, VkSurfaceKHR *out_surface) const;

    bool should_close() const;

    static void poll_events();
    static void get_required_vulkan_extensions(Vector<const char*> &out_required_extensions);

    bool pop_event(InputEvent &out_event);
    void push_event(InputEvent event);

    Vector2i get_client_size() const;
    Vector2i get_framebuffer_size() const;

private:
    GLFWwindow* _window;
    Vector2i _size;

    Vector<InputEvent> _events;
};

#endif // HEADER_WINDOW_H
