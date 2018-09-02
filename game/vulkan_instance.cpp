#include "vulkan_instance.h"
#include "core/console.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data) {

    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        Console::print_raw("ERROR: ");

    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        Console::print_raw("WARNING: ");

    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        Console::print_raw("INFO: ");
    }

    Console::print_raw("Vulkan: ");

    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        Console::print_raw("General: ");
    }
    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        Console::print_raw("Performance: ");
    }
    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        Console::print_raw("Validation: ");
    }

    Console::print_line(callback_data->pMessage);

    return VK_FALSE;
}

VulkanInstance::VulkanInstance() {
    _instance = nullptr;
    _debug_messenger = nullptr;
}

VulkanInstance::~VulkanInstance() {
    if(_instance) {

        if(_debug_messenger) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(_instance, _debug_messenger, nullptr);
            }
        }

        vkDestroyInstance(_instance, nullptr);
    }
}

bool VulkanInstance::create(
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
        Console::print_raw("\t");
        Console::print_line(required_extensions[i]);
    }
    Console::print_line();
    Console::print_line("Required Vulkan layers:");
    for(int i = 0; i < required_layers.size(); ++i) {
        Console::print_raw("\t");
        Console::print_line(required_layers[i]);
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
            Console::print_raw(L"\t");
            Console::print_line(extensions[i].extensionName);
        }
        Console::print_line();

        for (int i = 0; i < required_extensions.size(); ++i) {

            bool found = false;
            for (int j = 0; j < extensions.size() && !found; ++j) {
                found |= strcmp(required_extensions[i], extensions[j].extensionName) == 0;
            }

            if (!found) {
                Console::print_raw("ERROR: Required Vulkan extension is not available: ");
                Console::print_line(required_extensions[i]);
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
            Console::print_raw(L"\t");
            Console::print_line(available_layers[i].layerName);
        }
        Console::print_line();

        for (int i = 0; i < required_layers.size(); ++i) {

            bool found = false;
            for (int j = 0; j < available_layers.size() && !found; ++j) {
                found |= strcmp(required_layers[i], available_layers[j].layerName) == 0;
            }

            if (!found) {
                Console::print_raw("ERROR: Required Vulkan layer is not available: ");
                Console::print_line(required_layers[i]);
                return false;
            }
        }
    }

    // Create instance

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
        Console::print_line(String::format(L"ERROR: Failed to create Vulkan instance: result %", result));
        return false;
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
            Console::print_line("ERROR: Could not get function address");
        }

        if (result != VK_SUCCESS) {
            Console::print_line(String::format(L"ERROR: Failed to create Vulkan debug callback: result %", result));
            return false;
        }
    }
#endif

    return true;
}

