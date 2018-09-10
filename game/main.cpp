#include "window.h"
#include "core/log.h"
#include "vulkan_driver.h"

int main_loop();

int main() {

    Console::print_line(L"Hello World");

    int ret = main_loop();

    Log::info(L"Alloc count on exit: ", (int64_t)Memory::get_alloc_count());

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
    if (!driver.create(app_name, required_extensions, required_layers, window)) {
        return EXIT_FAILURE;
    }

    while (!window.should_close()) {

        Window::poll_events();

        InputEvent event;
        while (window.pop_event(event)) {

            if(event.type == InputEvent::FRAMEBUFFER_RESIZED) {
                // Workaround for some drivers not returning VK_ERROR_OUT_OF_DATE_KHR on resize
                driver.schedule_resize();
            }
        }

        // Don't draw in minimized state, framebuffer size is zero
        if (window.get_framebuffer_size() != Vector2i()) {

            if (!driver.draw(window)) {
                // If something wrong happens in rendering, don't bail-loop forever
                break;
            }
        }

        // TODO Limit framerate, maybeee
    }

    driver.wait();

    return EXIT_SUCCESS;
}


