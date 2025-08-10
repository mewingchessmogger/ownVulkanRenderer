#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS 
#include <vector>
#include<glm/vec3.hpp>
#include<glm/mat4x4.hpp>


struct BufferContext {
    
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };
    std::vector<Vertex> vertices{
        {{0.0f, -0.5f,0.0f}, {1.0f,0.0f,0.0f}},
        {{0.5f, 0.5f,0.0f}, {0.0f,1.0f,0.0f}},
        {{-0.5f, 0.5f,0.0f}, {0.0f,0.0f,1.0f}}
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

};
