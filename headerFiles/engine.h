#pragma once

#include <vector>
namespace vk {
    vk::ResultValue<uint32_t> ;
    vk::Semaphore ;
    vk::CommandBuffer;
    vk::CommandBufferAllocateInfo;
}

struct VulkanContext;
struct BufferContext;
struct WindowContext;
class Engine {
public:
    Engine();
    ~Engine();

    void run();// creates window, init, main loop, cleanup
    VulkanContext* ctx = nullptr;
    BufferContext* btx = nullptr;
    WindowContext* wtx = nullptr;

private:
    
    void initWindow();
    void initInstance();
    void initDevice();
    void initAllocator();
    void initCommands();
    void initSyncs();
    void initVertexBuffer();
    void initGraphicsPipeline();
   
    void createSwapchain();
    void recreateSwapchain();
    bool isValidSwapchain(vk::ResultValue<uint32_t> imgResult, vk::Semaphore imageReadySemaphore);
    void clearSwapchain();
    std::vector<vk::CommandBuffer> createCommandBuffers(vk::CommandBufferAllocateInfo allocInfo, uint32_t cmdBufferCount);
    
    
    void drawFrame();
    void cleanup();


    
};