#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/math/vector4.h"
#include "core/math/matrix4.h"

#include "core/console.h"

int main() {
    Console::print_line(L"Hello World");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    Console::print_line(String::format(L"% extensions supported", extension_count));
    //Log::info(extensionCount, " extensions supported");
    //std::cout << extensionCount << " extensions supported" << std::endl;

    Matrix4 matrix;
    Vector4 vec;
    Vector4 test = matrix.transform(vec);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
