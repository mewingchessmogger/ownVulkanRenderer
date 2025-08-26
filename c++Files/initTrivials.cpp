#include "pch.h"
#include <engine.h>
#include <iostream>
#include "VulkanContext.h"  
#include "WindowContext.h"
#include "BufferContext.h"
Engine::Engine() {
	ctx = new VulkanContext();
	btx = new BufferContext();
	wtx = new WindowContext();
}



Engine::~Engine() {
	delete ctx->vkbInstance;
	delete ctx->vkbDevice;
	delete ctx;
	ctx = nullptr;
	delete btx;
	btx = nullptr;
	delete wtx;
	wtx = nullptr;

}

static void frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto context = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	context->ctx->frameBufferResized = true;
}
//


void Engine::initWindow() {


	if (!glfwInit()) {
		throw std::runtime_error("glfw init not working");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	wtx->window = glfwCreateWindow(ctx->WIDTH, ctx->HEIGHT, " ayo wassup", nullptr, nullptr);
	glfwSetInputMode(wtx->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(wtx->window, this);
	glfwSetFramebufferSizeCallback(wtx->window, frameBufferResizeCallback);

}

void Engine::initInstance() {

	vkb::InstanceBuilder instanceBuilder;
	uint32_t count{};
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);

	auto vkbInstanceBuilder = instanceBuilder.request_validation_layers(ctx->bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.enable_extensions(count, extensions)
		.build();
	ctx->vkbInstance = new vkb::Instance(vkbInstanceBuilder.value());

	ctx->_instance = vk::Instance{ ctx->vkbInstance->instance };
	ctx->_debugMessenger = vk::DebugUtilsMessengerEXT{ ctx->vkbInstance->debug_messenger };

}


void Engine::initDevice() {

	//necessary right here otherwise crash
	glfwCreateWindowSurface(ctx->_instance, wtx->window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&ctx->_surface));

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector physicalDeviceSelector(*ctx->vkbInstance);


	//vulkan 1.3 features
	vk::PhysicalDeviceVulkan13Features features{};
	features.dynamicRendering = true;
	features.synchronization2 = true;

	//vulkan 1.2 features
	vk::PhysicalDeviceVulkan12Features features12{};
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vk::PhysicalDeviceFeatures feat{};
	feat.samplerAnisotropy = true;


	vkb::PhysicalDevice physicalDevice = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_required_features(feat)
		.set_surface(ctx->_surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	ctx->vkbDevice = new vkb::Device{ deviceBuilder.build().value() };

	// Get the VkDevice handle used in the rest of a vulkan application
	ctx->_device = vk::Device{ ctx->vkbDevice->device };

	auto resQueue = ctx->vkbDevice->get_queue(vkb::QueueType::graphics);
	auto resFamilyIndex = ctx->vkbDevice->get_queue_index(vkb::QueueType::graphics);

	ctx->_graphicsQueue.handle = vk::Queue{ resQueue.value() };
	ctx->_graphicsQueue.famIndex = resFamilyIndex.value();
	ctx->_chosenGPU = vk::PhysicalDevice{ physicalDevice.physical_device };

	VmaAllocatorCreateInfo allocInfo{};
	allocInfo.physicalDevice = ctx->_chosenGPU;
	allocInfo.device = ctx->_device;
	allocInfo.instance = ctx->_instance;
	vmaCreateAllocator(&allocInfo, &btx->_allocator);


}

void Engine::initCommands() {

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.setQueueFamilyIndex(ctx->_graphicsQueue.famIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	ctx->_cmdPool = ctx->_device.createCommandPool(cmdPoolInfo);
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.setCommandPool(ctx->_cmdPool)
		.setCommandBufferCount(ctx->NUM_OF_IMAGES)
		.setLevel(vk::CommandBufferLevel::ePrimary);

	ctx->_cmdBuffers.resize(ctx->NUM_OF_IMAGES);
	ctx->_cmdBuffers = ctx->_device.allocateCommandBuffers(allocInfo);
	for (auto i : ctx->_cmdBuffers) {
		std::cout << i << "\n";
	}

}




void Engine::initSyncs() {

	vk::FenceCreateInfo createInfo{};
	createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
	vk::SemaphoreCreateInfo semaphInfo{};

	for (int i{}; i < 2; i++) {
		ctx->_imageReadySemaphores[i] = ctx->_device.createSemaphore(semaphInfo);
		ctx->_renderFinishedSemaphores[i] = ctx->_device.createSemaphore(semaphInfo);
		ctx->_fences.push_back(ctx->_device.createFence(createInfo));

	}



}