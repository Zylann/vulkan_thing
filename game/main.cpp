#define GLFW_INCLUDE_VULKAN

#include "window.h"
#include "core/log.h"
#include "vulkan_driver.h"

int main_loop();

int main() {

    Console::print_line(L"Hello World");

    int ret = main_loop();

    Log::info(L"Alloc count on exit: ", Memory::get_alloc_count());

    return ret;
}

int main_loop() {

    // TODO Handle resizeable window
    const char *app_name = "Vulkan test";
    Window window(Vector2i(800, 600), app_name);

    Vector<const char*> required_extensions;
    Window::get_required_vulkan_extensions(required_extensions);
    Vector<const char*> required_layers;

    VulkanDriver driver;
    if (!driver.create(app_name, required_extensions, required_layers)) {
        return EXIT_FAILURE;
    }

    while (!window.should_close()) {
        Window::poll_events();
    }

    return EXIT_SUCCESS;
}


