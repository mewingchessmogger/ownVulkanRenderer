#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS 
#include <vector>
#include<glm/vec3.hpp>
#include<glm/mat4x4.hpp>
struct VmaAllocator_T;      using VmaAllocator = VmaAllocator_T*;
struct VmaAllocation_T;     using VmaAllocation = VmaAllocation_T*;
struct VmaAllocationInfo;
struct VmaAllocationCreateInfo;

struct BufferContext {

   
    struct allocatedBuffer {
        vk::Buffer buffer{};
        VmaAllocation alloc{};
        VmaAllocationInfo allocInfo{};
        VmaAllocationCreateInfo allocCreateInfo{};
    };
    allocatedBuffer _stagingBuffer;
    allocatedBuffer _vertexBuffer;
    allocatedBuffer _uniformBuffer;
    VmaAllocator _allocator;


    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };
    std::vector<Vertex> vertices{
        {{0.0f, -0.5f,0.0f}, {1.0f,0.0f,0.0f}},
        {{0.5f, 0.5f,0.0f}, {0.0f,1.0f,0.0f}},
        {{-0.5f, 0.5f,0.0f}, {0.0f,0.0f,1.0f}}
    };


    struct alignas(16) TransformUBO {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    TransformUBO dataUBO;
    TransformUBO dataUBOsec;

    size_t strideUBO;


};
