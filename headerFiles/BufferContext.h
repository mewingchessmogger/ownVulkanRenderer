#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS 
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
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
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >> 1);
                
                
        }
    };
}

struct Lights {
    glm::vec4 position{};
    glm::vec4 color{};
};


struct alignas(16) TransformUBO {
    glm::mat4 view;
    glm::mat4 proj;
    std::array<Lights,8> lights{};
    alignas(16) int currLightCount{};
};



struct BufferContext {

   
    struct indexDataModels {
        uint32_t startVBO;
        uint32_t endVBO;

        uint32_t startIBO;
        uint32_t endIBO;
    };
    
    struct IDGenerator{
        std::unordered_map<size_t, indexDataModels> modelMapper;
        std::hash<std::string> hasher;
        
    };


    struct GameObj {
        glm::vec3 pos = { 0.0f,0.0f,0.0f };
        glm::vec3 rot = { 0.0f,0.0f,0.0f };
        glm::vec3 scale = { 1.0f,1.0f,1.0f };

        size_t modelID;
        uint32_t objectIndex;
        int usingTexture = 0;
        glm::vec3 color = glm::vec3(0.2f);
        //std::vector<AllocatedBuffer> uniBuffers{};
        std::vector<vk::DescriptorSet> descSets{};


        glm::mat4 getModelMatrix() const {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, scale);
            return model;
        };
    };


    struct pushConstants {

        glm::mat4 model;
        glm::vec3 color;
        int useTexture;
        int indexInIBO;

    };

    struct skyboxPC {
        glm::mat4 invView;
        glm::mat4 invProj;
    };




    static const int NUM_OF_IMAGES = 2;
    std::vector<indexDataModels> offsetOfModels{};
    IDGenerator ComposerID;
    std::vector<std::string> modPaths;

    static const int MAX_OBJECTS = 3;
    std::vector<GameObj> gameObjs{};
    
    AllocatedBuffer _GameObjUBO;
    std::vector<GameObj> lightObjs{};

    //BUFFER STUFF
    AllocatedBuffer _stagingBuffer;
    AllocatedBuffer _vertexBuffer;
    AllocatedBuffer _indexBuffer;

    VmaAllocator _allocator;


    
    //IMAGE STUFF
    std::vector<AllocatedImage> _renderTargets;
    //std::vector<vk::ImageView> _renderTargetViews;
    
    std::vector<AllocatedImage> _txtImages;
    std::vector<AllocatedImage> _cubeImages;
    std::vector<AllocatedImage> _depthImages;
    



    //BUFFER DATA : vertex, rgb , texCoord
    std::vector<Vertex> vertices = {};         
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
