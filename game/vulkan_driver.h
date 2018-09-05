#ifndef HEADER_VULKAN_DRIVER_H
#define HEADER_VULKAN_DRIVER_H

#include <vulkan/vulkan.h>
#include "core/vector.h"

class Window;

class VulkanDriver {
public:
    VulkanDriver();
    ~VulkanDriver();

    bool create(const char *app_name,
        Vector<const char *> required_extensions,
        Vector<const char *> required_layers,
        Window &window);

private:
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkDevice _device;
    VkQueue _graphics_queue;
    VkQueue _present_queue;
    VkSurfaceKHR _surface;
};

#endif // HEADER_VULKAN_DRIVER_H
