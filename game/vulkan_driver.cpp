#include "vulkan_driver.h"
#include "core/log.h"
#include "core/file.h"
#include "window.h"

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

VulkanDriver::VulkanDriver() {

    _instance = VK_NULL_HANDLE;
    _debug_messenger = VK_NULL_HANDLE;
    _device = VK_NULL_HANDLE;
    _graphics_queue = VK_NULL_HANDLE;
    _surface = VK_NULL_HANDLE;

    _swap_chain = VK_NULL_HANDLE;
    _swap_chain_image_format = {};
    _swap_chain_extent = {};

    _render_pass = VK_NULL_HANDLE;
    _pipeline_layout = VK_NULL_HANDLE;
    _graphics_pipeline = VK_NULL_HANDLE;
}

VulkanDriver::~VulkanDriver() {
    if(_instance) {

        if(_graphics_pipeline) {
            vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
        }

        if(_pipeline_layout) {
            vkDestroyPipelineLayout(_device, _pipeline_layout, nullptr);
        }

        if (_render_pass) {
            vkDestroyRenderPass(_device, _render_pass, nullptr);
        }

        for (int i = 0; i < _swap_chain_image_views.size(); ++i) {
            VkImageView view = _swap_chain_image_views[i];
            if(view) {
                vkDestroyImageView(_device, view, nullptr);
            }
        }

        if(_swap_chain) {
            vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
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

bool VulkanDriver::create(const char *app_name,
    Vector<const char*> required_extensions,
    Vector<const char*> required_layers,
    Window &window) {

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

        if(!contains_all_extensions(extensions, required_extensions, true)) {
            return false;
        }
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

        VkResult result = vkCreateInstance(&create_info, nullptr, &_instance);
        if (result != VK_SUCCESS) {
            Log::error(L"Failed to create Vulkan instance: result ", result);
            return false;
        }
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
    {
        VkResult result = window.create_vulkan_surface(_instance, nullptr, &_surface);
        if(result != VK_SUCCESS) {
            Log::error(L"Failed to create Vulkan surface: result ", result);
            return false;
        }
    }

    struct QueueFamilyIndices {
        int graphics = -1;
        int presentation = -1;

        bool is_complete() const {
            return graphics != -1 && presentation != -1;
        }
    };

    struct SwapChainSupportDetails  {
        VkSurfaceCapabilitiesKHR capabilities;
        Vector<VkSurfaceFormatKHR> formats;
        Vector<VkPresentModeKHR> modes;
    };

    // Pick physical device
    QueueFamilyIndices queue_family_indices;
    SwapChainSupportDetails swap_chain_support_details;
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

        Log::info(L"Found ", physical_devices_count, L" Vulkan physical devices");

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
            {
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

                uint32_t formats_count = 0;
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formats_count, nullptr);
                if(formats_count == 0)
                    continue;
                details.formats.resize_no_init(formats_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formats_count, details.formats.data());

                uint32_t modes_count = 0;
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &modes_count, nullptr);
                if(modes_count == 0)
                    continue;
                details.modes.resize_no_init(modes_count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &modes_count, details.modes.data());
            }

            // Pick that device
            queue_family_indices = indices;
            swap_chain_support_details = details;
            physical_device = physical_devices[i];
            break;
        }

        if (!physical_device) {
            Log::error("No suitable Vulkan physical device");
            return false;
        }
    }

    // Create logical device
    {
        Vector<int> unique_queue_indices;
        unique_queue_indices.push_back(queue_family_indices.graphics);
        if(!unique_queue_indices.contains(queue_family_indices.presentation))
            unique_queue_indices.push_back(queue_family_indices.presentation);

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

        VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &_device);
        if (result != VK_SUCCESS) {
            Log::error(L"Failed to create Vulkan device: result ", result);
            return false;
        }
    }

    vkGetDeviceQueue(_device, queue_family_indices.graphics, 0, &_graphics_queue);
    vkGetDeviceQueue(_device, queue_family_indices.presentation, 0, &_present_queue);

    // Create swap chain
    {
        // Format
        VkSurfaceFormatKHR surface_format = {};
        if (swap_chain_support_details.formats.size() == 1 && swap_chain_support_details.formats[0].format == VK_FORMAT_UNDEFINED) {
            surface_format = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

        } else {
            for(int i = 0; i < swap_chain_support_details.formats.size(); ++i){
                VkSurfaceFormatKHR f = swap_chain_support_details.formats[i];
                if(f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    surface_format = f;
                    break;
                }
            }
        }
        if(surface_format.format == VK_FORMAT_UNDEFINED) {
            Log::warning("Falling back on first found surface format");
            surface_format = swap_chain_support_details.formats[0];
        }

        // Presentation mode
        VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (int i = 0; i < swap_chain_support_details.modes.size(); ++i) {
            VkPresentModeKHR m = swap_chain_support_details.modes[i];
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                present_mode = m;
                break;
            } else if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                present_mode = m;
            }
        }

        // Swap extent
        VkExtent2D extent = {};
        if (swap_chain_support_details.capabilities.currentExtent.width != 0xffffffff) {
            extent = swap_chain_support_details.capabilities.currentExtent;

        } else {
            Vector2i window_size = window.get_size();
            const VkSurfaceCapabilitiesKHR &c = swap_chain_support_details.capabilities;
            extent.width = Math::clamp(static_cast<uint32_t>(window_size.x), c.minImageExtent.width, c.maxImageExtent.width);
            extent.height = Math::clamp(static_cast<uint32_t>(window_size.y), c.minImageExtent.height, c.maxImageExtent.height);
        }

        uint32_t image_count = swap_chain_support_details.capabilities.minImageCount + 1;
        // 0 means no limit
        if (swap_chain_support_details.capabilities.maxImageCount > 0 && image_count > swap_chain_support_details.capabilities.maxImageCount) {
            image_count = swap_chain_support_details.capabilities.maxImageCount;
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

            uint32_t indices[] = {(uint32_t) queue_family_indices.graphics, (uint32_t) queue_family_indices.presentation};
            if (queue_family_indices.graphics != queue_family_indices.presentation) {
                create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = 2;
                create_info.pQueueFamilyIndices = indices;
            } else {
                // Preferred, more performant
                create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                create_info.queueFamilyIndexCount = 0; // Optional
                create_info.pQueueFamilyIndices = nullptr; // Optional
            }

            create_info.preTransform = swap_chain_support_details.capabilities.currentTransform;
            create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Window is not transparent
            create_info.presentMode = present_mode;
            create_info.clipped = VK_TRUE; // Don't care about pixels behind other windows
            create_info.oldSwapchain = VK_NULL_HANDLE;

            VkResult result = vkCreateSwapchainKHR(_device, &create_info, nullptr, &_swap_chain);
            if (result != VK_SUCCESS) {
                Log::error("Could not create Vulkan swap chain, result: ", result);
                return false;
            }
        }

        vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, nullptr);
        _swap_chain_images.resize_no_init(image_count);
        vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, _swap_chain_images.data());

        _swap_chain_image_format = surface_format.format;
        _swap_chain_extent = extent;

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

            VkResult result = vkCreateImageView(_device, &create_info, nullptr, &_swap_chain_image_views[i]);
            if (result != VK_SUCCESS) {
                Log::error("Failed to create Vulkan swap chain image view, result: ", result);
                return false;
            }
        }
    }

    // Create render pass
    {
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

        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &color_attachment;
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;

        VkResult result = vkCreateRenderPass(_device, &create_info, nullptr, &_render_pass);
        if (result != VK_SUCCESS) {
            Log::error("Could not create render pass: ", result);
            return false;
        }
    }

    // Create pipeline
    {
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

            VkResult result = vkCreateShaderModule(_device, &create_info, nullptr, &vert_shader_module);
            if (result != VK_SUCCESS) {
                Log::error("Could not create shader module");
                return false;
            }
        }
        VkShaderModule frag_shader_module = VK_NULL_HANDLE;
        {
            VkShaderModuleCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.codeSize = frag_shader_code.size();
            create_info.pCode = reinterpret_cast<const uint32_t*>(frag_shader_code.data());

            VkResult result = vkCreateShaderModule(_device, &create_info, nullptr, &frag_shader_module);
            if (result != VK_SUCCESS) {
                Log::error("Could not create shader module");
                return false;
            }
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
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // No mesh to draw yet
        vertex_input_info.vertexBindingDescriptionCount = 0;
        vertex_input_info.pVertexBindingDescriptions = nullptr; // Optional
        vertex_input_info.vertexAttributeDescriptionCount = 0;
        vertex_input_info.pVertexAttributeDescriptions = nullptr; // Optional

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

            VkResult result = vkCreatePipelineLayout(_device, &create_info, nullptr, &_pipeline_layout);
            if (result != VK_SUCCESS) {
                Log::error("Could not create pipeline layout: ", result);
                return false;
            }
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

            VkResult result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &create_info, nullptr, &_graphics_pipeline);
            if (result != VK_SUCCESS) {
                Log::error("Could not create graphics pipeline, result: ", result);
                return false;
            }
        }

        //...

//        vkDestroyShaderModule(_device, vert_shader_module, nullptr);
//        vkDestroyShaderModule(_device, frag_shader_module, nullptr);
    }

    return true;
}







