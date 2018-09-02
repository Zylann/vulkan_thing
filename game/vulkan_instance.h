#ifndef HEADER_VULKAN_INSTANCE_H
#define HEADER_VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>
#include "core/vector.h"

class VulkanInstance {
public:
    VulkanInstance();
    ~VulkanInstance();

    bool create(const char *app_name,
        Vector<const char *> required_extensions,
        Vector<const char *> required_layers);

private:
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
};

#endif // HEADER_VULKAN_INSTANCE_H
