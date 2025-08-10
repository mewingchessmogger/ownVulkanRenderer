#include "pch.h"
#include <engine.h>
#include "vkutils.h"
#include <Vulkancontext.h>
#include <BufferContext.h>
#include <WindowContext.h>
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

void Engine::initWindow(){
	

	if (!glfwInit()) {
		throw std::runtime_error("glfw init not working");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	wtx->window = glfwCreateWindow(ctx->WIDTH, ctx->HEIGHT, " ayo wassup", nullptr, nullptr);
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

	ctx->_instance = vk::Instance{ ctx->vkbInstance->instance};
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



	vkb::PhysicalDevice physicalDevice = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)

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
	vmaCreateAllocator(&allocInfo, &ctx->_allocator);


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



 std::vector<vk::CommandBuffer> Engine::createCommandBuffers(vk::CommandBufferAllocateInfo allocInfo, uint32_t cmdBufferCount) {
	 
	 std::vector<vk::CommandBuffer> cmdBuffers{};
	 cmdBuffers.resize(cmdBufferCount);
	 cmdBuffers = ctx->_device.allocateCommandBuffers(allocInfo);
	 return cmdBuffers;
	 //gotta destroy i tlayter...

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

void Engine::initVertexBuffer() {
	
	auto byteSize = btx->vertices.size() * sizeof(btx->vertices[0]);

	vk::BufferCreateInfo stagingInfo{};
	stagingInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc);


	vk::BufferCreateInfo dLocalInfo{};
	dLocalInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);


	ctx->_stagingBuffer.allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST; 						//keep pointer alive bit
	ctx->_stagingBuffer.allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	auto result = vmaCreateBuffer(ctx->_allocator, reinterpret_cast<VkBufferCreateInfo*>(&stagingInfo)
		, &ctx->_stagingBuffer.allocCreateInfo, reinterpret_cast<VkBuffer*>(&ctx->_stagingBuffer.buffer), &ctx->_stagingBuffer.alloc, &ctx->_stagingBuffer.allocInfo);


	ctx->_vertexBuffer.allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	ctx->_vertexBuffer.allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	
	
	auto result2 = vmaCreateBuffer(ctx->_allocator, reinterpret_cast<VkBufferCreateInfo*>(&dLocalInfo)
		, &ctx->_vertexBuffer.allocCreateInfo, reinterpret_cast<VkBuffer*>(&ctx->_vertexBuffer.buffer), &ctx->_vertexBuffer.alloc, nullptr);

	if (result != VK_SUCCESS || result2 != VK_SUCCESS) {
		throw std::runtime_error("vma alloc tweaking cuh");
	}


	std::memcpy(ctx->_stagingBuffer.allocInfo.pMappedData, btx->vertices.data(), byteSize);
	//flush it down the gpu drain so it gets visible for gpu 
	vmaFlushAllocation(ctx->_allocator, ctx->_stagingBuffer.alloc, 0, byteSize);


	//create commandbuffer

	//submit to queue


	vk::CommandBufferBeginInfo beginInfo{};													//used for copying stage buffer to fast buffer in gpu mem
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);						//
																							//
	ctx->_cmdBuffers[0].begin(beginInfo);														//
																							//
	vk::BufferCopy region{};																//
	region.setSize(byteSize);																//
																							//
	ctx->_cmdBuffers[0].copyBuffer(ctx->_stagingBuffer.buffer, ctx->_vertexBuffer.buffer, region);	//
																							//
	ctx->_cmdBuffers[0].end();																	//
																							//
	vk::SubmitInfo info{};																	//
	info.setCommandBuffers(ctx->_cmdBuffers[0]);												//
																							//
	ctx->_graphicsQueue.handle.submit(info);													//
	ctx->_graphicsQueue.handle.waitIdle();														//waitIdle cheap hack so it doesnt pot collide with buffer in drawFrame()
																							//
																							//

}



void Engine::initGraphicsPipeline() {
	

	vk::PipelineRenderingCreateInfo dynRenderInfo{};
	dynRenderInfo
		.setColorAttachmentCount(1)
		.setPColorAttachmentFormats(&ctx->_swapchainFormat);


	auto vertexCode = vkutils::readFile("shaders/vertex/firstVertex.spv");
	auto fragCode = vkutils::readFile("shaders/frag/firstFrag.spv");
	ctx->_vertexShader = vkutils::createShaderModule(ctx->_device,vertexCode);
	ctx->_fragShader = vkutils::createShaderModule(ctx->_device,fragCode);
	vk::PipelineShaderStageCreateInfo vertexInfo{};
	vertexInfo
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(ctx->_vertexShader)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo fragInfo{};
	fragInfo
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(ctx->_fragShader)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[2] = { vertexInfo,fragInfo };



	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	
	vk::VertexInputBindingDescription posBinding{};
	vk::VertexInputBindingDescription clrBinding{};
	vk::VertexInputAttributeDescription posAttrib{};
	vk::VertexInputAttributeDescription clrAttrib{};

	posBinding
		.setBinding(0)
		.setInputRate(vk::VertexInputRate::eVertex)
		.setStride(sizeof(btx->vertices[0]));

	posAttrib
		.setLocation(0)
		.setOffset(0)
		.setBinding(0)
		.setFormat(vk::Format::eR32G32B32Sfloat);
	clrAttrib
		.setLocation(1)
		.setOffset(sizeof(btx->vertices[0].pos))
		.setBinding(0)
		.setFormat(vk::Format::eR32G32B32Sfloat);

	vk::VertexInputBindingDescription bindings[] = { posBinding };
	vk::VertexInputAttributeDescription attribs[] = { posAttrib,clrAttrib };

	vertexInputInfo
		.setVertexBindingDescriptions(bindings)
		.setVertexAttributeDescriptions(attribs);

	vk::PipelineInputAssemblyStateCreateInfo assInfo{};
	assInfo.setTopology(vk::PrimitiveTopology::eTriangleList);


	vk::PipelineViewportStateCreateInfo viewportInfo{};
	vk::Rect2D scissor{};
	scissor
		.setOffset(vk::Offset2D{ 0,0 })
		.setExtent(ctx->_swapchainExtent);
	vk::Viewport viewport{};
	viewport
		.setX(0)
		.setY(0)
		.setWidth((float)ctx->_swapchainExtent.width)
		.setHeight((float)ctx->_swapchainExtent.height)
		.setMinDepth(0.0)
		.setMaxDepth(1.0);
	viewportInfo
		.setScissorCount(1)
		.setViewportCount(1);


	vk::PipelineRasterizationStateCreateInfo razInfo{};
	razInfo
		.setDepthClampEnable(vk::False)
		.setRasterizerDiscardEnable(vk::False)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(vk::False)
		.setDepthBiasSlopeFactor(1.0)
		.setLineWidth(1.0);


	vk::PipelineMultisampleStateCreateInfo multisampleInfo{};
	multisampleInfo
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setSampleShadingEnable(vk::False);


	vk::PipelineDepthStencilStateCreateInfo stencilInfo{};//nullptr


	vk::PipelineColorBlendStateCreateInfo blendInfo{};
	vk::PipelineColorBlendAttachmentState attachInfo{};
	attachInfo.colorWriteMask =
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;
	attachInfo.blendEnable = vk::True;
	attachInfo.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	attachInfo.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	attachInfo.colorBlendOp = vk::BlendOp::eAdd;
	attachInfo.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	attachInfo.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	attachInfo.alphaBlendOp = vk::BlendOp::eAdd;
	blendInfo.setAttachments(attachInfo);


	std::vector dynamicStates = {
	vk::DynamicState::eViewport,
	vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.setDynamicStates(dynamicStates);


	vk::PushConstantRange  range{};

	vk::PipelineLayoutCreateInfo layoutInfo{};
	
	ctx->_layout = ctx->_device.createPipelineLayout(layoutInfo);





	vk::GraphicsPipelineCreateInfo createPipelineInfo{};
	createPipelineInfo
		.setPNext(&dynRenderInfo)			// setting or dynrendering
		.setFlags(vk::PipelineCreateFlags{})// tweakiung pipelines set to nothing
		.setStages(shaderStages)// ctx->arr of shaderstageInfos
		.setPVertexInputState(&vertexInputInfo)// setting up buffers future
		.setPInputAssemblyState(&assInfo)// setting up that interpereting every three oints as a triangle here
		.setPViewportState(&viewportInfo)
		.setPRasterizationState(&razInfo)
		.setPMultisampleState(&multisampleInfo)
		.setPDepthStencilState(nullptr)
		.setPColorBlendState(&blendInfo)
		.setPDynamicState(&dynamicState)
		.setLayout(ctx->_layout)
		.setBasePipelineHandle(vk::Pipeline{})
		.setBasePipelineIndex(-1);

	ctx->_graphicsPipeline = ctx->_device.createGraphicsPipeline({}, createPipelineInfo).value;




}

void Engine::drawFrame() {
	
	static float counter{};
	counter += 0.01f;

	vk::Fence curFence[] = { ctx->_fences[ctx->currentFrame] };

	ctx->_device.waitForFences(1, curFence, vk::True, 1000000000);
	
	vk::Semaphore imageReadySemaph = ctx->_imageReadySemaphores[ctx->currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = ctx->_cmdBuffers[ctx->currentFrame];//this is indx currentFrame cuz the fence above 
	//guarantees buffer is done submitting, therefore choose buffer based on fences index"currentFrame"

	auto imgResult = ctx->_device.acquireNextImageKHR(ctx->_swapchain, 1000000000, imageReadySemaph);
	if (isValidSwapchain(imgResult, imageReadySemaph) == false) {
		return;
	}
	ctx->imageIndex = imgResult.value;
	ctx->_device.resetFences(curFence);

	//now were using imageindex as index, swapchainimages using it is obvious as that is the currently free from rendering
	//we use renderfinished semaphore to signal finished rendering the image, therefore it should be tighlty coupled to same index as swapchainimages
	// "use currently free imagiendex to the image and its semaphore, otherwise the problem might be you reuse semaphore that is already being used in waitsemaphore field in presentsrckhr eg race condition
	vk::Semaphore renderFinishedSemaph = ctx->_renderFinishedSemaphores[ctx->imageIndex];
	vk::Image curImage = ctx->_swapchainImages[ctx->imageIndex];

	cmdBuffer.reset();

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	cmdBuffer.begin(beginInfo);

	vk::ClearColorValue clr{};
	clr.setFloat32({ 0.0f, 0.0f, sinf(counter), 1.0f });

	vkutils::transitionImage(curImage, cmdBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	//begin dynrendering
	vk::RenderingAttachmentInfo attachInfo{};
	attachInfo
		.setImageView(ctx->_swapchainImageViews[ctx->imageIndex])
		.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setClearValue(clr)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);


	vk::RenderingInfoKHR renderInfo{};
	vk::Rect2D area;
	area.setOffset(vk::Offset2D(0, 0)).setExtent(ctx->_swapchainExtent);
	renderInfo
		.setRenderArea(area)
		.setLayerCount(1)
		.setColorAttachments(attachInfo);
	cmdBuffer.beginRendering(renderInfo);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, ctx->_graphicsPipeline);
	cmdBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, float(ctx->_swapchainExtent.width), float(ctx->_swapchainExtent.height), 0.0f, 1.0f));
	cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), ctx->_swapchainExtent));
	vk::DeviceSize offset{};

	cmdBuffer.bindVertexBuffers(0, ctx->_vertexBuffer.buffer, offset);
	cmdBuffer.draw(3, 1, 0, 0);
	cmdBuffer.endRendering();

	vkutils::transitionImage(curImage, cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

	cmdBuffer.end();

	vkutils::submitHelper(cmdBuffer, imageReadySemaph, renderFinishedSemaph, vk::PipelineStageFlagBits2::eColorAttachmentOutput
		,vk::PipelineStageFlagBits2::eAllGraphics, ctx->_graphicsQueue.handle, ctx->_fences[ctx->currentFrame]);

	vk::PresentInfoKHR presInfo{};
	presInfo.setWaitSemaphores({ renderFinishedSemaph })
		.setSwapchains(ctx->_swapchain)
		.setImageIndices({ ctx->imageIndex });
	ctx->_graphicsQueue.handle.presentKHR(presInfo);

	ctx->currentFrame = (ctx->currentFrame + 1) % ctx->NUM_OF_IMAGES;
}

void Engine::cleanup() {
	
	
	ctx->_device.waitIdle();


	ctx->_device.destroyShaderModule(ctx->_vertexShader);
	ctx->_device.destroyShaderModule(ctx->_fragShader);


	vmaDestroyBuffer(ctx->_allocator, ctx->_vertexBuffer.buffer, ctx->_vertexBuffer.alloc);
	vmaDestroyBuffer(ctx->_allocator, ctx->_stagingBuffer.buffer, ctx->_stagingBuffer.alloc);
	vmaDestroyAllocator(ctx->_allocator);

	ctx->_device.destroyPipelineLayout(ctx->_layout);
	ctx->_device.destroyPipeline(ctx->_graphicsPipeline);

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


void Engine::run() {
	initWindow();
	initInstance();
	initDevice();
	createSwapchain();
	initCommands();
	initSyncs();
	initVertexBuffer();
	initGraphicsPipeline();
	
	

	
		while (!glfwWindowShouldClose(wtx->window)) {
			glfwPollEvents();
			if (glfwGetKey(wtx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(wtx->window, GLFW_TRUE);
			}
			drawFrame();

		}
		cleanup(); 
	

}