#include "pch.h"
#include <engine.h>
#include <iostream>
#include "VulkanContext.h"  
#include "WindowContext.h"

void Engine::createSwapchain() {
	
	vkb::SwapchainBuilder swapchainBuilder(ctx->_chosenGPU, ctx->_device, ctx->_surface);
	ctx->_swapchainFormat = vk::Format::eB8G8R8A8Unorm;
	vk::SurfaceFormatKHR formatInfo{};
	formatInfo.format = ctx->_swapchainFormat;
	formatInfo.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

	vkb::Swapchain vkbSwapchain = swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_format(formatInfo)
		.set_desired_extent(ctx->WIDTH, ctx->HEIGHT)
		.set_required_min_image_count(ctx->NUM_OF_IMAGES)
		.set_desired_min_image_count(ctx->NUM_OF_IMAGES)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	ctx->_swapchain = vk::SwapchainKHR{ vkbSwapchain.swapchain };
	//_swapchainFormat = vk::Format{ vkbSwapchain.image_format };
	ctx->_swapchainExtent = vk::Extent2D{ vkbSwapchain.extent };

	//gotta convert the vals in vectors to hpp format
									//value() actually gets vector, otherwise you get vkresult struct iguess
	auto vkbImages = vkbSwapchain.get_images().value();
	auto vkbImageViews = vkbSwapchain.get_image_views().value();

	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
		ctx->_swapchainImages.push_back(vk::Image{ vkbImages[i] });
		ctx->_swapchainImageViews.push_back(vk::ImageView{ vkbImageViews[i] });
		std::cout << "WOAH" << "\n";
	}

	
	//createimages
	//allocate them
	//createimageviewsd

}
void Engine::clearSwapchain() {

	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
		ctx->_device.destroyImageView(ctx->_swapchainImageViews[i]);
	}
	ctx->_swapchainImages.clear();
	
	ctx->_swapchainImageViews.clear();
	
	ctx->_device.destroySwapchainKHR(ctx->_swapchain);
}

void Engine::rethinkSwapchain() {

	int width = 0, height = 0;
	glfwGetFramebufferSize(wtx->window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(wtx->window, &width, &height);
		glfwWaitEvents();
	}
	ctx->_device.waitIdle();

	clearSwapchain();
	
	
	ctx->WIDTH = width;
	ctx->HEIGHT = height;

	createSwapchain();
	

}


bool Engine::isValidSwapchain(vk::ResultValue<uint32_t> imgResult, vk::Semaphore imageReadySemaphore) {

	if (imgResult.result == vk::Result::eErrorOutOfDateKHR || ctx->frameBufferResized == true) {
		ctx->frameBufferResized = false;
		ctx->_device.destroySemaphore(imageReadySemaphore);
		ctx->_imageReadySemaphores[ctx->currentFrame] = ctx->_device.createSemaphore(vk::SemaphoreCreateInfo{});
		rethinkSwapchain();
		
		return false;
	}
	else if (imgResult.result != vk::Result::eSuccess && imgResult.result != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	return true;
}
