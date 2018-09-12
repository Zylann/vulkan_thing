#include "vulkan_driver.h"
#include "core/macros.h"
#include "core/file.h"
#include "window.h"
#include "mesh.h"

// How many frames can be processed concurrently
const int MAX_FRAMES_IN_FLIGHT = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data) {

    String msg = L"Vulkan: ";

    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        msg += L"General: ";
    }
    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        msg += L"Performance: ";
    }
    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        msg += L"Validation: ";
    }

    msg += callback_data->pMessage;

    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        Log::error(msg);

    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        Log::warning(msg);

    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        Log::info(msg);

    } else {
        Log::info(msg);
    }

    return VK_FALSE;
}

static bool contains_all_extensions(const Vector<VkExtensionProperties> &extensions, const Vector<const char*> &expected_names, bool log_error=false) {
    for(int j = 0; j < expected_names.size(); ++j) {
        bool found = false;
        for(int i = 0; i < extensions.size() && !found; ++i) {
            found |= strcmp(extensions[i].extensionName, expected_names[j]) == 0;
        }
        if(!found) {
            if(log_error)
                Log::error("Required Vulkan extension was not found: ", expected_names[j]);
            return false;
        }
    }
    return true;
}

static VkSemaphore create_semaphore(VkDevice device) {
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    CHECK_RESULT_V(vkCreateSemaphore(device, &create_info, nullptr, &semaphore), VK_NULL_HANDLE);
    return semaphore;
}

static VkFence create_fence(VkDevice device, bool signaled) {
    VkFence fence = VK_NULL_HANDLE;

    VkFenceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled)
        create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    CHECK_RESULT_V(vkCreateFence(device, &create_info, nullptr, &fence), VK_NULL_HANDLE);
    return fence;
}

VulkanDriver::VulkanDriver() {

    _instance = VK_NULL_HANDLE;
    _debug_messenger = VK_NULL_HANDLE;
    _physical_device = VK_NULL_HANDLE;
    _device = VK_NULL_HANDLE;
    _graphics_queue = VK_NULL_HANDLE;
    _surface = VK_NULL_HANDLE;

    _swap_chain = VK_NULL_HANDLE;
    _swap_chain_image_format = {};
    _swap_chain_extent = {};

    _render_pass = VK_NULL_HANDLE;
    _pipeline_layout = VK_NULL_HANDLE;
    _graphics_pipeline = VK_NULL_HANDLE;

    _command_pool = VK_NULL_HANDLE;
    _short_lived_command_pool = VK_NULL_HANDLE;

    _current_frame = 0;

    _scheduled_resize = false;
}

VulkanDriver::~VulkanDriver() {
    if(_instance) {

        wait();

        for(int i = 0; i < scene.size(); ++i) {
            delete scene[i];
        }
        scene.clear();

        clear_swap_chain();

        if(_command_pool) {
            vkDestroyCommandPool(_device, _command_pool, nullptr);
        }
        if(_short_lived_command_pool) {
            vkDestroyCommandPool(_device, _short_lived_command_pool, nullptr);
        }

        for (int i = 0; i < _render_finished_semaphores.size(); ++i) {
            if(_render_finished_semaphores[i]) {
                vkDestroySemaphore(_device, _render_finished_semaphores[i], nullptr);
            }
        }
        for (int i = 0; i < _image_available_semaphores.size(); ++i) {
            if(_image_available_semaphores[i]) {
                vkDestroySemaphore(_device, _image_available_semaphores[i], nullptr);
            }
        }
        for (int i = 0; i < _in_flight_fences.size(); ++i) {
            if(_in_flight_fences[i]) {
                vkDestroyFence(_device, _in_flight_fences[i], nullptr);
            }
        }

        if(_device) {
            vkDestroyDevice(_device, nullptr);
        }

        if(_surface) {
            vkDestroySurfaceKHR(_instance, _surface, nullptr);
        }

        if(_debug_messenger) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(_instance, _debug_messenger, nullptr);
            }
        }

        vkDestroyInstance(_instance, nullptr);
    }
}

void VulkanDriver::schedule_resize() {
    _scheduled_resize = true;
}

void VulkanDriver::query_swap_chain_details(VkPhysicalDevice device, VkSurfaceKHR surface, SwapChainSupportDetails &out_details) const {

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &out_details.capabilities);

    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats_count, nullptr);
    out_details.formats.resize_no_init(formats_count);
    if(formats_count != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats_count, out_details.formats.data());
    }

    uint32_t modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes_count, nullptr);
    out_details.modes.resize_no_init(modes_count);
    if(modes_count != 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modes_count, out_details.modes.data());
    }
}

bool VulkanDriver::create(const char *app_name,
    Vector<const char*> required_extensions,
    Vector<const char*> required_layers,
    Window &window) {

    assert(_instance == VK_NULL_HANDLE);

#if DEBUG
    // Check validation layers
    Console::print_line("Adding Vulkan validation layers");

    required_layers.push_back("VK_LAYER_LUNARG_standard_validation");

    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    Console::print_line("Required Vulkan extensions:");
    for(int i = 0; i < required_extensions.size(); ++i) {
        Console::print_line("\t", required_extensions[i]);
    }
    Console::print_line();
    Console::print_line("Required Vulkan layers:");
    for(int i = 0; i < required_layers.size(); ++i) {
        Console::print_line("\t", required_layers[i]);
    }
    Console::print_line();

    // List extensions
    {
        uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        Vector<VkExtensionProperties> extensions;
        extensions.resize_no_init(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

        Console::print_line("Available Vulkan extensions:");
        for (int i = 0; i < extensions.size(); ++i) {
            Console::print_line(L"\t", extensions[i].extensionName);
        }
        Console::print_line();

        ERR_FAIL_COND_V(!contains_all_extensions(extensions, required_extensions, true), false);
    }

    // List layers
    if (required_layers.size() > 0) {

        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        Vector<VkLayerProperties> available_layers;
        available_layers.resize_no_init(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        Console::print_line("Available Vulkan layers:");
        for (int i = 0; i < available_layers.size(); ++i) {
            Console::print_line(L"\t", available_layers[i].layerName);
        }
        Console::print_line();

        for (int i = 0; i < required_layers.size(); ++i) {

            bool found = false;
            for (int j = 0; j < available_layers.size() && !found; ++j) {
                found |= strcmp(required_layers[i], available_layers[j].layerName) == 0;
            }

            if (!found) {
                Log::error(L"Required Vulkan layer is not available: ", required_layers[i]);
                return false;
            }
        }
    }

    // Create instance
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = app_name;
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = required_extensions.size();
        create_info.ppEnabledExtensionNames = required_extensions.data();
        create_info.enabledLayerCount = required_layers.size();
        create_info.ppEnabledLayerNames = required_layers.is_empty() ? nullptr : required_layers.data();

        CHECK_RESULT_V(vkCreateInstance(&create_info, nullptr, &_instance), false);
    }

#if DEBUG
    // Setup debug callback
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity =
                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType =
                  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = vulkan_debug_callback;
        create_info.pUserData = nullptr; // Optional

        VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            result = func(_instance, &create_info, nullptr, &_debug_messenger);
        } else {
            Log::error("Could not get function address");
        }

        if (result != VK_SUCCESS) {
            Log::error(L"Failed to create Vulkan debug callback: result ", result);
            return false;
        }
    }
#endif

    // Create main surface
    CHECK_RESULT_V(window.create_vulkan_surface(_instance, nullptr, &_surface), false);

    // Pick physical device
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    Vector<const char*> required_device_extensions;
    required_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    {
        // Enumerate physical devices
        uint32_t physical_devices_count = 0;
        vkEnumeratePhysicalDevices(_instance, &physical_devices_count, nullptr);
        if (physical_devices_count == 0) {
            Log::error("No Vulkan physical devices found");
            return false;
        }
        Vector<VkPhysicalDevice> physical_devices;
        physical_devices.resize_no_init(physical_devices_count);
        vkEnumeratePhysicalDevices(_instance, &physical_devices_count, physical_devices.data());

        Log::info(L"Found ", (int64_t)physical_devices_count, L" Vulkan physical devices");

        // Select device
        for (int i = 0; i < physical_devices.size(); ++i) {

            VkPhysicalDevice device = physical_devices[i];

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);

            // Find queue families

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
            Vector<VkQueueFamilyProperties> queue_families;
            queue_families.resize_no_init(queue_family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

            QueueFamilyIndices indices;

            for(int j = 0; j < queue_families.size(); ++j) {
                const VkQueueFamilyProperties &family = queue_families[j];

                // Graphics support
                if (family.queueCount > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphics = j;
                }

                // Presentation support
                VkBool32 present_support = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, j, _surface, &present_support);
                if(present_support) {
                    indices.presentation = j;
                }

                if (indices.is_complete())
                    break;
            }

            if (!indices.is_complete()) {
                // This device doesn't have all the queue families we need
                continue;
            }

            // Check device extensions
            {
                uint32_t device_extensions_count = 0;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &device_extensions_count, nullptr);
                Vector<VkExtensionProperties> device_extensions;
                device_extensions.resize_no_init(device_extensions_count);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &device_extensions_count, device_extensions.data());

                if (!contains_all_extensions(device_extensions, required_device_extensions)) {
                    // This device doesn't have all extensions we need
                    continue;
                }
            }

            // Check swap chain support
            SwapChainSupportDetails details;
            query_swap_chain_details(device, _surface, details);

            if (details.formats.size() == 0 || details.modes.size() == 0)
                continue;

            // Pick that device
            _queue_family_indices = indices;
            _swap_chain_support_details = details;
            _physical_device = physical_devices[i];
            break;
        }

        if (!_physical_device) {
            Log::error("No suitable Vulkan physical device");
            return false;
        }
    }

    // Create logical device
    {
        Vector<int> unique_queue_indices;
        unique_queue_indices.push_back(_queue_family_indices.graphics);
        if(!unique_queue_indices.contains(_queue_family_indices.presentation))
            unique_queue_indices.push_back(_queue_family_indices.presentation);

        float queue_priority = 1.0f;
        Vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for(int i = 0; i < unique_queue_indices.size(); ++i) {
            int queue_index = unique_queue_indices[i];

            VkDeviceQueueCreateInfo queue_create_info = {};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_index;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledLayerCount = required_layers.size();
        create_info.ppEnabledLayerNames = required_layers.data();
        create_info.enabledExtensionCount = static_cast<uint32_t>(required_device_extensions.size());
        create_info.ppEnabledExtensionNames = required_device_extensions.data();

        CHECK_RESULT_V(vkCreateDevice(_physical_device, &create_info, nullptr, &_device), false);
    }

    vkGetDeviceQueue(_device, _queue_family_indices.graphics, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, _queue_family_indices.presentation, 0, &_present_queue);

    ERR_FAIL_COND_V(!create_view(window), false);

    // Synchronization

    _image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    _render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    _in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        _image_available_semaphores[i] = create_semaphore(_device);
        _render_finished_semaphores[i] = create_semaphore(_device);

        // Note: we create those fences in a signaled state to avoid a deadlock when rendering our first frame,
        // because they are created in unsignaled state by default
        _in_flight_fences[i] = create_fence(_device, true);

        if(_image_available_semaphores[i] == VK_NULL_HANDLE || _render_finished_semaphores[i] == VK_NULL_HANDLE || _in_flight_fences[i] == VK_NULL_HANDLE)
            return false;
    }

    return true;
}

bool VulkanDriver::resize(const Window &window) {

    vkDeviceWaitIdle(_device);

    clear_swap_chain();

    // TODO Optimize resizing
    // It is possible to create a new swap chain while drawing commands on an image from the old swap chain are still in-flight.
    // You need to pass the previous swap chain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct
    // and destroy the old swap chain as soon as you've finished using it.

    _scheduled_resize = false;

    ERR_FAIL_COND_V(!create_view(window), false);

    return create_command_buffers();
}

void VulkanDriver::clear_swap_chain() {
    // Clear the swap chain and everything depending on it

    for(int i = 0; i < _swap_chain_framebuffers.size(); ++i) {
        VkFramebuffer fb = _swap_chain_framebuffers[i];
        if(fb) {
            vkDestroyFramebuffer(_device, fb, nullptr);
        }
    }
    _swap_chain_framebuffers.clear();

    if(_command_buffers.size() != 0) {
        vkFreeCommandBuffers(_device, _command_pool, static_cast<uint32_t>(_command_buffers.size()), _command_buffers.data());
        _command_buffers.clear();
    }

    if(_graphics_pipeline) {
        vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
        _graphics_pipeline = VK_NULL_HANDLE;
    }

    if(_pipeline_layout) {
        vkDestroyPipelineLayout(_device, _pipeline_layout, nullptr);
        _pipeline_layout = VK_NULL_HANDLE;
    }

    if (_render_pass) {
        vkDestroyRenderPass(_device, _render_pass, nullptr);
        _render_pass = VK_NULL_HANDLE;
    }

    for (int i = 0; i < _swap_chain_image_views.size(); ++i) {
        VkImageView view = _swap_chain_image_views[i];
        if(view) {
            vkDestroyImageView(_device, view, nullptr);
        }
    }
    _swap_chain_image_views.clear();

    if(_swap_chain) {
        vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
        _swap_chain = VK_NULL_HANDLE;
    }
}


bool VulkanDriver::create_swap_chain(const Window &window) {

    assert(_swap_chain == VK_NULL_HANDLE);

    // TODO Do we really need to query this again?
    query_swap_chain_details(_physical_device, _surface, _swap_chain_support_details);
    const SwapChainSupportDetails &support_details = _swap_chain_support_details;

    // Format
    VkSurfaceFormatKHR surface_format = {};
    if (support_details.formats.size() == 1 && support_details.formats[0].format == VK_FORMAT_UNDEFINED) {
        surface_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    } else {
        for(int i = 0; i < support_details.formats.size(); ++i){
            VkSurfaceFormatKHR f = support_details.formats[i];
            if(f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surface_format = f;
                break;
            }
        }
    }
    if(surface_format.format == VK_FORMAT_UNDEFINED) {
        Log::warning("Falling back on first found surface format");
        surface_format = support_details.formats[0];
    }

    // Presentation mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < support_details.modes.size(); ++i) {
        VkPresentModeKHR m = support_details.modes[i];
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = m;
            break;
        } else if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            present_mode = m;
        }
    }

    // Swap extent
    VkExtent2D extent = {};
    if (support_details.capabilities.currentExtent.width != 0xffffffff) {
        extent = support_details.capabilities.currentExtent;

    } else {
        Vector2i window_size = window.get_framebuffer_size();
        const VkSurfaceCapabilitiesKHR &c = support_details.capabilities;
        extent.width = Math::clamp(static_cast<uint32_t>(window_size.x), c.minImageExtent.width, c.maxImageExtent.width);
        extent.height = Math::clamp(static_cast<uint32_t>(window_size.y), c.minImageExtent.height, c.maxImageExtent.height);
    }

    uint32_t image_count = support_details.capabilities.minImageCount + 1;
    // 0 means no limit
    if (support_details.capabilities.maxImageCount > 0 && image_count > support_details.capabilities.maxImageCount) {
        image_count = support_details.capabilities.maxImageCount;
    }

    {
        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = _surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Note: use TRANSFERT_DST if we do post-processing

        uint32_t indices[] = {(uint32_t) _queue_family_indices.graphics, (uint32_t) _queue_family_indices.presentation};
        if (_queue_family_indices.graphics != _queue_family_indices.presentation) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = indices;
        } else {
            // Preferred, more performant
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0; // Optional
            create_info.pQueueFamilyIndices = nullptr; // Optional
        }

        create_info.preTransform = support_details.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Window is not transparent
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE; // Don't care about pixels behind other windows
        create_info.oldSwapchain = VK_NULL_HANDLE;

        CHECK_RESULT_V(vkCreateSwapchainKHR(_device, &create_info, nullptr, &_swap_chain), false);
    }

    vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, nullptr);
    _swap_chain_images.resize_no_init(image_count);
    vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, _swap_chain_images.data());

    _swap_chain_image_format = surface_format.format;
    _swap_chain_extent = extent;

    // Image views
    _swap_chain_image_views.resize(_swap_chain_images.size(), VK_NULL_HANDLE);
    for(int i = 0; i < _swap_chain_image_views.size(); ++i) {

        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = _swap_chain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = _swap_chain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        CHECK_RESULT_V(vkCreateImageView(_device, &create_info, nullptr, &_swap_chain_image_views[i]), false);
    }

    return true;
}

bool VulkanDriver::create_render_pass() {

    assert(_render_pass == VK_NULL_HANDLE);

    VkAttachmentDescription color_attachment = {};
    color_attachment.format = _swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // Applies to color and depth
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // Not using stencil for now
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Layout for presentation
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    // The index of the attachment in this array is directly referenced from
    // the fragment shader with the layout(location = 0) out vec4 outColor directive!
    subpass.pColorAttachments = &color_attachment_ref;

    // We need to wait for the swap chain to finish reading from the image before we can access it.
    // This can be accomplished by waiting on the color attachment output stage itself.
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &color_attachment;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;

    CHECK_RESULT_V(vkCreateRenderPass(_device, &create_info, nullptr, &_render_pass), false);

    return true;
}

bool VulkanDriver::create_pipeline() {

    assert(_graphics_pipeline == VK_NULL_HANDLE);
    assert(_render_pass != VK_NULL_HANDLE);

    // Shader stages

    Vector<uint8_t> vert_shader_code;
    Vector<uint8_t> frag_shader_code;

    if (!File::read_all_bytes("default.vert.spv", vert_shader_code)) {
        Log::error("Failed to read vertex shader");
        return false;
    }
    if (!File::read_all_bytes("default.frag.spv", frag_shader_code)) {
        Log::error("Failed to read fragment shader");
        return false;
    }

    vert_shader_code.align(sizeof(uint32_t), 0);
    frag_shader_code.align(sizeof(uint32_t), 0);

    VkShaderModule vert_shader_module = VK_NULL_HANDLE;
    {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = vert_shader_code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(vert_shader_code.data());

        CHECK_RESULT_V(vkCreateShaderModule(_device, &create_info, nullptr, &vert_shader_module), false);
    }
    VkShaderModule frag_shader_module = VK_NULL_HANDLE;
    {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = frag_shader_code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(frag_shader_code.data());

        CHECK_RESULT_V(vkCreateShaderModule(_device, &create_info, nullptr, &frag_shader_module), false);
    }

    struct AutoDestroyShaderModule {

        VkDevice device;
        VkShaderModule shader_module;

        ~AutoDestroyShaderModule() {
            vkDestroyShaderModule(device, shader_module, nullptr);
        }
    };

    AutoDestroyShaderModule auto_destroy_vert_shader_module = { _device, vert_shader_module };
    AutoDestroyShaderModule auto_destroy_frag_shader_module = { _device, frag_shader_module };

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info,
        frag_shader_stage_info
    };

    // Fixed stages

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    Vector<VkVertexInputBindingDescription> vertex_bindings;
    Vector<VkVertexInputAttributeDescription> vertex_attributes;
    Mesh::get_description(vertex_bindings, vertex_attributes);

    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = vertex_bindings.size();
    vertex_input_info.pVertexBindingDescriptions = vertex_bindings.data();
    vertex_input_info.vertexAttributeDescriptionCount = vertex_attributes.size();
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swap_chain_extent.width;
    viewport.height = (float) _swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = _swap_chain_extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    // Alpha blending
//        color_blend_attachment.blendEnable = VK_TRUE;
//        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
//        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional

//        VkDynamicState dynamic_states[] = {
//            VK_DYNAMIC_STATE_VIEWPORT
//        };

//        VkPipelineDynamicStateCreateInfo dynamic_state = {};
//        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//        dynamic_state.dynamicStateCount = 1;
//        dynamic_state.pDynamicStates = dynamic_states;

    // Pipeline layout
    {
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = 0; // Optional
        create_info.pSetLayouts = nullptr; // Optional
        create_info.pushConstantRangeCount = 0; // Optional
        create_info.pPushConstantRanges = nullptr; // Optional

        CHECK_RESULT_V(vkCreatePipelineLayout(_device, &create_info, nullptr, &_pipeline_layout), false);
    }

    {
        VkGraphicsPipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.stageCount = 2;
        create_info.pStages = shader_stages;
        create_info.pVertexInputState = &vertex_input_info;
        create_info.pInputAssemblyState = &input_assembly;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterizer;
        create_info.pMultisampleState = &multisampling;
        create_info.pDepthStencilState = nullptr; // Optional
        create_info.pColorBlendState = &color_blending;
        create_info.pDynamicState = nullptr; // Optional
        create_info.layout = _pipeline_layout;
        create_info.renderPass = _render_pass;
        create_info.subpass = 0;
        create_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
        create_info.basePipelineIndex = -1; // Optional

        CHECK_RESULT_V(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &create_info, nullptr, &_graphics_pipeline), false);
    }

    //...

//        vkDestroyShaderModule(_device, vert_shader_module, nullptr);
//        vkDestroyShaderModule(_device, frag_shader_module, nullptr);
    return true;
}

bool VulkanDriver::create_framebuffers() {

    assert(_swap_chain_framebuffers.size() == 0);
    assert(_swap_chain_images.size() != 0);

    _swap_chain_framebuffers.resize(_swap_chain_images.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < _swap_chain_images.size(); ++i) {

        VkImageView attachments[] = {
            _swap_chain_image_views[i]
        };

        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = _render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = _swap_chain_extent.width;
        create_info.height = _swap_chain_extent.height;
        create_info.layers = 1;

        CHECK_RESULT_V(vkCreateFramebuffer(_device, &create_info, nullptr, &_swap_chain_framebuffers[i]), false);
    }

    return true;
}

bool VulkanDriver::create_command_buffers() {

    // TODO Support updating command buffers

    assert(_command_buffers.size() == 0);
    assert(_swap_chain_framebuffers.size() != 0);

    if (_command_pool == VK_NULL_HANDLE) {

        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = _queue_family_indices.graphics;
        create_info.flags = 0; // Optional

        CHECK_RESULT_V(vkCreateCommandPool(_device, &create_info, nullptr, &_command_pool), false);
    }

    {
        _command_buffers.resize(_swap_chain_framebuffers.size(), VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = _command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = (uint32_t) _command_buffers.size();

        CHECK_RESULT_V(vkAllocateCommandBuffers(_device, &alloc_info, _command_buffers.data()), false);
    }

    for (size_t i = 0; i < _command_buffers.size(); ++i) {

        VkCommandBuffer command_buffer = _command_buffers[i];

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        begin_info.pInheritanceInfo = nullptr; // Optional

        CHECK_RESULT_V(vkBeginCommandBuffer(command_buffer, &begin_info), false);

        VkRenderPassBeginInfo pass_info = {};
        pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        pass_info.renderPass = _render_pass;
        pass_info.framebuffer = _swap_chain_framebuffers[i];
        pass_info.renderArea.offset = {0, 0};
        pass_info.renderArea.extent = _swap_chain_extent;
        VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
        pass_info.clearValueCount = 1;
        pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphics_pipeline);

        for(int i = 0; i < scene.size(); ++i)
            scene[i]->draw(command_buffer);

        vkCmdEndRenderPass(command_buffer);

        CHECK_RESULT_V(vkEndCommandBuffer(command_buffer), false);
    }

    return true;
}

bool VulkanDriver::create_view(const Window &window) {

    ERR_FAIL_COND_V(!create_swap_chain(window), false);
    ERR_FAIL_COND_V(!create_render_pass(), false);
    ERR_FAIL_COND_V(!create_pipeline(), false);
    ERR_FAIL_COND_V(!create_framebuffers(), false);

    return true;
}

bool VulkanDriver::draw(const Window &window) {

    const uint64_t max_uint64 = 0xffffffffffffffff;

    // Wait in case the current frame is still rendering
    vkWaitForFences(_device, 1, &_in_flight_fences[_current_frame], VK_TRUE, max_uint64);

    // Acquire image

    uint32_t image_index;
    {
        VkResult result = vkAcquireNextImageKHR(_device, _swap_chain, max_uint64, _image_available_semaphores[_current_frame], VK_NULL_HANDLE, &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            resize(window);
            return true;

        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            Log::error("Failed to acquire next swap chain image, result: ", result);
            return false;
        }
    }

    // Submit commands

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore submit_wait_semaphores[] = { _image_available_semaphores[_current_frame] };
    VkSemaphore submit_signal_semaphores[] = { _render_finished_semaphores[_current_frame] };

    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = submit_wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &_command_buffers[image_index];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = submit_signal_semaphores;

    vkResetFences(_device, 1, &_in_flight_fences[_current_frame]);

    // Note: we use a fence which will be signaled when the command buffers finish to execute
    CHECK_RESULT_V(vkQueueSubmit(_graphics_queue, 1, &submit_info, _in_flight_fences[_current_frame]), false);

    // Present

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = submit_signal_semaphores;
    VkSwapchainKHR swap_chains[] = { _swap_chain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr; // Optional

    {
        VkResult result = vkQueuePresentKHR(_present_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _scheduled_resize) {
            resize(window);

        } else if (result != VK_SUCCESS) {
            Log::error("Vulkan present failed with result ", result);
            return false;
        }
    }

    _current_frame = (_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
}

void VulkanDriver::wait() {
    vkDeviceWaitIdle(_device);
}

VkDevice VulkanDriver::get_device() const {
    return _device;
}

VkPhysicalDevice VulkanDriver::get_physical_device() const {
    return _physical_device;
}

static bool find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties, uint32_t &out_memory_type) {

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            out_memory_type = i;
            return true;
        }
    }

    Log::error("Could not find Vulkan memory type");
    return false;
}

bool VulkanDriver::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {

    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_RESULT_V(vkCreateBuffer(_device, &create_info, nullptr, &buffer), false);

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    ERR_FAIL_COND_V(!find_memory_type(_physical_device, memory_requirements.memoryTypeBits, properties, alloc_info.memoryTypeIndex), false);
    CHECK_RESULT_V(vkAllocateMemory(_device, &alloc_info, nullptr, &buffer_memory), false);

    // Note: if the offset is non-zero, then it is required to be divisible by memRequirements.alignment.
    // Also we are limited to a few thousand buffers.
    // Make one memory for all our buffers?

    CHECK_RESULT_V(vkBindBufferMemory(_device, buffer, buffer_memory, 0), false);

    return true;
}

bool VulkanDriver::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {

    if (_short_lived_command_pool == VK_NULL_HANDLE) {

        // You may wish to create a separate command pool for these kinds of short-lived buffers,
        // because the implementation may be able to apply memory allocation optimizations
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = _queue_family_indices.graphics;
        create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        CHECK_RESULT_V(vkCreateCommandPool(_device, &create_info, nullptr, &_short_lived_command_pool), false);
    }

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = _short_lived_command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    CHECK_RESULT_V(vkAllocateCommandBuffers(_device, &alloc_info, &command_buffer), false);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CHECK_RESULT_V(vkBeginCommandBuffer(command_buffer, &begin_info), false);

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0; // Optional
    copy_region.dstOffset = 0; // Optional
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    CHECK_RESULT_V(vkEndCommandBuffer(command_buffer), false);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    CHECK_RESULT_V(vkQueueSubmit(_graphics_queue, 1, &submit_info, VK_NULL_HANDLE), false);

    // Note: we could also use a fence to upload multiple buffers simultaneously and wait for them at once
    CHECK_RESULT_V(vkQueueWaitIdle(_graphics_queue), false);

    vkFreeCommandBuffers(_device, _short_lived_command_pool, 1, &command_buffer);

    return true;
}




