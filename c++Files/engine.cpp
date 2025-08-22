
#include "pch.h"
#include <engine.h>
#include "vkutils.h"
#include <pipelineBuilder.h>
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
//


void Engine::initWindow(){
	

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

//save offset indexbuffer

void Engine::forgeImages(vk::Format format,uint32_t width, uint32_t height, vk::ImageUsageFlags imageUsageIntent,
						int imageCount, std::vector<AllocatedImage> &images, vk::ImageAspectFlags aspectMask,std::string type ) {

	vk::ImageCreateInfo imageInfo{};

	//vk::Format::eD32Sfloat;
	auto extent = vk::Extent3D{};

	extent.setWidth(width)
		.setHeight(height)
		.setDepth(1);
	uint32_t mipLevels = 1; //+ floor(log2(std::max(ctx->WIDTH, ctx->HEIGHT)));

	
	imageInfo
		.setImageType(vk::ImageType::e2D)
		.setFormat(format)
		.setExtent(extent)
		.setMipLevels(mipLevels)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(imageUsageIntent)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	VmaAllocationCreateInfo allocImgInfo{};
	allocImgInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocImgInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	for (int i{}; i < imageCount; i++) {
		AllocatedImage img{};
		img.format = format;
		img.extent = extent;

		auto result = vmaCreateImage(btx->_allocator, imageInfo, &allocImgInfo, reinterpret_cast<VkImage*>(&img.image), &img.alloc, &img.allocInfo);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed allocating type: " + type + " images");
		}
		vk::ImageViewCreateInfo info{};
		vk::ImageSubresourceRange subRange{};

		subRange
			.setAspectMask(aspectMask)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		
		info
			.setViewType(vk::ImageViewType::e2D)
			.setImage(img.image)
			.setFormat(format)
			.setSubresourceRange(subRange);

		img.view = ctx->_device.createImageView(info);
		
		
		images.push_back(img);

	}
}


void Engine::createDepthImages() {



	forgeImages(
		vk::Format::eD32Sfloat, ctx->WIDTH, ctx->HEIGHT,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		ctx->NUM_OF_IMAGES,
		btx->_depthImages,
		vk::ImageAspectFlagBits::eDepth,
		"depth");



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

	

	forgeImages(vk::Format::eR16G16B16A16Sfloat,
		ctx->WIDTH, ctx->HEIGHT,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
		ctx->NUM_OF_IMAGES,
		btx->_renderTargets,
		vk::ImageAspectFlagBits::eColor,
		"texture"
	);
}


void Engine::createTextureImage() {
	


	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	//get size 

	AllocatedBuffer stagingBuffer{};
	createStagingBuffer(imageSize, stagingBuffer);

	memcpy(stagingBuffer.allocInfo.pMappedData, pixels, static_cast<size_t>(imageSize));
	vmaFlushAllocation(btx->_allocator, stagingBuffer.alloc, 0, imageSize);


	forgeImages(
		vk::Format::eR8G8B8A8Srgb, static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight),
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		1,
		btx->_txtImages, 
		vk::ImageAspectFlagBits::eColor, 
		"texture");

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

void Engine::createStagingBuffer(unsigned int long byteSize, AllocatedBuffer& stagingBuffer) {
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

void Engine::loadModelsGLTF(){
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;

	std::string err;
	std::string warn;

	std::vector<std::string> modPaths{"models/dragon.gltf" };// , "models/stanfordBunny" 

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, modPaths[0]);

	if (!warn.empty()) {
		std::cout << "glTfw warning: " << warn << "\n";
	}
	if (!err.empty()) {
		std::cout << "glTfw error: " << err << "\n";
	}


	if (!ret) {
		throw std::runtime_error("gltf not working bruh");
	}
	BufferContext::indexDataModels data{};


	data.startVBO = btx->vertices.size();
	data.startIBO = btx->indices.size();



	for (const auto& mesh : model.meshes) {
		for (const auto& primitive : mesh.primitives) {
			uint32_t baseVertex = static_cast<uint32_t>(btx->vertices.size());


			// the vertex positions
			const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
			const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

			const float* posData = reinterpret_cast<const float*>(
				&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

			//the normals 
			const float* normalData = nullptr;
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				const tinygltf::Accessor& normalAccessor = model.accessors.at(primitive.attributes.at("NORMAL"));
				const tinygltf::BufferView& normalView = model.bufferViews[normalAccessor.bufferView];
				const tinygltf::Buffer& normalBuffer = model.buffers[normalView.buffer];
				normalData = reinterpret_cast<const float*>(
					&normalBuffer.data[normalView.byteOffset + normalAccessor.byteOffset]);
			}
			


			//get texture coordinates if exist
			const float* texCoordData = nullptr;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				const tinygltf::Accessor& texCoordAccessor = model.accessors.at(primitive.attributes.at("TEXCOORD_0"));
				const tinygltf::BufferView& texCoordView = model.bufferViews[texCoordAccessor.bufferView];
				const tinygltf::Buffer& texCoordBuffer = model.buffers[texCoordView.buffer];
				texCoordData = reinterpret_cast<const float*>(
					&texCoordBuffer.data[texCoordView.byteOffset + texCoordAccessor.byteOffset]);
			}

			for (size_t i{}; i < posAccessor.count; i++) {
				Vertex vertex{};

				vertex.pos = { posData[3 * i + 0], posData[3 * i + 1], posData[3 * i + 2] };
				
				vertex.normal = normalData ? glm::vec3(normalData[3 * i + 0], normalData[3 * i + 1], normalData[3 * i + 2]) : glm::vec3(0.0f);

				vertex.texCoord = texCoordData ? glm::vec2(texCoordData[2 * i + 0], texCoordData[2 * i + 1]) : glm::vec2(0.0f);

				btx->vertices.push_back(vertex);

			}

		


			//the indices
			const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
			const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

				
			// Handle different index component types
			if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
				const uint16_t* indices16 = reinterpret_cast<const uint16_t*>(indexData);
				for (size_t i = 0; i < indexAccessor.count; i++) {
					btx->indices.push_back(baseVertex + indices16[i]);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
				const uint32_t* indices32 = reinterpret_cast<const uint32_t*>(indexData);
				for (size_t i = 0; i < indexAccessor.count; i++) {
					btx->indices.push_back(baseVertex + indices32[i]);
				}
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
				const uint8_t* indices8 = reinterpret_cast<const uint8_t*>(indexData);
				for (size_t i = 0; i < indexAccessor.count; i++) {
					btx->indices.push_back(baseVertex + indices8[i]);
				}
			}


			





		}


	}

	data.endVBO = btx->vertices.size();
	data.endIBO = btx->indices.size();

	size_t id = btx->ComposerID.hasher("models/dragon.gltf");

	btx->ComposerID.modelMapper[id] = data;
	std::cout << "gltf!!!\n";
	std::cout << "model/dragon.gltf" << " ID: " << id << "--- VBO start/end: " << data.startVBO << ", " << data.endVBO << " ---IBO start/end: " << data.startIBO << ", " << data.endIBO << "\n";


}
void Engine::loadModels() {


	std::vector<std::string> modPaths{ "models/cube.obj", "models/stanfordBunny.obj","models/viking_room.obj"};// , "models/stanfordBunny" 
	btx->modPaths = modPaths;
	//btx->vertices.reserve(446228);
	//btx->indices.reserve(2640342);
	for (const auto& path : modPaths) {

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
			throw std::runtime_error(err + warn);
		}
		//save indexstart index end 
		//save vertex start vertex end
		BufferContext::indexDataModels data{};


		data.startVBO= btx->vertices.size();
		data.startIBO = btx->indices.size();
		


		for (auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};


				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (index.texcoord_index >= 0) {

					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };

				}
				
				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}
				

				if (btx->uniqueVertices.count(vertex) == 0) {
					btx->uniqueVertices[vertex] = static_cast<uint32_t>(btx->vertices.size() - data.startVBO);
					btx->vertices.push_back(vertex);

				}

				btx->indices.push_back(btx->uniqueVertices[vertex]);

			}
		}
		//save indices size
		btx->uniqueVertices.clear();//exteremly important
		
		data.endVBO = btx->vertices.size();
		data.endIBO = btx->indices.size();

		btx->offsetOfModels.push_back(data);//maybe not used
		size_t id = btx->ComposerID.hasher(path);
		
		btx->ComposerID.modelMapper[id] = data;
		
		std::cout << "OBJ!!: " << path << " ID: " << id << "--- VBO start/end: " << data.startVBO << ", " << data.endVBO << " ---IBO start/end: " << data.startIBO << ", " << data.endIBO << "\n";

	}


}

void Engine::initGameObjects() {
	// Object 1 - Center

	std::hash<std::string> hasher;

	btx->gameObjs.resize(4);
	
	btx->gameObjs[0].pos = { -1.0f, 0.2f, 0.0f };
	btx->gameObjs[0].rot = { glm::radians(180.0f), 0.0f, 0.0f };
	btx->gameObjs[0].scale = { 5.0f,0.2f,5.0f };
	btx->gameObjs[0].color = { 0.0f,0.3f,0.0f };
	btx->gameObjs[0].modelID = hasher("models/cube.obj");

	btx->gameObjs[1].pos = { 0.0f, 0.0f, 0.0f };
	btx->gameObjs[1].rot = { glm::radians(180.0f), glm::radians(180.0f), 0.0f };
	btx->gameObjs[1].scale = { 2.0f,2.0f,2.0f };
	btx->gameObjs[1].color = { 1.0f,1.0f,1.0f };
	btx->gameObjs[1].modelID = hasher("models/stanfordBunny.obj");

	// Object 2 - Left


	btx->gameObjs[2].pos = { -1.0f, 0.0f, 0.0f };
	btx->gameObjs[2].rot = { glm::radians(90.0f), 0.0f, 0.0f };
	btx->gameObjs[2].scale = { 0.5f,0.5f,0.5f };
	btx->gameObjs[2].usingTexture = 1;
	btx->gameObjs[2].modelID = hasher("models/viking_room.obj");


	btx->gameObjs[3].pos = { -1.0f, 0.2f, 1.0f };
	btx->gameObjs[3].rot = { glm::radians(180.0f), glm::radians(270.0f), 0.0f };
	btx->gameObjs[3].scale = { 4.0f,4.0f,4.0f };
	btx->gameObjs[3].color = { 1.0f,1.0f, 1.0f};
	btx->gameObjs[3].modelID = hasher("models/dragon.gltf");

	//btx->gameObjs[1].pos = { -1.0f, 0.0f, -1.0f };
	//btx->gameObjs[1].rot = { glm::radians(180.0f), glm::radians(180.0f), 0.0f };
	//btx->gameObjs[1].scale = { 3.0f, 3.0f,3.0f };
	for (auto& obj : btx->gameObjs) {
		std::cout << "ID: " << obj.modelID << "\n";
	}
}


void Engine::initVertexBuffer() {
	//size_t byteSize = loadModels("models/viking_roomb.obj");
	size_t byteSize = btx->vertices.size() * sizeof(btx->vertices[0]);
	
	
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


	AllocatedBuffer stagingBuffer{};
	createStagingBuffer(byteSize, stagingBuffer);


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

void Engine::initIndexBuffer() {
	//size_t byteSize = loadModels("models/viking_roomb.obj");
	size_t byteSizeIndex = btx->indices.size() * sizeof(uint32_t);


	vk::BufferCreateInfo dLocalInfo{};
	dLocalInfo
		.setSize(byteSizeIndex)
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);

	VmaAllocationCreateInfo vertexAllocCreateInfo{};

	vertexAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	vertexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;


	auto result1 = vmaCreateBuffer(btx->_allocator, reinterpret_cast<VkBufferCreateInfo*>(&dLocalInfo)
		, &vertexAllocCreateInfo, reinterpret_cast<VkBuffer*>(&btx->_indexBuffer.buffer), &btx->_indexBuffer.alloc, nullptr);

	if (result1 != VK_SUCCESS) {
		throw std::runtime_error("vma alloc tweaking cuh");
	}


	AllocatedBuffer stagingBuffer{};
	createStagingBuffer(byteSizeIndex, stagingBuffer);


	std::memcpy(stagingBuffer.allocInfo.pMappedData, btx->indices.data(), byteSizeIndex);
	//flush it down the gpu drain so it gets visible for gpu 
	vmaFlushAllocation(btx->_allocator, stagingBuffer.alloc, 0, byteSizeIndex);


	//create commandbuffer

	//submit to queue


	vk::CommandBufferBeginInfo beginInfo{};													//used for copying stage buffer to fast buffer in gpu mem
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);						//
	//
	ctx->_cmdBuffers[0].begin(beginInfo);														//
	//
	vk::BufferCopy region{};																//
	region.setSize(byteSizeIndex);																//
	//
	ctx->_cmdBuffers[0].copyBuffer(stagingBuffer.buffer, btx->_indexBuffer.buffer, region);	//
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
	vk::DeviceSize uboStructSize = sizeof(btx->dataUBO);                 // 192
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
						//iter over num of frames times
	
		auto byteSize = currStride * (size_t)btx->MAX_OBJECTS * (size_t)btx->NUM_OF_IMAGES;//final size of desired ubo adjusted for limitations of gpu, my gpu has a min 64 byte ubo
		btx->strideUBO = currStride;//trackkiings stride so could make one big ubo for all objects per frame

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
			, &allocCreateInfo, reinterpret_cast<VkBuffer*>(&btx->_GameObjUBO.buffer), &btx->_GameObjUBO.alloc, &btx->_GameObjUBO.allocInfo);

		if (result2 != VK_SUCCESS) {
			throw std::runtime_error("vma uniform alloc tweaking cuh");
		}

}

void Engine::initDescriptors() {

	vk::DescriptorSetLayoutCreateInfo UBOSetLayout{};
	vk::DescriptorSetLayoutCreateInfo samplerSetLayout{};

	vk::DescriptorSetLayoutBinding uboBind{};
	uboBind
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	

	vk::DescriptorSetLayoutBinding samplerBind{};
	samplerBind
		.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);


	UBOSetLayout.setBindings(uboBind);
	samplerSetLayout.setBindings(samplerBind);//same var just changing bind
	

	ctx->_descSetLayoutUBO = ctx->_device.createDescriptorSetLayout(UBOSetLayout);
	ctx->_descSetLayoutSampler = ctx->_device.createDescriptorSetLayout(samplerSetLayout);


	vk::DescriptorPoolCreateInfo poolInfo{};
	vk::DescriptorPoolSize uboSizes{};
	vk::DescriptorPoolSize samplerSizes{};

	uboSizes
		.setDescriptorCount(1)//how many of this type of descritpor pool fits
		.setType(vk::DescriptorType::eUniformBufferDynamic);

	samplerSizes
		.setDescriptorCount(1)//how many of this type of descritpor pool fits
		.setType(vk::DescriptorType::eCombinedImageSampler);

	std::array<vk::DescriptorPoolSize,2> poolSizes = { uboSizes,samplerSizes };

	poolInfo
		.setMaxSets(2)//how many "DESCRIPTOR SETS" can be allcoated maximum
		.setPoolSizes(poolSizes);

	ctx->_descPool = ctx->_device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo setAlloc{};
	
	std::vector<vk::DescriptorSetLayout> layouts{ctx->_descSetLayoutUBO,ctx->_descSetLayoutSampler};
	
	setAlloc
		.setDescriptorPool(ctx->_descPool)
		.setDescriptorSetCount(2)//how many "DESCRIPTOR SETS" we choose to allocate
		.setSetLayouts(layouts);


	ctx->_descSets = ctx->_device.allocateDescriptorSets(setAlloc);
	std::cout << ctx->_descSets.size() << " amount of desc sets\n";
	

	vk::DescriptorBufferInfo info1{};
	info1
		.setBuffer(btx->_GameObjUBO.buffer)
		.setOffset(0)
		.setRange(sizeof(btx->dataUBO));
		


	vk::WriteDescriptorSet writes[2];

	
		writes[0]
			.setBufferInfo(info1)
			.setDstBinding(0)
			.setDstSet(ctx->_descSets[0])
			.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
			.setDstArrayElement(0)
			.setDescriptorCount(1);

	


	vk::DescriptorImageInfo imgInfo{};
	imgInfo
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setImageView(btx->_txtImages[0].view)
		.setSampler(ctx->_sampler);

	writes[1]
		.setImageInfo(imgInfo)
		.setDstBinding(0)
		.setDstSet(ctx->_descSets[1])
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDstArrayElement(0)
		.setDescriptorCount(1);

	

	ctx->_device.updateDescriptorSets(writes,{});


}

void Engine::initGraphicsPipeline() {


	auto bindings = btx->getVertBindings();
	auto attributes = btx->getVertAttributes();
	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { ctx->_descSetLayoutUBO,ctx->_descSetLayoutSampler };



	PipelineBuilder plb{};
	plb
		.setDevice(ctx->_device)
		.setDynRendering(1, { ctx->_swapchainFormat }, btx->_depthImages[0].format)
		.setShaderStages("shaders/vertex/firstVertex.spv", "shaders/frag/firstFrag.spv")
		.setVertexInputInfo(bindings, attributes)
		.setAssemblyInfo()
		.setScissorAndViewport(ctx->_swapchainExtent)
		.setRasterizerInfo()
		.setMultiSampling()//nuffin right now
		.setBlendState()
		.setDynState()
		.setPCRange(sizeof(BufferContext::pushConstants), 0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.setDescLayout(descriptorSetLayouts)
		.createPipeLineLayout()
		.setDepthStencilState()
		.createPipeline();

	ctx->_graphicsPipeline = plb.getPipeline();
	ctx->_layout = plb.pipeLineLayout;

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
	clr.setFloat32({ 0.0f, 0.0f, 0.3f, 1.0f });

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



	//camera
	glm::mat4 view(1.0f);

	 // camera position
	glm::vec3 eye = camera.eye;
	glm::mat4 model(1.0f);
	glm::mat4 proj(1.0f);
	static glm::vec3 direction{};  // where the camera looks
	
	direction = camera.dir;
	view = glm::lookAt(eye, eye+direction, camera.up);

	btx->dataUBO.view = view;
	//drawing
	

	proj = glm::perspective(glm::radians(45.0f), (float)ctx->WIDTH / (float)ctx->HEIGHT, 0.1f, 100.0f);
	//model  = glm::translate(model, glm::vec3(0.0,1.0f,-0.5f));
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));   // rotate around Z
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1));   // rotate around Z
	btx->dataUBO.proj = proj;




	//cmdBuffer.pushConstants(ctx->_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &model);
	uint8_t* base = static_cast<uint8_t*>(btx->_GameObjUBO.allocInfo.pMappedData);

	cmdBuffer.bindVertexBuffers(0, btx->_vertexBuffer.buffer, offset);
	cmdBuffer.bindIndexBuffer(btx->_indexBuffer.buffer, offset, vk::IndexType::eUint32);


	for (int i{}; i < btx->gameObjs.size();i++) {
		auto& obj = btx->gameObjs[i];

		BufferContext::indexDataModels modData = btx->ComposerID.modelMapper[obj.modelID];

		BufferContext::pushConstants pc{};
		
		

		TransformUBO ubo{};
		ubo.proj = proj;
		ubo.view = view;
		
		pc.model = obj.getModelMatrix();
		pc.color = obj.color;
		pc.useTexture = obj.usingTexture;

		size_t offset = ((ctx->currentFrame * btx->MAX_OBJECTS + i) * btx->strideUBO);
		std::memcpy(base + offset, &ubo, sizeof(ubo));

		uint32_t dynOffset = static_cast<uint32_t>(offset);
		cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			ctx->_layout, 0, 2, ctx->_descSets.data(), 1, &dynOffset);
		
		cmdBuffer.pushConstants(ctx->_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0, sizeof(BufferContext::pushConstants),&pc);

		auto indexCount = static_cast<uint32_t>(modData.endIBO - modData.startIBO);
		cmdBuffer.drawIndexed(
			indexCount, //how many indices
			1, //instances
			modData.startIBO,//where to start in ibo 
			modData.startVBO, //where to start in vbo
			0);//instance shit 

	}
	
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

	ctx->_device.destroyDescriptorSetLayout(ctx->_descSetLayoutUBO);
	ctx->_device.destroyDescriptorPool(ctx->_descPool);//sets are destroyed within pool 

	ctx->_device.destroyImageView(btx->_renderTargets[0].view);
	ctx->_device.destroyImageView(btx->_renderTargets[1].view);


	for (int i{}; i < ctx->NUM_OF_IMAGES; i++) {
	vmaDestroyImage(btx->_allocator, btx->_renderTargets[i].image, btx->_renderTargets[i].alloc);
	vmaDestroyImage(btx->_allocator, btx->_depthImages[i].image, btx->_depthImages[i].alloc);

	}

	vmaDestroyImage(btx->_allocator, btx->_txtImages[0].image, btx->_txtImages[0].alloc);


	vmaDestroyBuffer(btx->_allocator, btx->_vertexBuffer.buffer, btx->_vertexBuffer.alloc);
	vmaDestroyBuffer(btx->_allocator, btx->_indexBuffer.buffer, btx->_indexBuffer.alloc);

	vmaDestroyBuffer(btx->_allocator, btx->_GameObjUBO.buffer, btx->_GameObjUBO.alloc);
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




void mouseHandler(Camera &c,float dT) {


	float xoffset = c.newPos.x - c.oldPos.x;
	float yoffset = c.newPos.y - c.oldPos.y;

	float sens = 10.0f *dT;


	c.yaw += sens * xoffset;
	c.pitch += sens * yoffset;

	c.pitch = std::clamp(c.pitch, -89.0f, 89.0f);

	c.dir.x = cos(glm::radians(c.yaw)) * cos(glm::radians(c.pitch));
	c.dir.z = sin(glm::radians(c.yaw)) * cos(glm::radians(c.pitch));
	c.dir.y = sin(glm::radians(c.pitch));
	c.dir = glm::normalize(c.dir);

	c.oldPos = c.newPos;



}

void inputHandler(Camera* camera, float dT,WindowContext* wtx) {
	
	const float cameraSpeed = 1.0f * dT; // 

	if (glfwGetKey(wtx->window, GLFW_KEY_C) == GLFW_PRESS) {//forwrad
		camera->dir.y = 0.0f;
		camera->eye += cameraSpeed * camera->dir;

	} 
	if (glfwGetKey(wtx->window, GLFW_KEY_X) == GLFW_PRESS) {//BACK!!!!
		camera->dir.y = 0.0f;
		camera->eye -= cameraSpeed * camera->dir;
	}
	if (glfwGetKey(wtx->window, GLFW_KEY_Z) == GLFW_PRESS)
		camera->eye -= glm::normalize(glm::cross(camera->dir, camera->up)) * cameraSpeed;//left
	if (glfwGetKey(wtx->window, GLFW_KEY_V) == GLFW_PRESS)
		camera->eye += glm::normalize(glm::cross(camera->dir, camera->up)) * cameraSpeed;//right

	if (glfwGetKey(wtx->window, GLFW_KEY_LEFT) == GLFW_PRESS)
		camera->eye.y -= cameraSpeed;
	if (glfwGetKey(wtx->window, GLFW_KEY_END) == GLFW_PRESS)
		camera->eye.y += cameraSpeed;



}
void majorHandler(Camera &camera,WindowContext *wtx) {
	static float lastTime = glfwGetTime();

	float currTime = glfwGetTime();

	float dT = currTime - lastTime;

	lastTime = currTime;

	static bool gWasPressed = false;
	static bool toggleCursor = false;

	int state = glfwGetKey(wtx->window, GLFW_KEY_G);

	if (state == GLFW_PRESS && !gWasPressed) {
		toggleCursor = !toggleCursor;

		if (!toggleCursor) {
			glfwSetInputMode(wtx->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			double cx, cy;
			glfwGetCursorPos(wtx->window, &cx, &cy);
			camera.oldPos = { cx, cy };
		}
		else {
			glfwSetInputMode(wtx->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		}

	}


	if (!toggleCursor) {
		inputHandler(&camera, dT, wtx);
		mouseHandler(camera, dT);
	}

	gWasPressed = (state == GLFW_PRESS);

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
	loadModelsGLTF();
	loadModels();
	initGameObjects();
	initVertexBuffer();
	initIndexBuffer();
	initUniformBuffer();
	initDescriptors();
	initGraphicsPipeline();
	
	

	
		while (!glfwWindowShouldClose(wtx->window)) {
			glfwPollEvents();

			glfwGetCursorPos(wtx->window, &camera.newPos.x, &camera.newPos.y);
			if (glfwGetKey(wtx->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(wtx->window, GLFW_TRUE);
			}
			
			majorHandler(camera,wtx);
			
			drawFrame();

		}
		cleanup(); 
	

}