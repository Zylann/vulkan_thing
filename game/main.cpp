#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/math/vector2.h"
#include "core/console.h"

int main() {

    Console::print_line(L"Hello World");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // TODO Handle resizeable window
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    Vector2i window_size(800, 600);
    const char *app_name = "Vulkan test";
    GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, app_name, nullptr, nullptr);

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

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

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;

        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        Console::print_line(String::format(L"% Vulkan extensions supported", extension_count));

        create_info.enabledExtensionCount = glfw_extension_count;
        create_info.ppEnabledExtensionNames = glfw_extensions;

        create_info.enabledExtensionCount = 0;

        VkResult result = vkCreateInstance(&create_info, nullptr, &vulkan_instance);
        if(result != VK_SUCCESS) {
            Console::print_line(String::format(L"Failed to create Vulkan instance: result %", result));
            return EXIT_FAILURE;
        }
    }

    //Log::info(extensionCount, " extensions supported");
    //std::cout << extensionCount << " extensions supported" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    Console::print_line(String::format(L"Alloc count on exit: %", Memory::get_alloc_count()));

    return EXIT_SUCCESS;
}

