#include "window.h"
#include "core/macros.h"
#include "vulkan_driver.h"
#include "core/math/vector3.h"
#include "mesh.h"

int main_loop();

int main() {

    Console::print_line(L"Hello World");

    int ret = main_loop();

    Log::info(L"Alloc count on exit: ", (int64_t)Memory::get_alloc_count());

    return ret;
}

int main_loop() {

    const char *app_name = "Vulkan test";
    Window window(Vector2i(800, 600), app_name);

    Vector<const char*> required_extensions;
    Window::get_required_vulkan_extensions(required_extensions);
    Vector<const char*> required_layers;

    VulkanDriver driver;
    ERR_FAIL_COND_V(!driver.create(app_name, required_extensions, required_layers, window), EXIT_FAILURE);

    Mesh *mesh = new Mesh();
    mesh->make_triangle();
    mesh->upload(driver);

    driver.scene.push_back(mesh);

    driver.create_command_buffers();

    while (!window.should_close()) {

        Window::poll_events();

        InputEvent event;
        while (window.pop_event(event)) {

            if(event.type == InputEvent::FRAMEBUFFER_RESIZED) {
                // Workaround for some drivers not returning VK_ERROR_OUT_OF_DATE_KHR on resize
                driver.schedule_resize();
            }

            // TODO Repaint while resizing?
            // https://stackoverflow.com/questions/45880238/how-to-draw-while-resizing-glfw-window
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


