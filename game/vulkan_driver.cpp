#include "vulkan_driver.h"
#include "core/log.h"

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

VulkanDriver::VulkanDriver() {
    _instance = VK_NULL_HANDLE;
    _debug_messenger = VK_NULL_HANDLE;
    _device = VK_NULL_HANDLE;
    _graphics_queue = VK_NULL_HANDLE;
}

VulkanDriver::~VulkanDriver() {
    if(_instance) {

        if(_device) {
            vkDestroyDevice(_device, nullptr);
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

bool VulkanDriver::create(
    const char *app_name,
    Vector<const char*> required_extensions,
    Vector<const char*> required_layers) {

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

        for (int i = 0; i < required_extensions.size(); ++i) {

            bool found = false;
            for (int j = 0; j < extensions.size() && !found; ++j) {
                found |= strcmp(required_extensions[i], extensions[j].extensionName) == 0;
            }

            if (!found) {
                Log::error("Required Vulkan extension is not available: ", required_extensions[i]);
                return false;
            }
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
            Log::error(L"Failed to create Vulkan debug callback: result %", result);
            return false;
        }
    }
#endif

    struct QueueFamilyIndices {
        int graphics = -1;

        bool is_complete() const {
            return graphics != -1;
        }
    };

    // Pick physical device
    QueueFamilyIndices queue_family_indices;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    {
        // Enumerate physical devices
        uint32_t physical_devices_count = 0;
        vkEnumeratePhysicalDevices(_instance, &physical_devices_count, nullptr);
        if (physical_devices_count == 0) {
            Console::print_line("ERROR: No Vulkan physical devices found");
            return false;
        }
        Vector<VkPhysicalDevice> physical_devices;
        physical_devices.resize_no_init(physical_devices_count);
        vkEnumeratePhysicalDevices(_instance, &physical_devices_count, physical_devices.data());

        Log::info(L"Found", physical_devices_count, L" Vulkan physical devices");

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

                if (family.queueCount > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphics = j;
                }

                if (indices.is_complete())
                    break;
            }

            if (!indices.is_complete()) {
                // This device doesn't have all the queue families we need
                continue;
            }

            queue_family_indices = indices;
            physical_device = physical_devices[i];
            break;
        }

        if (!physical_device) {
            Console::print_line("ERROR: No suitable Vulkan physical device");
            return false;
        }
    }

    // Create logical device
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family_indices.graphics;
        queue_create_info.queueCount = 1;
        float queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority;

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos = &queue_create_info;
        create_info.queueCreateInfoCount = 1;
        create_info.pEnabledFeatures = &device_features;
        create_info.enabledLayerCount = required_layers.size();
        create_info.ppEnabledLayerNames = required_layers.data();

        VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &_device);
        if (result != VK_SUCCESS) {
            Console::print_line(String::format(L"ERROR: Failed to create Vulkan device: result %", result));
            return false;
        }
    }

    vkGetDeviceQueue(_device, queue_family_indices.graphics, 0, &_graphics_queue);

    return true;
}







