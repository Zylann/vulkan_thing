#ifndef HEADER_MESH_H
#define HEADER_MESH_H

#include "core/vector.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include <vulkan/vulkan.h>

class VulkanDriver;

class Mesh {
public:
    Mesh();
    ~Mesh();

    void make_triangle();

    int get_vertex_count();

    static void get_description(Vector<VkVertexInputBindingDescription> & out_bindings, Vector<VkVertexInputAttributeDescription> &out_attributes);

    bool upload(VulkanDriver &driver);
    void draw(VkCommandBuffer command_buffer);

private:
    Vector<Vector2> _positions;
    Vector<Vector3> _colors;

    VkBuffer _positions_buffer;
    VkBuffer _colors_buffer;

    VkDeviceMemory _positions_buffer_memory;
    VkDeviceMemory _colors_buffer_memory;

    VulkanDriver *_driver;
};

#endif // HEADER_MESH_H
