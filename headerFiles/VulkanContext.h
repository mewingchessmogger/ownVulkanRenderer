#pragma once
#include <vulkan/vulkan_handles.hpp>


//forwrad declarations
#include <functional>


namespace vkb {
    struct Instance;
    struct Device;
}
struct VulkanContext {
    const int MAX_OBJECTS = 2;
    const int NUM_OF_IMAGES = 2;
    int WIDTH = 1600;
    int HEIGHT = 1200;
    const bool bUseValidationLayers = true;
    bool frameBufferResized = false;

    vk::Instance _instance;
    vk::DebugUtilsMessengerEXT _debugMessenger;
    vk::SurfaceKHR _surface;
    vk::Device _device;
    vk::PhysicalDevice _chosenGPU;

    struct queueStruct {
        vk::Queue handle;
        uint32_t famIndex;
    };
    queueStruct _graphicsQueue;

    vk::SwapchainKHR _swapchain;
    vk::Format _swapchainFormat;
    vk::Sampler _sampler;

    std::vector<vk::Image> _swapchainImages;
    std::vector<vk::ImageView> _swapchainImageViews;
    vk::Extent2D _swapchainExtent;

    vk::DescriptorSetLayout _descSetLayoutUBO;
    vk::DescriptorSetLayout _descSetLayoutSampler;

    vk::DescriptorPool _descPool;
    std::vector<vk::DescriptorSet> _descSets;
    vk::CommandPool _cmdPool;
    std::vector<vk::CommandBuffer> _cmdBuffers;
    
    vk::Pipeline _graphicsPipeline;
    vk::PipelineLayout _layout;
    vk::Pipeline _skyboxPipeline;
    vk::PipelineLayout _skyboxLayout;


    std::vector<vk::Fence> _fences;
    std::array<vk::Semaphore, 2> _imageReadySemaphores;
    std::array<vk::Semaphore, 2> _renderFinishedSemaphores;




    vk::ShaderModule _vertexShader;
    vk::ShaderModule _fragShader;
    
    std::vector < std::function<void()>> trashCollector = {};
    glm::vec3 movementSpeed{};

    vkb::Instance* vkbInstance;
    vkb::Device* vkbDevice;

    float deltaT;

    uint32_t currentImgIndex{};
    int currentFrame{};
};

