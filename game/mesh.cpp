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

static bool find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties, uint32_t &out_memory_type) {

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            out_memory_type = i;
            return true;
        }
    }

    Log::error("Could not find Vulkan memory type");
    return false;
}

static bool allocate_memory(VkDevice device, VkPhysicalDevice physical_device, VkBuffer buffer, VkDeviceMemory &out_memory) {

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (!find_memory_type(physical_device, memory_requirements.memoryTypeBits, flags, alloc_info.memoryTypeIndex))
        return false;
    CHECK_RESULT_V(vkAllocateMemory(device, &alloc_info, nullptr, &out_memory), false);

    return true;
}

template <typename T>
static bool copy_to(VkDevice device, const Vector<T> &src, VkDeviceMemory memory) {
    void *dst;
    CHECK_RESULT_V(vkMapMemory(device, memory, 0, size_in_bytes(src), 0, &dst), false);
    memcpy(dst, src.data(), size_in_bytes(src));
    vkUnmapMemory(device, memory);
    return true;
}

bool Mesh::upload(VulkanDriver &driver) {

    // TODO Handle subsequent uploads on mesh modification
    assert(_positions_buffer == VK_NULL_HANDLE);

    VkDevice device = driver.get_device();
    VkPhysicalDevice physical_device = driver.get_physical_device();

    _driver = &driver;

    if (_positions_buffer == VK_NULL_HANDLE) {
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = sizeof(Vector2) * get_vertex_count();
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CHECK_RESULT_V(vkCreateBuffer(device, &create_info, nullptr, &_positions_buffer), false);
    }
    if (_colors_buffer == VK_NULL_HANDLE) {
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = sizeof(Vector3) * get_vertex_count();
        create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CHECK_RESULT_V(vkCreateBuffer(device, &create_info, nullptr, &_colors_buffer), false);
    }

    ERR_FAIL_COND_V(!allocate_memory(device, physical_device, _positions_buffer, _positions_buffer_memory), false);
    ERR_FAIL_COND_V(!allocate_memory(device, physical_device, _colors_buffer, _colors_buffer_memory), false);

    // Note: if the offset is non-zero, then it is required to be divisible by memRequirements.alignment.
    // Also we are limited to a few thousand buffers.
    // Make one memory for all our buffers?
    CHECK_RESULT_V(vkBindBufferMemory(device, _positions_buffer, _positions_buffer_memory, 0), false);
    CHECK_RESULT_V(vkBindBufferMemory(device, _colors_buffer, _colors_buffer_memory, 0), false);

    ERR_FAIL_COND_V(!copy_to(device, _positions, _positions_buffer_memory), false);
    ERR_FAIL_COND_V(!copy_to(device, _colors, _colors_buffer_memory), false);

    return true;
}

void Mesh::draw(VkCommandBuffer command_buffer) {

    VkBuffer buffers[] = { _positions_buffer, _colors_buffer };
    VkDeviceSize offsets[] = { 0, 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);

    vkCmdDraw(command_buffer, static_cast<uint32_t>(get_vertex_count()), 1, 0, 0);
}


