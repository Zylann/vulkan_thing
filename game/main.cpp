#define GLFW_INCLUDE_VULKAN

#include "window.h"
#include "core/console.h"

int main_loop();

int main() {

    Console::print_line(L"Hello World");

    int ret = main_loop();

    Console::print_line(String::format(L"Alloc count on exit: %", Memory::get_alloc_count()));

    return ret;
}

int main_loop() {

    // TODO Handle resizeable window
    const char *app_name = "Vulkan test";
    Window window(Vector2i(800, 600), app_name);

    // List extensions
    uint32_t required_extension_count = 0;
    const char** required_extensions = Window::get_required_vulkan_extensions(required_extension_count);
    {
        uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        Vector<VkExtensionProperties> extensions;
        extensions.resize_no_init(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

        Console::print_line("Available Vulkan extensions:");
        for(int i = 0; i < extensions.size(); ++i) {
            Console::print_raw(L"\t");
            Console::print_line(extensions[i].extensionName);
        }

        for (int i = 0; i < required_extension_count; ++i) {
            const char *required_extension_name = required_extensions[i];

            bool found = false;
            for (int j = 0; j < extensions.size() && !found; ++j) {
                found |= strcmp(required_extension_name, extensions[j].extensionName) == 0;
            }

            if (!found) {
                Console::print_raw("ERROR: Required Vulkan extension is not available: ");
                Console::print_line(required_extension_name);
                return EXIT_FAILURE;
            }
        }
    }

    // Create Vulkan instance
    VkInstance vulkan_instance;
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
        create_info.enabledExtensionCount = required_extension_count;
        create_info.ppEnabledExtensionNames = required_extensions;
        create_info.enabledExtensionCount = 0;

        VkResult result = vkCreateInstance(&create_info, nullptr, &vulkan_instance);
        if(result != VK_SUCCESS) {
            Console::print_line(String::format(L"Failed to create Vulkan instance: result %", result));
            return EXIT_FAILURE;
        }
    }

    //Log::info(extensionCount, " extensions supported");
    //std::cout << extensionCount << " extensions supported" << std::endl;

    while (!window.should_close()) {
        Window::poll_events();
    }

    vkDestroyInstance(vulkan_instance, nullptr);

    return EXIT_SUCCESS;
}


