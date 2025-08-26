#include "pch.h"
#include <engine.h>
#include <iostream>
#include "VulkanContext.h"  
#include "WindowContext.h"
#include "BufferContext.h"

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
		auto minStride = minimumSizeUBO;
		auto dummySize = minimumSizeUBO;
		while (currStride > dummySize) {
			dummySize += minStride;
		}
		currStride = dummySize;
	}
	//iter over num of frames times

	auto byteSize = currStride * (size_t)btx->NUM_OF_IMAGES;//final size of desired ubo adjusted for limitations of gpu, my gpu has a min 64 byte ubo
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
