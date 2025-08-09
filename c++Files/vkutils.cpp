#include "vkutils.h"
#include <iostream>
#include <fstream>


void vkutils::submitHelper(vk::CommandBuffer cmdBuffer, vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore,
	vk::PipelineStageFlagBits2 waitStageMask, vk::PipelineStageFlagBits2 signalStageMask,vk::Queue graphicsQueue,vk::Fence fence) {

	vk::SubmitInfo2 subInfo{};
	vk::CommandBufferSubmitInfo cmdInfo{};
	vk::SemaphoreSubmitInfo waitInfo{};
	vk::SemaphoreSubmitInfo signalInfo{};

	cmdInfo.setCommandBuffer(cmdBuffer).setDeviceMask(0);

	waitInfo.setSemaphore(waitSemaphore)
		.setValue(0)
		.setStageMask(waitStageMask)
		.setDeviceIndex(0);

	signalInfo.setSemaphore(signalSemaphore)
		.setValue(0)
		.setStageMask(signalStageMask)
		.setDeviceIndex(0);



	subInfo.setWaitSemaphoreInfos(waitInfo)
		.setSignalSemaphoreInfos(signalInfo)
		.setCommandBufferInfos(cmdInfo);

	graphicsQueue.submit2(subInfo, fence);
	}


void vkutils::transitionImage(vk::Image image, vk::CommandBuffer cmdBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {

	vk::ImageSubresourceRange subRange{};
	subRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(vk::RemainingArrayLayers)
		.setBaseMipLevel(0)
		.setLevelCount(vk::RemainingMipLevels)
		.setBaseArrayLayer(0);




	vk::ImageMemoryBarrier2 barrier{};
	barrier.setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
		.setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
		.setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
		.setDstAccessMask(vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite)
		.setImage(image)
		.setOldLayout(oldLayout)
		.setNewLayout(newLayout)
		.setSubresourceRange(subRange);




	vk::DependencyInfo depInfo{};
	depInfo.setImageMemoryBarrierCount(1).setPImageMemoryBarriers(&barrier);

	cmdBuffer.pipelineBarrier2(depInfo);


}

std::vector<char> vkutils::readFile(const std::string& fileName) {
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);//set pointer at end of file
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}
	size_t fileSize = (size_t)file.tellg();//gets length of file cuz its telling where the pointer is
	std::vector<char> buffer(fileSize);//allocate exact length
	file.seekg(0);//resets pointer to start of file
	file.read(buffer.data(), fileSize);//fills up vector
	file.close();
	std::cout << fileSize << "!!!!!!!!\n";
	return buffer;
}



vk::ShaderModule vkutils::createShaderModule(vk::Device _device, const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::ShaderModule shaderModule;
	shaderModule = _device.createShaderModule(createInfo);

	return shaderModule;
}
