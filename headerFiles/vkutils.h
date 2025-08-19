#pragma once
#include <vulkan/vulkan.hpp>   // not just vulkan_handles.hpp
#include <iostream>
#include <fstream>

namespace vkutils {

	 void submitHelper(vk::CommandBuffer cmdBuffer, vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore,
		vk::PipelineStageFlagBits2 waitStageMask, vk::PipelineStageFlagBits2 signalStageMask, vk::Queue graphicsQueue, vk::Fence fence);
	
	 void transitionImage(vk::Image image, vk::CommandBuffer cmdBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

	 std::vector<char> readFile(const std::string& fileName);
	 
	 vk::ShaderModule createShaderModule(vk::Device _device, const std::vector<char>& code );
}


