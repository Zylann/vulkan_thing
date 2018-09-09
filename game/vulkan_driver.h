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

    bool draw();

    void wait();

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
    Vector<VkFramebuffer> _swap_chain_framebuffers;
    VkFormat _swap_chain_image_format;
    VkExtent2D _swap_chain_extent;

    VkRenderPass _render_pass;
    VkPipelineLayout _pipeline_layout;
    VkPipeline _graphics_pipeline;

    VkCommandPool _command_pool;
    Vector<VkCommandBuffer> _command_buffers;

    VkSemaphore _image_available_semaphore;
    VkSemaphore _render_finished_semaphore;
};

#endif // HEADER_VULKAN_DRIVER_H
