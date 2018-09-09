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

    VkSwapchainKHR _swap_chain;
    Vector<VkImage> _swap_chain_images;
    Vector<VkImageView> _swap_chain_image_views;
    VkFormat _swap_chain_image_format;
    VkExtent2D _swap_chain_extent;

    VkPipelineLayout _pipeline_layout;
};

#endif // HEADER_VULKAN_DRIVER_H
