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


struct allocatedBuffer {
    vk::Buffer buffer{};
    VmaAllocation alloc{};
    VmaAllocationInfo allocInfo{};
};

struct allocatedImage {
    vk::Image image{};
    vk::ImageView view;
    VmaAllocation alloc{};
    VmaAllocationInfo allocInfo{};

};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

struct alignas(16) TransformUBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;

};
struct BufferContext {


   

    //BUFFER STUFF
    allocatedBuffer _stagingBuffer;
    allocatedBuffer _vertexBuffer;
    allocatedBuffer _uniformBuffer;
    VmaAllocator _allocator;


    
    //IMAGE STUFF
    std::vector<allocatedImage> _renderTargets;
    std::vector<vk::ImageView> _renderTargetViews;
    vk::Format _renderTargetFormat;
    vk::Extent3D _renderTargetExtent;
    
    std::vector<allocatedImage> _txtImages;
    std::vector<vk::ImageView> _txtImageViews;
    vk::Format _txtFormat;
    vk::Extent3D _txtExtent;


    //BUFFER DATA
    std::vector<Vertex> vertices{
        
        {{-0.5f, 0.5f,0.0f}, {1.0f,0.0f,0.0f}},
        {{-0.5f, -0.5f,0.0f}, {0.0f,1.0f,0.0f}},
        {{0.5f, 0.5f,0.0f}, {0.0f,0.0f,1.0f}},
        {{-0.5f, -0.5f,0.0f}, {0.0f,1.0f,0.0f}},
        {{0.5f, -0.5f,0.0f}, {1.0f,0.0f,0.0f}},
        {{0.5f, 0.5f,0.0f}, {0.0f,0.0f,1.0f}},


    };

    TransformUBO dataUBO;
    size_t strideUBO;


};
