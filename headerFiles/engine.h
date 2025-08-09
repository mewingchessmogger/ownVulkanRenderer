#pragma once
struct VulkanContext;


class Engine {
public:
    Engine();
    ~Engine();

    void run();// creates window, init, main loop, cleanup

private:
    VulkanContext* ctx = nullptr;
    void initWindow();
    void initInstance();
    void initDevice();
    void initAllocator();
    void initSwapchain();
    void initCommands();
    void initSyncs();
    void initBuffers();
    void initGraphicsPipeline();
    void drawFrame();
    void cleanup();

    
};