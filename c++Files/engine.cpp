
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "glm/glm.hpp"
#include <iostream>

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#include "vkutils.h"
#include <engine.h>


struct VulkanContext {
	
	const int NUM_OF_IMAGES = 2;
	const bool bUseValidationLayers = true;
	vk::Instance _instance;		//instance for vulkan libs and that
	vk::DebugUtilsMessengerEXT _debugMessenger; // validation layers req
	vk::SurfaceKHR _surface; // the surface between windows and vulkan api
	vk::Device _device;			//logical device interacting with the gpu	
	vk::PhysicalDevice _chosenGPU; // handel to physical gpu used by device

	struct queueStruct {
		vk::Queue handle;//simple "all in one" queue
		uint32_t famIndex;//fam index
	};

	struct allocatedBuffer {
		vk::Buffer	buffer{};
		VmaAllocation alloc{};
		VmaAllocationInfo allocInfo{};
		VmaAllocationCreateInfo allocCreateInfo{};
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
	};
	std::vector<Vertex> vertices{
		{{0.0f, -0.5f,0.0f}, {1.0f,0.0f,0.0f}},//pos , color
		{{0.5f, 0.5f,0.0f}, {0.0f,1.0f,0.0f}},
		{{-0.5f, 0.5f,0.0f}, {0.0f,0.0f,1.0f}}
	};


	queueStruct _graphicsQueue;
	vkb::Instance vkbInstance;
	vkb::Device vkbDevice;
	VmaAllocator _allocator;
	vk::SwapchainKHR _swapchain;//actually a conjoined machine using vkimages,extents and formats for img rep
	vk::Format _swapchainFormat;//format vkimages and colorspace
	std::vector<vk::Image> _swapchainImages{};//actual mem in gpu rep something
	std::vector<vk::ImageView> _swapchainImageViews{};//viewer and writer in the vkimage
	vk::Extent2D _swapchainExtent;//how big image is
	GLFWwindow* window;


	vk::CommandPool _cmdPool;
	std::vector<vk::CommandBuffer>_cmdBuffers;
	vk::Pipeline _graphicsPipeline;
	vk::PipelineLayout _layout;
	std::vector<vk::Fence> _fences{};
	std::array<vk::Semaphore, 2> _imageReadySemaphores{};
	std::array<vk::Semaphore, 2> _renderFinishedSemaphores{};

	vk::ShaderModule _vertexShader;
	vk::ShaderModule _fragShader;

	allocatedBuffer _stagingBuffer;
	allocatedBuffer _vertexBuffer;

	uint32_t imageIndex{};
	int currentFrame{};
};


Engine::Engine() {
	ctx = new VulkanContext();
}

Engine::~Engine() {
	delete ctx;
}

void Engine::initWindow(){
	auto& c = *ctx;

	if (!glfwInit()) {
		throw std::runtime_error("glfw init not working");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	ctx->window = glfwCreateWindow(800, 800, " ayo wassup", nullptr, nullptr);

}

void Engine::initInstance() {
	auto& c = *ctx;
	vkb::InstanceBuilder instanceBuilder;
	uint32_t count{};
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);

	auto vkbInstanceBuilder = instanceBuilder.request_validation_layers(c.bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.enable_extensions(count, extensions)
		.build();
	c.vkbInstance = vkbInstanceBuilder.value();
	c._instance = vk::Instance{ c.vkbInstance.instance };
	c._debugMessenger = vk::DebugUtilsMessengerEXT{ c.vkbInstance.debug_messenger };

}


void Engine::initDevice() {
	auto& c = *ctx;
	//necessary right here otherwise crash
	glfwCreateWindowSurface(c._instance, c.window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&c._surface));

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector physicalDeviceSelector(c.vkbInstance);


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

		.set_surface(c._surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	c.vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	c._device = vk::Device{ c.vkbDevice.device };

	auto resQueue = c.vkbDevice.get_queue(vkb::QueueType::graphics);
	auto resFamilyIndex = c.vkbDevice.get_queue_index(vkb::QueueType::graphics);

	c._graphicsQueue.handle = vk::Queue{ resQueue.value() };
	c._graphicsQueue.famIndex = resFamilyIndex.value();
	c._chosenGPU = vk::PhysicalDevice{ physicalDevice.physical_device };

	VmaAllocatorCreateInfo allocInfo{};
	allocInfo.physicalDevice = c._chosenGPU;
	allocInfo.device = c._device;
	allocInfo.instance = c._instance;
	vmaCreateAllocator(&allocInfo, &c._allocator);


}


void Engine::initSwapchain() {
	auto& c = *ctx;
	vkb::SwapchainBuilder swapchainBuilder(c._chosenGPU, c._device, c._surface);
	c._swapchainFormat = vk::Format::eB8G8R8A8Unorm;
	vk::SurfaceFormatKHR formatInfo{};
	formatInfo.format = c._swapchainFormat;
	formatInfo.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

	vkb::Swapchain vkbSwapchain = swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_format(formatInfo)
		.set_desired_extent(800, 800)
		.set_required_min_image_count(c.NUM_OF_IMAGES)
		.set_desired_min_image_count(c.NUM_OF_IMAGES)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	c._swapchain = vk::SwapchainKHR{ vkbSwapchain.swapchain };
	//_swapchainFormat = vk::Format{ vkbSwapchain.image_format };
	c._swapchainExtent = vk::Extent2D{ vkbSwapchain.extent };

	//gotta convert the vals in vectors to hpp format
									//value() actually gets vector, otherwise you get vkresult struct iguess
	auto vkbImages = vkbSwapchain.get_images().value();
	auto vkbImageViews = vkbSwapchain.get_image_views().value();

	for (int i{}; i < c.NUM_OF_IMAGES; i++) {
		c._swapchainImages.push_back(vk::Image{ vkbImages[i] });
		c._swapchainImageViews.push_back(vk::ImageView{ vkbImageViews[i] });
		std::cout << "WOAH" << "\n";
	}

}


void Engine::initCommands() {
	auto& c = *ctx;
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.setQueueFamilyIndex(c._graphicsQueue.famIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	c._cmdPool = c._device.createCommandPool(cmdPoolInfo);
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.setCommandPool(c._cmdPool)
		.setCommandBufferCount(c.NUM_OF_IMAGES)
		.setLevel(vk::CommandBufferLevel::ePrimary);
	
	c._cmdBuffers.resize(c.NUM_OF_IMAGES);
	c._cmdBuffers = c._device.allocateCommandBuffers(allocInfo);
	for (auto i : c._cmdBuffers) {
		std::cout << i << "\n";
	}
}

void Engine::initSyncs() {
	auto& c = *ctx;
	vk::FenceCreateInfo createInfo{};
	createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
	vk::SemaphoreCreateInfo semaphInfo{};

	for (int i{}; i < 2; i++) {
		c._imageReadySemaphores[i] = c._device.createSemaphore(semaphInfo);
		c._renderFinishedSemaphores[i] = c._device.createSemaphore(semaphInfo);
		c._fences.push_back(c._device.createFence(createInfo));

	}



}

void Engine::initBuffers() {
	auto& c = *ctx;
	auto byteSize = c.vertices.size() * sizeof(c.vertices[0]);

	vk::BufferCreateInfo stagingInfo{};
	stagingInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc);


	vk::BufferCreateInfo dLocalInfo{};
	dLocalInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);


	c._stagingBuffer.allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST; 						//keep pointer alive bit
	c._stagingBuffer.allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	auto result = vmaCreateBuffer(c._allocator, reinterpret_cast<VkBufferCreateInfo*>(&stagingInfo)
		, &c._stagingBuffer.allocCreateInfo, reinterpret_cast<VkBuffer*>(&c._stagingBuffer.buffer), &c._stagingBuffer.alloc, &c._stagingBuffer.allocInfo);


	c._vertexBuffer.allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	c._vertexBuffer.allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	
	
	auto result2 = vmaCreateBuffer(c._allocator, reinterpret_cast<VkBufferCreateInfo*>(&dLocalInfo)
		, &c._vertexBuffer.allocCreateInfo, reinterpret_cast<VkBuffer*>(&c._vertexBuffer.buffer), &c._vertexBuffer.alloc, nullptr);

	if (result != VK_SUCCESS || result2 != VK_SUCCESS) {
		throw std::runtime_error("vma alloc tweaking cuh");
	}


	std::memcpy(c._stagingBuffer.allocInfo.pMappedData, c.vertices.data(), byteSize);
	//flush it down the gpu drain so it gets visible for gpu 
	vmaFlushAllocation(c._allocator, c._stagingBuffer.alloc, 0, byteSize);



	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	c._cmdBuffers[0].begin(beginInfo);

	vk::BufferCopy region{};
	region.setSize(byteSize);

	c._cmdBuffers[0].copyBuffer(c._stagingBuffer.buffer, c._vertexBuffer.buffer, region);

	c._cmdBuffers[0].end();

	vk::SubmitInfo info{};
	info.setCommandBuffers(c._cmdBuffers[0]);

	c._graphicsQueue.handle.submit(info);
	c._graphicsQueue.handle.waitIdle();



}



void Engine::initGraphicsPipeline() {
	auto& c = *ctx;

	vk::PipelineRenderingCreateInfo dynRenderInfo{};
	dynRenderInfo
		.setColorAttachmentCount(1)
		.setPColorAttachmentFormats(&c._swapchainFormat);


	auto vertexCode = vkutils::readFile("shaders/vertex/firstVertex.spv");
	auto fragCode = vkutils::readFile("shaders/frag/firstFrag.spv");
	c._vertexShader = vkutils::createShaderModule(c._device,vertexCode);
	c._fragShader = vkutils::createShaderModule(c._device,fragCode);
	vk::PipelineShaderStageCreateInfo vertexInfo{};
	vertexInfo
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(c._vertexShader)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo fragInfo{};
	fragInfo
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(c._fragShader)
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
		.setStride(sizeof(c.vertices[0]));

	posAttrib
		.setLocation(0)
		.setOffset(0)
		.setBinding(0)
		.setFormat(vk::Format::eR32G32B32Sfloat);
	clrAttrib
		.setLocation(1)
		.setOffset(sizeof(c.vertices[0].pos))
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
		.setExtent(c._swapchainExtent);
	vk::Viewport viewport{};
	viewport
		.setX(0)
		.setY(0)
		.setWidth((float)c._swapchainExtent.width)
		.setHeight((float)c._swapchainExtent.height)
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

	c._layout = c._device.createPipelineLayout(layoutInfo);





	vk::GraphicsPipelineCreateInfo createPipelineInfo{};
	createPipelineInfo
		.setPNext(&dynRenderInfo)			// setting or dynrendering
		.setFlags(vk::PipelineCreateFlags{})// tweakiung pipelines set to nothing
		.setStages(shaderStages)// c arr of shaderstageInfos
		.setPVertexInputState(&vertexInputInfo)// setting up buffers future
		.setPInputAssemblyState(&assInfo)// setting up that interpereting every three oints as a triangle here
		.setPViewportState(&viewportInfo)
		.setPRasterizationState(&razInfo)
		.setPMultisampleState(&multisampleInfo)
		.setPDepthStencilState(nullptr)
		.setPColorBlendState(&blendInfo)
		.setPDynamicState(&dynamicState)
		.setLayout(c._layout)
		.setBasePipelineHandle(vk::Pipeline{})
		.setBasePipelineIndex(-1);

	c._graphicsPipeline = c._device.createGraphicsPipeline({}, createPipelineInfo).value;




}

void Engine::drawFrame() {
	
	
	auto& c = *ctx;
	
	
	static float counter{};
	counter += 0.01f;


	//SYNC!!!!!!!!{
	
	vk::Fence curFence[] = { c._fences[c.currentFrame] };

	c._device.waitForFences(1, curFence, vk::True, 1000000000);
	c._device.resetFences(curFence);




	vk::Semaphore imageReadySemaph = c._imageReadySemaphores[c.currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = c._cmdBuffers[c.currentFrame];//this is indx currentFrame cuz the fence above 
	//guarantees buffer is done submitting, therefore choose buffer based on fences index"currentFrame"


	c.imageIndex = c._device.acquireNextImageKHR(c._swapchain, 1000000000, imageReadySemaph).value;


	//now were using imageindex as index, swapchainimages using it is obvious as that is the currently free from rendering
	//we use renderfinished semaphore to signal finished rendering the image, therefore it should be tighlty coupled to same index as swapchainimages
	// "use currently free imagiendex to the image and its semaphore, otherwise the problem might be you reuse semaphore that is already being used in waitsemaphore field in presentsrckhr eg race condition
	vk::Semaphore renderFinishedSemaph = c._renderFinishedSemaphores[c.imageIndex];
	vk::Image curImage = c._swapchainImages[c.imageIndex];

	//SYNC!!!!!!!!}


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
		.setImageView(c._swapchainImageViews[c.imageIndex])
		.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setClearValue(clr)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);



	//attachInfo.set
	vk::RenderingInfoKHR renderInfo{};
	vk::Rect2D area;
	area.setOffset(vk::Offset2D(0, 0)).setExtent(c._swapchainExtent);
	renderInfo
		.setRenderArea(area)
		.setLayerCount(1)
		.setColorAttachments(attachInfo);

	cmdBuffer.beginRendering(renderInfo);


	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, c._graphicsPipeline);
	//cmdBuffer.bindVertexBuffers();
	cmdBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, float(c._swapchainExtent.width), float(c._swapchainExtent.height), 0.0f, 1.0f));
	cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), c._swapchainExtent));
	vk::DeviceSize offset{};

	cmdBuffer.bindVertexBuffers(0, c._vertexBuffer.buffer, offset);
	cmdBuffer.draw(3, 1, 0, 0);

	cmdBuffer.endRendering();

	vkutils::transitionImage(curImage, cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);


	cmdBuffer.end();

	vkutils::submitHelper(cmdBuffer, imageReadySemaph, renderFinishedSemaph, vk::PipelineStageFlagBits2::eColorAttachmentOutput
		, vk::PipelineStageFlagBits2::eAllGraphics, c._graphicsQueue.handle, c._fences[c.currentFrame]);

	vk::PresentInfoKHR presInfo{};
	presInfo.setWaitSemaphores({ renderFinishedSemaph })
		.setSwapchains(c._swapchain)
		.setImageIndices({ c.imageIndex });

	c._graphicsQueue.handle.presentKHR(presInfo);

	c.currentFrame = (c.currentFrame + 1) % c.NUM_OF_IMAGES;
}

void Engine::cleanup() {
	auto& c = *ctx;


	c._device.waitIdle();


	c._device.destroyShaderModule(c._vertexShader);
	c._device.destroyShaderModule(c._fragShader);


	vmaDestroyBuffer(c._allocator, c._vertexBuffer.buffer, c._vertexBuffer.alloc);
	vmaDestroyBuffer(c._allocator, c._stagingBuffer.buffer, c._stagingBuffer.alloc);
	vmaDestroyAllocator(c._allocator);

	c._device.destroyPipelineLayout(c._layout);
	c._device.destroyPipeline(c._graphicsPipeline);

	for (int i{}; i < c.NUM_OF_IMAGES; i++) {
		c._device.destroyFence(c._fences[i]);
		c._device.destroySemaphore(c._renderFinishedSemaphores[i]);
		c._device.destroySemaphore(c._imageReadySemaphores[i]);

	}

	c._device.freeCommandBuffers(c._cmdPool, c._cmdBuffers);

	c._device.destroyCommandPool(c._cmdPool);
	for (int i{}; i < c.NUM_OF_IMAGES; i++) {

		c._device.destroyImageView(c._swapchainImageViews[i]);
	}

	c._device.destroySwapchainKHR(c._swapchain); // as vkimages are owned by swapchain they are killed with it
	c._instance.destroySurfaceKHR(c._surface);

	c._device.destroy();
	glfwDestroyWindow(c.window);
	glfwTerminate();

}


void Engine::run() {
	initWindow();
	initInstance();
	initDevice();
	initSwapchain();
	initCommands();
	initSyncs();
	initBuffers();
	initGraphicsPipeline();
	
	

	
		while (!glfwWindowShouldClose(ctx->window)) {
			glfwPollEvents();
			if (glfwGetKey(ctx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(ctx->window, GLFW_TRUE);
			}
			drawFrame();

		}
		cleanup(); 
	

}