#pragma once

#include <vulkan/vulkan_handles.hpp>



struct VmaAllocator_T;      using VmaAllocator = VmaAllocator_T*;
struct VmaAllocation_T;     using VmaAllocation = VmaAllocation_T*;
struct VmaAllocationInfo;
struct VmaAllocationCreateInfo;


namespace vkb {
    struct Instance;
    struct Device;
}

struct VulkanContext {
    const int NUM_OF_IMAGES = 2;
    int WIDTH = 800;
    int HEIGHT = 800;
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
    std::vector<vk::Image> _swapchainImages;
    std::vector<vk::ImageView> _swapchainImageViews;
    vk::Extent2D _swapchainExtent;

    vk::CommandPool _cmdPool;
    std::vector<vk::CommandBuffer> _cmdBuffers;
    vk::Pipeline _graphicsPipeline;
    vk::PipelineLayout _layout;
    std::vector<vk::Fence> _fences;
    std::array<vk::Semaphore, 2> _imageReadySemaphores;
    std::array<vk::Semaphore, 2> _renderFinishedSemaphores;

    vk::ShaderModule _vertexShader;
    vk::ShaderModule _fragShader;

    struct allocatedBuffer {
        vk::Buffer buffer{};
        VmaAllocation alloc{};
        VmaAllocationInfo allocInfo{};
        VmaAllocationCreateInfo allocCreateInfo{};
    };
    allocatedBuffer _stagingBuffer;
    allocatedBuffer _vertexBuffer;

    VmaAllocator _allocator;

    vkb::Instance* vkbInstance;
    vkb::Device* vkbDevice;



    uint32_t imageIndex{};
    int currentFrame{};
};

