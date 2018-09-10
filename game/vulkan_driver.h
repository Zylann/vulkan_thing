#ifndef HEADER_VULKAN_DRIVER_H
#define HEADER_VULKAN_DRIVER_H

#include <vulkan/vulkan.h>
#include "core/vector.h"
#include "core/math/vector2.h"

class Window;

class VulkanDriver {
public:
    VulkanDriver();
    ~VulkanDriver();

    bool create(const char *app_name,
        Vector<const char *> required_extensions,
        Vector<const char *> required_layers,
        Window &window);

    bool draw(const Window &window);
    void schedule_resize();

    void wait();

private:
    bool resize(const Window &window);
    bool create_view(const Window &window);

    void clear_swap_chain();

    struct SwapChainSupportDetails  {
        VkSurfaceCapabilitiesKHR capabilities = {};
        Vector<VkSurfaceFormatKHR> formats;
        Vector<VkPresentModeKHR> modes;
    };

    void query_swap_chain_details(VkPhysicalDevice device, VkSurfaceKHR surface, SwapChainSupportDetails & out_details) const;

    bool create_swap_chain(const Window &window);
    bool create_render_pass();
    bool create_pipeline();
    bool create_framebuffers();
    bool create_command_buffers();

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _physical_device;
    VkDevice _device;
    VkQueue _graphics_queue;
    VkQueue _present_queue;

    struct QueueFamilyIndices {
        int graphics = -1;
        int presentation = -1;

        bool is_complete() const {
            return graphics != -1 && presentation != -1;
        }
    };

    QueueFamilyIndices _queue_family_indices;

    SwapChainSupportDetails _swap_chain_support_details;

    VkSurfaceKHR _surface;

    VkSwapchainKHR _swap_chain;
    Vector<VkImage> _swap_chain_images;
    Vector<VkImageView> _swap_chain_image_views;
    Vector<VkFramebuffer> _swap_chain_framebuffers;
    VkFormat _swap_chain_image_format;
    VkExtent2D _swap_chain_extent;
    bool _scheduled_resize;

    VkRenderPass _render_pass;
    VkPipelineLayout _pipeline_layout;
    VkPipeline _graphics_pipeline;

    VkCommandPool _command_pool;
    Vector<VkCommandBuffer> _command_buffers;

    // One for each in-flight image
    Vector<VkSemaphore> _image_available_semaphores;
    Vector<VkSemaphore> _render_finished_semaphores;
    Vector<VkFence> _in_flight_fences;
    uint32_t _current_frame;
};

#endif // HEADER_VULKAN_DRIVER_H


