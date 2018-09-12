#include "mesh.h"
#include "vulkan_driver.h"
#include "core/macros.h"

Mesh::Mesh() {

    _positions_buffer = VK_NULL_HANDLE;
    _colors_buffer = VK_NULL_HANDLE;

    _positions_buffer_memory = VK_NULL_HANDLE;
    _colors_buffer_memory = VK_NULL_HANDLE;

    _driver = nullptr;
}

void Mesh::make_triangle() {

    _positions.push_back(Vector2(0, -0.5));
    _positions.push_back(Vector2(0.5, 0.5));
    _positions.push_back(Vector2(-0.5, 0.5));

    _colors.push_back(Vector3(1, 0, 0));
    _colors.push_back(Vector3(0, 1, 0));
    _colors.push_back(Vector3(0, 0, 1));
}

Mesh::~Mesh() {

    if(_driver) {

        VkDevice device = _driver->get_device();

        if (_positions_buffer) {
            vkDestroyBuffer(device, _positions_buffer, nullptr);
        }
        if (_colors_buffer) {
            vkDestroyBuffer(device, _colors_buffer, nullptr);
        }

        if (_positions_buffer_memory) {
            vkFreeMemory(device, _positions_buffer_memory, nullptr);
        }
        if (_colors_buffer_memory) {
            vkFreeMemory(device, _colors_buffer_memory, nullptr);
        }
    }
}

int Mesh::get_vertex_count() {
    return _positions.size();
}

void Mesh::get_description(Vector<VkVertexInputBindingDescription> & out_bindings, Vector<VkVertexInputAttributeDescription> &out_attributes) {

    VkVertexInputBindingDescription position_binding_description = {};
    position_binding_description.binding = 0;
    position_binding_description.stride = sizeof(Vector2);
    position_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputBindingDescription color_binding_description = {};
    color_binding_description.binding = 1;
    color_binding_description.stride = sizeof(Vector3);
    color_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute_descriptions[2] = {};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[0].offset = 0;

    attribute_descriptions[1].binding = 1;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = 0;

    out_bindings.push_back(position_binding_description);
    out_bindings.push_back(color_binding_description);

    out_attributes.push_back(attribute_descriptions[0]);
    out_attributes.push_back(attribute_descriptions[1]);
}

template <typename T>
static bool copy_to(VkDevice device, const Vector<T> &src, VkDeviceMemory memory) {
    void *dst;
    CHECK_RESULT_V(vkMapMemory(device, memory, 0, size_in_bytes(src), 0, &dst), false);
    memcpy(dst, src.data(), size_in_bytes(src));
    vkUnmapMemory(device, memory);
    return true;
}

template <typename T>
static bool upload_buffer(VulkanDriver &driver, const Vector<T> &data, VkBuffer &buffer, VkDeviceMemory &buffer_memory) {

    VkDevice device = driver.get_device();

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    VkDeviceSize buffer_size = size_in_bytes(data);
    VkMemoryPropertyFlags staging_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ERR_FAIL_COND_V(!driver.create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_flags, staging_buffer, staging_buffer_memory), false);

    ERR_FAIL_COND_V(!copy_to(device, data, staging_buffer_memory), false);

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    ERR_FAIL_COND_V(!driver.create_buffer(buffer_size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, buffer_memory), false);

    driver.copy_buffer(staging_buffer, buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    return true;
}

bool Mesh::upload(VulkanDriver &driver) {

    // TODO Handle subsequent uploads on mesh modification
    assert(_positions_buffer == VK_NULL_HANDLE);

    _driver = &driver;

    ERR_FAIL_COND_V(!upload_buffer(driver, _positions, _positions_buffer, _positions_buffer_memory), false);
    ERR_FAIL_COND_V(!upload_buffer(driver, _colors, _colors_buffer, _colors_buffer_memory), false);

    return true;
}

void Mesh::draw(VkCommandBuffer command_buffer) {

    VkBuffer buffers[] = { _positions_buffer, _colors_buffer };
    VkDeviceSize offsets[] = { 0, 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);

    vkCmdDraw(command_buffer, static_cast<uint32_t>(get_vertex_count()), 1, 0, 0);
}


