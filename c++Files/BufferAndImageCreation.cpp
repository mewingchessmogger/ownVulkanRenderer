#include <pch.h>
#include <engine.h>
#include <Vulkancontext.h>
#include <BufferContext.h>
#include <WindowContext.h>

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

void Engine::forgeImages(vk::Format format, uint32_t width, uint32_t height, vk::ImageUsageFlags imageUsageIntent,
	int imageCount, std::vector<AllocatedImage>& images, vk::ImageAspectFlags aspectMask, std::string type,vk::ImageViewType viewtype,uint32_t arrLayers,bool cubeCompatible) {

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
		.setArrayLayers(arrLayers)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(imageUsageIntent)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(vk::ImageLayout::eUndefined);
		
	if (cubeCompatible) {
		imageInfo.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
	}
	

		
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
			.setLayerCount(arrLayers);

		info
			.setViewType(viewtype)
			.setImage(img.image)
			.setFormat(format)
			.setSubresourceRange(subRange);

		img.view = ctx->_device.createImageView(info);


		images.push_back(img);

	}
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

	vk::DeviceSize imageSize = texWidth * texHeight * 4 ;

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

	region.setImageSubresource(source).setImageExtent(vk::Extent3D{ static_cast<uint32_t>(texWidth),static_cast<uint32_t>(texHeight), 1 });												//
	//
	vkutils::transitionImage(btx->_txtImages[0].image, ctx->_cmdBuffers[0], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	ctx->_cmdBuffers[0].copyBufferToImage(stagingBuffer.buffer, btx->_txtImages[0].image, vk::ImageLayout::eTransferDstOptimal, region);	//
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
	stbi_image_free(pixels);
	vmaDestroyBuffer(btx->_allocator, stagingBuffer.buffer, stagingBuffer.alloc);
}


void Engine::createCubeImage() {

	struct pngInfo{
		stbi_uc* texData;
	};
	


	std::array<pngInfo,6> cubeSides{};

	int faceWidth, faceHeight, texChannels;
	
	
	
	cubeSides[0].texData = stbi_load("textures/cubemap/0084_east.png", &faceWidth, &faceHeight, &texChannels, STBI_rgb_alpha);
	cubeSides[1].texData = stbi_load("textures/cubemap/0084_west.png", &faceWidth, &faceHeight, &texChannels, STBI_rgb_alpha);
	
	cubeSides[2].texData = stbi_load("textures/cubemap/0084_top.png", &faceWidth, &faceHeight, &texChannels, STBI_rgb_alpha);
	cubeSides[3].texData = stbi_load("textures/cubemap/0084_bottom.png", &faceWidth, &faceHeight, &texChannels, STBI_rgb_alpha);

	cubeSides[4].texData = stbi_load("textures/cubemap/0084_north.png", &faceWidth, &faceHeight, &texChannels, STBI_rgb_alpha);
	cubeSides[5].texData = stbi_load("textures/cubemap/0084_south.png", &faceWidth, &faceHeight, &texChannels, STBI_rgb_alpha);

	vk::DeviceSize cubemapSize= faceWidth * faceHeight * 4 * 6; //4 is channels
	vk::DeviceSize sideSize = cubemapSize / 6; //rounded down 

	for (auto address : cubeSides) {

		if (!address.texData) {
			throw std::runtime_error("failed to load texture image!");
		}
		
	}


	//get size 

	AllocatedBuffer stagingBuffer{};
	createStagingBuffer(cubemapSize, stagingBuffer);
	uint8_t* stagingBase = static_cast<uint8_t*>(stagingBuffer.allocInfo.pMappedData);

	for (int i{}; i < cubeSides.size();i++) {
		memcpy(stagingBase + (i*sideSize), cubeSides[i].texData, static_cast<size_t>(sideSize));
	}


	vmaFlushAllocation(btx->_allocator, stagingBuffer.alloc, 0, cubemapSize);
	for (auto& side : cubeSides) {
		stbi_image_free(side.texData);
	}

	forgeImages(
		vk::Format::eR8G8B8A8Srgb, static_cast<uint32_t>(faceWidth),
		static_cast<uint32_t>(faceHeight),
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		1,
		btx->_cubeImages,
		vk::ImageAspectFlagBits::eColor,
		"texture",
		vk::ImageViewType::eCube,6,true);

	vk::CommandBufferBeginInfo beginInfo{};													//used for copying stage buffer to fast buffer in gpu mem
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);						//
	//
	ctx->_cmdBuffers[0].begin(beginInfo);														//
	//
	vk::BufferImageCopy region{};
	//v
	vk::ImageSubresourceLayers source{};
	source.setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(6);

	region.setImageSubresource(source).setImageExtent(vk::Extent3D{ static_cast<uint32_t>(faceWidth),static_cast<uint32_t>(faceHeight), 1 });												//
	//
	vkutils::transitionImage(btx->_cubeImages[0].image, ctx->_cmdBuffers[0], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	ctx->_cmdBuffers[0].copyBufferToImage(stagingBuffer.buffer, btx->_cubeImages[0].image, vk::ImageLayout::eTransferDstOptimal, region);	//
	//

	vkutils::transitionImage(btx->_cubeImages[0].image, ctx->_cmdBuffers[0], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
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
