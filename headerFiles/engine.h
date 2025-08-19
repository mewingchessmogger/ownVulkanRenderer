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
    struct AllocatedBuffer;
    struct AllocatedImage;

struct WindowContext;
struct MousePos {
    double x;
    double y;

};
struct Camera {
    MousePos oldPos{};
    MousePos newPos{};
    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::vec3 dir = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float pitch{};
    float yaw{-90.0f};


};
class Engine {
public:
    Engine();
    ~Engine();

    void run();// creates window, init, main loop, cleanup
    VulkanContext* ctx = nullptr;
    BufferContext* btx = nullptr;
    WindowContext* wtx = nullptr;
    
    Camera camera{};

private:
    
    void initWindow();
    void initInstance();
    void initDevice();
    void initAllocator();
    void initCommands();
    void initSyncs();
    void createStagingBuffer(unsigned int long byteSize, AllocatedBuffer& stagingBuffer);
    void initVertexBuffer();
    void initUniformBuffer();
    void initDescriptors();
    void initGraphicsPipeline();
    void createTextureSampler();
    void forgeImages(vk::Format format,
                    uint32_t width,
                    uint32_t height, 
                    vk::ImageUsageFlags imageUsageIntent,
                    int imageCount, 
                    std::vector<AllocatedImage>& images, 
                    vk::ImageAspectFlags aspectMask, 
                    std::string type);
    


    void createRenderTargetImages();
    void rethinkDepthImages();
    void createDepthImages();
    void createTextureImage();
    void createSwapchain();
    void rethinkSwapchain();
    bool isValidSwapchain(vk::ResultValue<uint32_t> imgResult, vk::Semaphore imageReadySemaphore);
    void clearSwapchain();
    std::vector<vk::CommandBuffer> createCommandBuffers(vk::CommandBufferAllocateInfo allocInfo, uint32_t cmdBufferCount);
    
    void drawFrame();
    void cleanup();
    

    
};