#include "vkutils.h"



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
