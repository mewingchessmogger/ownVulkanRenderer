#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vector>
#include<glm/vec3.hpp>
#include<glm/mat4x4.hpp>
#include <vulkan/vulkan.hpp>
struct VmaAllocator_T;      using VmaAllocator = VmaAllocator_T*;
struct VmaAllocation_T;     using VmaAllocation = VmaAllocation_T*;
struct VmaAllocationInfo;
struct VmaAllocationCreateInfo;


struct AllocatedBuffer {
    vk::Buffer buffer{};
    VmaAllocation alloc{};
    VmaAllocationInfo allocInfo{};
};

struct AllocatedImage {
    vk::Image image{};
    vk::ImageView view;
    VmaAllocation alloc{};
    VmaAllocationInfo allocInfo{};
    vk::Format format;
    vk::Extent3D extent;
};
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return (pos == other.pos) && (normal == other.normal) && (texCoord == other.texCoord);
    }

};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec2>()(vertex.texCoord) << 1)));
                //(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                
        }
    };
}


struct alignas(16) TransformUBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    

};

struct BufferContext {


   

    //BUFFER STUFF
    AllocatedBuffer _stagingBuffer;
    AllocatedBuffer _vertexBuffer;
    AllocatedBuffer _indexBuffer;
    AllocatedBuffer _uniformBuffer;

    VmaAllocator _allocator;


    
    //IMAGE STUFF
    std::vector<AllocatedImage> _renderTargets;
    //std::vector<vk::ImageView> _renderTargetViews;
    
    std::vector<AllocatedImage> _txtImages;
    
    std::vector<AllocatedImage> _depthImages;
    



    //BUFFER DATA : vertex, rgb , texCoord
    std::vector<Vertex> vertices = {

    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // top-right

    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // top-left

    // Back face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
    {{ 0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}}, // top-left

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right
    {{ 0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}}, // top-left
    {{ 0.5f, -0.5f, -0.5f}, {0.2f, 0.8f, 0.2f}, {0.0f, 0.0f}}, // bottom-left

    // Left face
    {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.1f, 0.3f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.7f, 0.1f, 0.3f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.7f, 0.1f, 0.3f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.1f, 0.3f}, {0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.7f, 0.1f, 0.3f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.7f, 0.1f, 0.3f}, {0.0f, 1.0f}},

    // Right face
    {{ 0.5f, -0.5f, -0.5f}, {0.1f, 0.7f, 0.3f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.1f, 0.7f, 0.3f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.1f, 0.7f, 0.3f}, {0.0f, 0.0f}},

    {{ 0.5f, -0.5f, -0.5f}, {0.1f, 0.7f, 0.3f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.1f, 0.7f, 0.3f}, {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.1f, 0.7f, 0.3f}, {0.0f, 1.0f}},

    // Top face
    {{-0.5f,  0.5f, -0.5f}, {0.3f, 0.3f, 0.9f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.3f, 0.3f, 0.9f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.3f, 0.3f, 0.9f}, {1.0f, 0.0f}},

    {{-0.5f,  0.5f, -0.5f}, {0.3f, 0.3f, 0.9f}, {0.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.3f, 0.3f, 0.9f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.3f, 0.3f, 0.9f}, {1.0f, 1.0f}},

    // Bottom face
    {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.9f, 0.2f}, {1.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.9f, 0.9f, 0.2f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.9f, 0.9f, 0.2f}, {1.0f, 0.0f}},

    {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.9f, 0.2f}, {1.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.9f, 0.9f, 0.2f}, {0.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.9f, 0.9f, 0.2f}, {0.0f, 0.0f}},
   
   // bot-right
    };         
                        //key  , value
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    std::vector<uint32_t> indices{};


    TransformUBO dataUBO;
    size_t strideUBO;

    std::vector<vk::VertexInputBindingDescription> getVertBindings(){
    vk::VertexInputBindingDescription posBinding{};
    posBinding
        .setBinding(0)
        .setInputRate(vk::VertexInputRate::eVertex)
        .setStride(sizeof(vertices[0]));
    return {posBinding};
    }

    std::vector<vk::VertexInputAttributeDescription> getVertAttributes() {

        vk::VertexInputAttributeDescription posAttrib{};
        vk::VertexInputAttributeDescription normAttrib{};
        vk::VertexInputAttributeDescription texCoordAttrib{};


        posAttrib
            .setLocation(0)
            .setOffset(0)
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat);
        normAttrib
            .setLocation(1)
            .setOffset(offsetof(Vertex, normal))
            .setBinding(0)
            .setFormat(vk::Format::eR32G32B32Sfloat);


        texCoordAttrib
            .setLocation(2)
            .setOffset(offsetof(Vertex, texCoord))
            .setBinding(0)
            .setFormat(vk::Format::eR32G32Sfloat);


        return { posAttrib,normAttrib,texCoordAttrib };
    }




};
