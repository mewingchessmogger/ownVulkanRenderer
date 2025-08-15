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
void Engine::createDepthImages() {

	vk::ImageCreateInfo depthImageInfo{};

	btx->_depthFormat = vk::Format::eD32Sfloat;
	auto extent = vk::Extent3D{};

	extent.setWidth(ctx->WIDTH)
		.setHeight(ctx->HEIGHT)
		.setDepth(1);
	uint32_t mipLevels = 1; //+ floor(log2(std::max(ctx->WIDTH, ctx->HEIGHT)));

	btx->_depthExtent = extent;
	depthImageInfo
		.setImageType(vk::ImageType::e2D)
		.setFormat(btx->_depthFormat)
		.setExtent(btx->_depthExtent)
		.setMipLevels(mipLevels)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	VmaAllocationCreateInfo allocImgInfo{};
	allocImgInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocImgInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
		allocatedImage img{};

		auto result = vmaCreateImage(btx->_allocator, depthImageInfo, &allocImgInfo, reinterpret_cast<VkImage*>(&img.image), &img.alloc, &img.allocInfo);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed allocating depth images");
		}
		vk::ImageViewCreateInfo info{};
		vk::ImageSubresourceRange subRange{};
		subRange
			.setAspectMask(vk::ImageAspectFlagBits::eDepth)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		info
			.setViewType(vk::ImageViewType::e2D)
			.setImage(img.image)
			.setFormat(btx->_depthFormat)
			.setSubresourceRange(subRange);

		img.view = ctx->_device.createImageView(info);
		btx->_depthImages.push_back(img);

	}
}

void Engine::rethinkDepthImages() {
	 

	//destroy old alloc 
	for (auto& allocImg : btx->_depthImages) {
		vmaDestroyImage(btx->_allocator, allocImg.image, allocImg.alloc);
	}
	btx->_depthImages.clear();

	createDepthImages();


}


void Engine::createRenderTargetImages() {

	vk::ImageCreateInfo imgCreateInfo{};

	btx->_renderTargetFormat = vk::Format::eR16G16B16A16Sfloat;
	auto extent = vk::Extent3D{};
	extent.setWidth(ctx->WIDTH)
		.setHeight(ctx->HEIGHT)
		.setDepth(1);
	uint32_t mipLevels = 1;

	btx->_renderTargetExtent = extent;
	imgCreateInfo
		.setImageType(vk::ImageType::e2D)
		.setFormat(btx->_renderTargetFormat)
		.setExtent(btx->_renderTargetExtent)
		.setMipLevels(mipLevels)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	VmaAllocationCreateInfo allocImgInfo{};
	allocImgInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
		allocatedImage img{};


		auto result = vmaCreateImage(btx->_allocator, imgCreateInfo, &allocImgInfo, reinterpret_cast<VkImage*>(&img.image), &img.alloc, &img.allocInfo);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed allocating rendertarget image");
		}
		vk::ImageViewCreateInfo info{};
		info
			.setViewType(vk::ImageViewType::e2D)
			.setImage(img.image)
			.setFormat(btx->_renderTargetFormat)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor,0,1,0,1 });
		
		img.view = ctx->_device.createImageView(info);
		
		btx->_renderTargets.push_back(img);
	
	
	}



}


void Engine::createTextureSampler() {


	vk::SamplerCreateInfo samplerInfo{};

	samplerInfo
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(vk::True)
		.setMaxAnisotropy(ctx->_chosenGPU.getProperties().limits.maxSamplerAnisotropy)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setUnnormalizedCoordinates(vk::False)
		.setCompareEnable(vk::False)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMipLodBias(0.0f)
		.setMinLod(0.0f)
		.setMaxLod(0.0f);
		


	ctx->_sampler = ctx->_device.createSampler(samplerInfo);


}

void Engine::createTextureImage() {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/sonne.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	//get size 
	
	allocatedBuffer stagingBuffer{};
	createStagingBuffer(imageSize, stagingBuffer);

	memcpy(stagingBuffer.allocInfo.pMappedData, pixels, static_cast<size_t>(imageSize));
	vmaFlushAllocation(btx->_allocator, stagingBuffer.alloc, 0, imageSize);



	vk::ImageCreateInfo imgCreateInfo{};
	
	btx->_txtFormat = vk::Format::eR8G8B8A8Srgb;
	auto extent = vk::Extent3D{};
	extent.setWidth(texWidth)
		.setHeight(texHeight)
		.setDepth(1);			
	uint32_t mipLevels = 1; //+ floor(log2(std::max(ctx->WIDTH, ctx->HEIGHT)));

	btx->_txtExtent = extent;
	imgCreateInfo
		.setImageType(vk::ImageType::e2D)
		.setFormat(btx->_txtFormat)
		.setExtent(btx->_txtExtent)
		.setMipLevels(mipLevels)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	VmaAllocationCreateInfo allocImgInfo{};
	allocImgInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	for (int i{}; i < 1; i++) {
		allocatedImage img{};

		auto result = vmaCreateImage(btx->_allocator, imgCreateInfo, &allocImgInfo, reinterpret_cast<VkImage*>(&img.image), &img.alloc, &img.allocInfo);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed allocating txt images");
		}
		vk::ImageViewCreateInfo info{};
		info
			.setViewType(vk::ImageViewType::e2D)
			.setImage(img.image)
			.setFormat(btx->_txtFormat)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor,0,1,0,1 });

		img.view = ctx->_device.createImageView(info);
	    btx->_txtImages.push_back(img);
	}
	

	vk::CommandBufferBeginInfo beginInfo{};													//used for copying stage buffer to fast buffer in gpu mem
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);						//
	//
	ctx->_cmdBuffers[0].begin(beginInfo);														//
	//
	vk::BufferImageCopy region{};
	//v
	vk::ImageSubresourceLayers source{};
	source.setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(1);

	region.setImageSubresource(source).setImageExtent(vk::Extent3D{static_cast<uint32_t>(texWidth),static_cast<uint32_t>(texHeight), 1});												//
	//
	vkutils::transitionImage(btx->_txtImages[0].image, ctx->_cmdBuffers[0], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);


	ctx->_cmdBuffers[0].copyBufferToImage(stagingBuffer.buffer,btx->_txtImages[0].image,vk::ImageLayout::eTransferDstOptimal,region);	//
	//

	vkutils::transitionImage(btx->_txtImages[0].image, ctx->_cmdBuffers[0], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	ctx->_cmdBuffers[0].end();																	//
	//
	vk::SubmitInfo info{};																	//
	info.setCommandBuffers(ctx->_cmdBuffers[0]);												//
	//
	ctx->_graphicsQueue.handle.submit(info);													//
	ctx->_graphicsQueue.handle.waitIdle();														//waitIdle cheap hack so it doesnt pot collide with buffer in drawFrame()
	ctx->_cmdBuffers[0].reset();
	vmaDestroyBuffer(btx->_allocator, stagingBuffer.buffer, stagingBuffer.alloc);
}

void Engine::createStagingBuffer(unsigned int long byteSize, allocatedBuffer& stagingBuffer) {
	vk::BufferCreateInfo stagingInfo{};
	stagingInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc);

	VmaAllocationCreateInfo stageAllocCreateInfo{};
	stageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST; 						//keep pointer alive bit
	stageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	auto result = vmaCreateBuffer(btx->_allocator, reinterpret_cast<VkBufferCreateInfo*>(&stagingInfo)
				, &stageAllocCreateInfo, reinterpret_cast<VkBuffer*>(&stagingBuffer.buffer), &stagingBuffer.alloc, &stagingBuffer.allocInfo);	

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed creation of staging buffer");
	}


}



void Engine::initVertexBuffer() {
	
	auto byteSize = btx->vertices.size() * sizeof(btx->vertices[0]);
	
	allocatedBuffer stagingBuffer{};
	createStagingBuffer(byteSize, stagingBuffer);

	
	vk::BufferCreateInfo dLocalInfo{};
	dLocalInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);

	VmaAllocationCreateInfo vertexAllocCreateInfo{};

	vertexAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vertexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	
	
	auto result1 = vmaCreateBuffer(btx->_allocator, reinterpret_cast<VkBufferCreateInfo*>(&dLocalInfo)
		, &vertexAllocCreateInfo, reinterpret_cast<VkBuffer*>(&btx->_vertexBuffer.buffer), &btx->_vertexBuffer.alloc, nullptr);

	if (result1 != VK_SUCCESS) {
		throw std::runtime_error("vma alloc tweaking cuh");
	}

	

	std::memcpy(stagingBuffer.allocInfo.pMappedData, btx->vertices.data(), byteSize);
	//flush it down the gpu drain so it gets visible for gpu 
	vmaFlushAllocation(btx->_allocator, stagingBuffer.alloc, 0, byteSize);


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
	ctx->_cmdBuffers[0].copyBuffer(stagingBuffer.buffer, btx->_vertexBuffer.buffer, region);	//
																//
	ctx->_cmdBuffers[0].end();																	//
																							//
	vk::SubmitInfo info{};																	//
	info.setCommandBuffers(ctx->_cmdBuffers[0]);												//
																							//
	ctx->_graphicsQueue.handle.submit(info);													//
	ctx->_graphicsQueue.handle.waitIdle();														//waitIdle cheap hack so it doesnt pot collide with buffer in drawFrame()
	ctx->_cmdBuffers[0].reset();																		//
																							//
	vmaDestroyBuffer(btx->_allocator, stagingBuffer.buffer, stagingBuffer.alloc);

}

void Engine::initUniformBuffer() {
	vk::DeviceSize uboStructSize = sizeof(btx->dataUBO);                   // 192
	vk::DeviceSize minimumSizeUBO = ctx->_chosenGPU.getProperties().limits.minUniformBufferOffsetAlignment;
	std::cout << minimumSizeUBO << " alignmin\n";
	size_t currStride = uboStructSize;

	//check if ubosize is less than align
	if (currStride <= minimumSizeUBO) {
		currStride = minimumSizeUBO;

	}
	else {
		auto minStride= minimumSizeUBO;
		auto dummySize = minimumSizeUBO;
		while (currStride >dummySize) {
			dummySize+= minStride;
		}
		currStride = dummySize;
	}

	auto byteSize = currStride * (size_t)ctx->NUM_OF_IMAGES;//final size of desired ubo adjusted for limitations of gpu, my gpu has a min 64 byte ubo
	btx->strideUBO = currStride;

	vk::BufferCreateInfo uniformInfo{};
	uniformInfo
		.setSize(byteSize)
		.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
		.setSharingMode(vk::SharingMode::eExclusive);

	VmaAllocationCreateInfo allocCreateInfo{};

	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

	//bits below indicate firstly that we want to sequentially write to the gpu through cpu AND we will probs not read gpu mem, seconde gives us a undying pointer to gpu mem
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	
	//force these flags so theres no funny business, they might be included from above vma bits buts its not ensured
	allocCreateInfo.requiredFlags =
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	auto result2 = vmaCreateBuffer(btx->_allocator, reinterpret_cast<VkBufferCreateInfo*>(&uniformInfo)//								allocinfo is not same as allocCreateInfo
		, &allocCreateInfo, reinterpret_cast<VkBuffer*>(&btx->_uniformBuffer.buffer), &btx->_uniformBuffer.alloc, &btx->_uniformBuffer.allocInfo);

	if (result2 != VK_SUCCESS) {
		throw std::runtime_error("vma uniform alloc tweaking cuh");
	}

}

void Engine::initDescriptors() {

	vk::DescriptorSetLayoutCreateInfo descLayout{};

	vk::DescriptorSetLayoutBinding uboBind{};
	uboBind
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	descLayout.setBindings(uboBind);

	ctx->_descLayoutUBO = ctx->_device.createDescriptorSetLayout(descLayout);



	vk::DescriptorSetLayoutBinding samplerBind{};
	samplerBind
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);


	descLayout.setBindings(samplerBind);//same var just changing bind
	
	ctx->_descLayoutSampler = ctx->_device.createDescriptorSetLayout(descLayout);


	vk::DescriptorPoolCreateInfo poolInfo{};
	vk::DescriptorPoolSize uboSizes{};
	vk::DescriptorPoolSize samplerSizes{};

	uboSizes
		.setDescriptorCount(2)//how many "DESCRIPTORS" can pool fit
		.setType(vk::DescriptorType::eUniformBuffer);

	samplerSizes
		.setDescriptorCount(1)//how many "DESCRIPTORS" can pool fit
		.setType(vk::DescriptorType::eCombinedImageSampler);

	std::array<vk::DescriptorPoolSize,2> poolSizes = { uboSizes,samplerSizes };

	poolInfo
		.setMaxSets(3)//how many "DESCRIPTOR SETS" can be allcoated maximum
		.setPoolSizes(poolSizes);

	ctx->_descPool = ctx->_device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo setAlloc{};
	
	std::array<vk::DescriptorSetLayout,3> layouts{ctx->_descLayoutUBO,ctx->_descLayoutUBO,ctx->_descLayoutSampler};
	
	setAlloc
		.setDescriptorPool(ctx->_descPool)
		.setDescriptorSetCount(3)//how many "DESCRIPTOR SETS" we choose to allocate
		.setSetLayouts(layouts);


	ctx->_descSets = ctx->_device.allocateDescriptorSets(setAlloc);
	std::cout << ctx->_descSets.size() << " amount of desc sets\n";
	

	vk::DescriptorBufferInfo info1{};
	info1
		.setBuffer(btx->_uniformBuffer.buffer)
		.setOffset(0)
		.setRange(sizeof(btx->dataUBO));
		

	vk::DescriptorBufferInfo info2{};
	info2 
		.setBuffer(btx->_uniformBuffer.buffer)
		.setOffset(btx->strideUBO)
		.setRange(sizeof(btx->dataUBO));

	

	std::array<vk::DescriptorBufferInfo, 2> bufferInfos = { info1,info2 };

	vk::WriteDescriptorSet writes[3];

	for (int i{}; i < 2; i++) {
		writes[i]
			.setBufferInfo(bufferInfos[i])
			.setDstBinding(0)
			.setDstSet(ctx->_descSets[i])
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDstArrayElement(0)
			.setDescriptorCount(1);

	}


	vk::DescriptorImageInfo imgInfo{};
	imgInfo
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setImageView(btx->_txtImages[0].view)
		.setSampler(ctx->_sampler);

	writes[2]
		.setImageInfo(imgInfo)
		.setDstBinding(0)
		.setDstSet(ctx->_descSets[2])
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDstArrayElement(0)
		.setDescriptorCount(1);

	

	ctx->_device.updateDescriptorSets(writes,{});


}

void Engine::initGraphicsPipeline() {
	

	vk::PipelineRenderingCreateInfo dynRenderInfo{};
	dynRenderInfo
		.setColorAttachmentCount(1)
		.setPColorAttachmentFormats(&ctx->_swapchainFormat)
		.setDepthAttachmentFormat(btx->_depthFormat);
		

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
	vk::VertexInputBindingDescription texCoordBinding{};

	vk::VertexInputAttributeDescription posAttrib{};
	vk::VertexInputAttributeDescription clrAttrib{};
	vk::VertexInputAttributeDescription texCoordAttrib{};

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
		.setOffset(offsetof(Vertex, color))
		.setBinding(0)
		.setFormat(vk::Format::eR32G32B32Sfloat);


	texCoordAttrib
		.setLocation(2)
		.setOffset(offsetof(Vertex,texCoord))
		.setBinding(0)
		.setFormat(vk::Format::eR32G32Sfloat);
	


	vk::VertexInputBindingDescription bindings[] = { posBinding };
	vk::VertexInputAttributeDescription attribs[] = { posAttrib,clrAttrib,texCoordAttrib };

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


	vk::PushConstantRange  pcRange{};
	pcRange
		.setOffset(0)
		.setSize(sizeof(glm::mat4))
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	vk::PipelineLayoutCreateInfo layoutInfo{};
	std::array<vk::DescriptorSetLayout, 3> descriptorSetLayouts = {ctx->_descLayoutUBO,ctx->_descLayoutUBO,ctx->_descLayoutSampler};

	layoutInfo
		.setSetLayouts(descriptorSetLayouts)
		.setPushConstantRanges(pcRange);

	ctx->_layout = ctx->_device.createPipelineLayout(layoutInfo);
	

	vk::PipelineDepthStencilStateCreateInfo depthInfo{};
	depthInfo
		.setDepthTestEnable(vk::True)
		.setDepthWriteEnable(vk::True)
		.setDepthCompareOp(vk::CompareOp::eLess)//if the incoming depth is "less" than stored depth we store the incoming fragment otherwise discard
		.setDepthBoundsTestEnable(vk::False)//some obscure thing
		.setStencilTestEnable(vk::False);//what is stencils duhguh
		//.setFront() //these two need to be set if stenciltest is enabled!
		//.setBack()

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
		.setPDepthStencilState(&depthInfo)
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
		//swapchain recreated in func above
		rethinkDepthImages();
		return;
	}
	auto imageIndex = imgResult.value;
	ctx->_device.resetFences(curFence);
	
	//now we are using imageindex as index, swapchainimages using it is obvious as that is the currently free from rendering
	//we use renderfinished semaphore to signal finished rendering the image, therefore it should be tighlty coupled to same index as swapchainimages
	// "use currently free imagiendex to the image and its semaphore, otherwise the problem might be you reuse semaphore that is already being used in waitsemaphore field in presentsrckhr eg race condition
	vk::Semaphore renderFinishedSemaph = ctx->_renderFinishedSemaphores[imageIndex];
	vk::Image curImage = ctx->_swapchainImages[imageIndex];
	
	cmdBuffer.reset();

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	cmdBuffer.begin(beginInfo);

	vk::ClearColorValue clr{};	//sinf(counter)
	clr.setFloat32({ 0.0f, 0.0f, 1.0f, 1.0f });

	vkutils::transitionImage(curImage, cmdBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
	
	
	//setup for dynrenderin

	vk::RenderingAttachmentInfo attachInfo{};
	attachInfo
		.setImageView(ctx->_swapchainImageViews[imageIndex])
		.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setClearValue(clr)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);
		

	vk::RenderingAttachmentInfo depthInfo{};
	depthInfo
		.setImageView(btx->_depthImages[imageIndex].view)
		.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
		.setClearValue(vk::ClearDepthStencilValue{1,0})
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);

	vk::RenderingInfoKHR renderInfo{};
	
	vk::Rect2D area;
	area.setOffset(vk::Offset2D(0, 0)).setExtent(ctx->_swapchainExtent);
	renderInfo
		.setRenderArea(area)
		.setLayerCount(1)
		.setColorAttachments(attachInfo)
		.setPDepthAttachment(&depthInfo);

	cmdBuffer.beginRendering(renderInfo);
	
	
	//drawing shit 


	

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, ctx->_graphicsPipeline);
	cmdBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, float(ctx->_swapchainExtent.width), float(ctx->_swapchainExtent.height), 0.0f, 1.0f));
	cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), ctx->_swapchainExtent));
	vk::DeviceSize offset{};
	cmdBuffer.bindVertexBuffers(0, btx->_vertexBuffer.buffer, offset);



	//camera
	glm::mat4 view(1.0f);

	glm::vec3 eye = glm::vec3(0.0f, 0.0f, 2.0f);  // camera position
	glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);  // where the camera looks
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // camera's up direction
	view = glm::lookAt(eye, center, up);
	btx->dataUBO.view = view;
	//drawing
	glm::mat4 model(1.0f);
	glm::mat4 proj(1.0f);

	proj = glm::perspective(glm::radians(45.0f), static_cast<float>(ctx->WIDTH / ctx->HEIGHT), 0.1f, 100.0f);
	//model  = glm::translate(model, glm::vec3(0.0, ,0.0f));
	model = glm::rotate(model, glm::radians(60.0f), glm::vec3(1, 0, 0));   // rotate around Z

	model = glm::rotate(model, counter*2, glm::vec3(0, 0, 1));   // rotate around Z
	cmdBuffer.pushConstants(ctx->_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &model);

	
	//btx->dataUBO.model = model;
	btx->dataUBO.proj = proj;
	uint8_t* base = static_cast<uint8_t*>(btx->_uniformBuffer.allocInfo.pMappedData);
	std::memcpy(base + ctx->currentFrame * btx->strideUBO, &btx->dataUBO, sizeof(btx->dataUBO));
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		ctx->_layout, 0, 3, ctx->_descSets.data(), 0, nullptr);
	cmdBuffer.draw(6, 1, 0, 0);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0, sinf(counter), 0));
	model = glm::rotate(model, glm::radians(60.0f), glm::vec3(1, 0, 0));   // rotate around e
	cmdBuffer.pushConstants(ctx->_layout, vk::ShaderStageFlagBits::eVertex, 0,sizeof(glm::mat4), &model);
	cmdBuffer.draw(6, 1, 0, 0);


	cmdBuffer.endRendering();

	vkutils::transitionImage(curImage, cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

	cmdBuffer.end();

	vkutils::submitHelper(cmdBuffer, imageReadySemaph, renderFinishedSemaph, vk::PipelineStageFlagBits2::eColorAttachmentOutput
		,vk::PipelineStageFlagBits2::eAllGraphics, ctx->_graphicsQueue.handle, ctx->_fences[ctx->currentFrame]);

	vk::PresentInfoKHR presInfo{};
	presInfo.setWaitSemaphores({ renderFinishedSemaph })
		.setSwapchains(ctx->_swapchain)
		.setImageIndices({ imageIndex });
	ctx->_graphicsQueue.handle.presentKHR(presInfo);

	ctx->currentFrame = (ctx->currentFrame + 1) % ctx->NUM_OF_IMAGES;
}



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

	ctx->_device.destroyDescriptorSetLayout(ctx->_descLayoutUBO);
	ctx->_device.destroyDescriptorPool(ctx->_descPool);//sets are destroyed within pool 

	ctx->_device.destroyImageView(btx->_renderTargets[0].view);
	ctx->_device.destroyImageView(btx->_renderTargets[1].view);


	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
	vmaDestroyImage(btx->_allocator, btx->_renderTargets[i].image, btx->_renderTargets[i].alloc);
	vmaDestroyImage(btx->_allocator, btx->_depthImages[i].image, btx->_depthImages[i].alloc);

	}

	vmaDestroyImage(btx->_allocator, btx->_txtImages[0].image, btx->_txtImages[0].alloc);


	vmaDestroyBuffer(btx->_allocator, btx->_vertexBuffer.buffer, btx->_vertexBuffer.alloc);
	vmaDestroyBuffer(btx->_allocator, btx->_uniformBuffer.buffer, btx->_uniformBuffer.alloc);
	vmaDestroyAllocator(btx->_allocator);

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
	createDepthImages();
	createRenderTargetImages();
	createTextureImage();
	createTextureSampler();
	initVertexBuffer();
	initUniformBuffer();
	initDescriptors();
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