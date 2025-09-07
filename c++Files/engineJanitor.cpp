#include "pch.h"
#include <engine.h>
#include <Vulkancontext.h>
#include <BufferContext.h>
#include <WindowContext.h>

void Engine::cleanup() {


	ctx->_device.waitIdle();
	//run trashcollector

	ctx->trashCollector.push_back([this]() {
		ctx->_device.destroyShaderModule(ctx->_vertexShader);
		ctx->_device.destroyShaderModule(ctx->_fragShader);
		});
	for (auto& i : ctx->trashCollector) {
		i();
	}

	ctx->_device.destroyDescriptorSetLayout(ctx->_descSetLayoutUBO);
	ctx->_device.destroyDescriptorPool(ctx->_descPool);//sets are destroyed within pool 

	

	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
		ctx->_device.destroyImageView(btx->_renderTargets[i].view);
		ctx->_device.destroyImageView(btx->_depthImages[i].view);

		vmaDestroyImage(btx->_allocator, btx->_renderTargets[i].image, btx->_renderTargets[i].alloc);
		vmaDestroyImage(btx->_allocator, btx->_depthImages[i].image, btx->_depthImages[i].alloc);

	}

	ctx->_device.destroyImageView(btx->_cubeImages[0].view);
	ctx->_device.destroyImageView(btx->_txtImages[0].view);

	vmaDestroyImage(btx->_allocator, btx->_txtImages[0].image, btx->_txtImages[0].alloc);
	vmaDestroyImage(btx->_allocator, btx->_cubeImages[0].image, btx->_cubeImages[0].alloc);


	vmaDestroyBuffer(btx->_allocator, btx->_vertexBuffer.buffer, btx->_vertexBuffer.alloc);
	vmaDestroyBuffer(btx->_allocator, btx->_indexBuffer.buffer, btx->_indexBuffer.alloc);

	vmaDestroyBuffer(btx->_allocator, btx->_GameObjUBO.buffer, btx->_GameObjUBO.alloc);
	vmaDestroyAllocator(btx->_allocator);
	


	ctx->_device.destroyPipelineLayout(ctx->_layout);
	ctx->_device.destroyPipeline(ctx->_graphicsPipeline);
	ctx->_device.destroyPipelineLayout(ctx->_skyboxLayout);
	ctx->_device.destroyPipeline(ctx->_skyboxPipeline);

	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
		ctx->_device.destroyFence(ctx->_fences[i]);
		ctx->_device.destroySemaphore(ctx->_renderFinishedSemaphores[i]);
		ctx->_device.destroySemaphore(ctx->_imageReadySemaphores[i]);

	}

	ctx->_device.freeCommandBuffers(ctx->_cmdPool, ctx->_cmdBuffers);

	ctx->_device.destroyCommandPool(ctx->_cmdPool);
	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {

		ctx->_device.destroyImageView(ctx->_swapchainImageViews[i]);
	}

	ctx->_device.destroySwapchainKHR(ctx->_swapchain); // as vkimages are owned by swapchain they are killed with it
	ctx->_instance.destroySurfaceKHR(ctx->_surface);

	ctx->_device.destroy();
	glfwDestroyWindow(wtx->window);
	glfwTerminate();

}

