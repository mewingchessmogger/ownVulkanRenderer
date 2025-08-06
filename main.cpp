#include <iostream>
#include <fstream>
#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
constexpr int NUM_OF_IMAGES = 2;
constexpr bool bUseValidationLayers = true;
vk::Instance _instance;		//instance for vulkan libs and that
vk::DebugUtilsMessengerEXT _debugMessenger; // validation layers req
vk::SurfaceKHR _surface; // the surface between windows and vulkan api
vk::Device _device;			//logical device interacting with the gpu	
vk::PhysicalDevice _chosenGPU; // handel to physical gpu used by device

struct queueStruct {
	vk::Queue handle;//simple "all in one" queue
	uint32_t famIndex;//fam index
};

queueStruct _graphicsQueue;

vkb::Instance vkbInstance;
vkb::Device vkbDevice;

vk::SwapchainKHR _swapchain;//actually a conjoined machine using vkimages,extents and formats for img rep
vk::Format _swapchainFormat;//format vkimages and colorspace
std::vector<vk::Image> _swapchainImages{};//actual mem in gpu rep something
std::vector<vk::ImageView> _swapchainImageViews{};//viewer and writer in the vkimage
vk::Extent2D _swapchainExtent;//how big image is
GLFWwindow* window;


vk::CommandPool _cmdPool;
std::vector<vk::CommandBuffer>_cmdBuffers;
vk::Pipeline _graphicsPipeline;
std::vector<vk::Fence> _fences{};
std::array<vk::Semaphore, 2> _imageReadySemaphores{};
std::array<vk::Semaphore, 2> _renderFinishedSemaphores{};

vk::ShaderModule _vertexShader;
vk::ShaderModule _fragShader;

uint32_t imageIndex{};
int currentFrame{};

void initWindow() {


	if (!glfwInit()) {
		throw std::runtime_error("glfw init not working");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(800, 800, " ayo wassup", nullptr, nullptr);


}
void initInstance() {

	vkb::InstanceBuilder instanceBuilder;
	uint32_t count{};
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);

	auto vkbInstanceBuilder = instanceBuilder.request_validation_layers(bUseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1,3,0)
		.enable_extensions(count, extensions)
		.build();
	vkbInstance = vkbInstanceBuilder.value();
	_instance = vk::Instance{ vkbInstance.instance };
	_debugMessenger = vk::DebugUtilsMessengerEXT{ vkbInstance.debug_messenger };

}
void initDeviceAndGPU() {

	//necessary right here otherwise crash
	glfwCreateWindowSurface(_instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_surface));

	//use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector physicalDeviceSelector(vkbInstance);


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
		
		.set_surface(_surface)
		.select()
		.value();


	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	_device = vk::Device{ vkbDevice.device };

	auto resQueue = vkbDevice.get_queue(vkb::QueueType::graphics);
	auto resFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics);

	_graphicsQueue.handle = vk::Queue{ resQueue.value() };
	_graphicsQueue.famIndex = resFamilyIndex.value();
	_chosenGPU = vk::PhysicalDevice{ physicalDevice.physical_device };

	

}
void initSwapchain() {

	vkb::SwapchainBuilder swapchainBuilder(_chosenGPU, _device, _surface);
	_swapchainFormat = vk::Format::eB8G8R8A8Unorm;
	vk::SurfaceFormatKHR formatInfo{};
	formatInfo.format = _swapchainFormat;
	formatInfo.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

	vkb::Swapchain vkbSwapchain = swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
												  .set_desired_format(formatInfo)
												  .set_desired_extent(800, 800)
												  .set_required_min_image_count(NUM_OF_IMAGES)
												  .set_desired_min_image_count(NUM_OF_IMAGES)
												  .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
												  .build()
												  .value();

	_swapchain = vk::SwapchainKHR{ vkbSwapchain.swapchain };
	//_swapchainFormat = vk::Format{ vkbSwapchain.image_format };
	_swapchainExtent = vk::Extent2D{ vkbSwapchain.extent };

	//gotta convert the vals in vectors to hpp format
									//value() actually gets vector, otherwise you get vkresult struct iguess
	auto vkbImages = vkbSwapchain.get_images().value();
	auto vkbImageViews = vkbSwapchain.get_image_views().value();

	for (int i{}; i < NUM_OF_IMAGES; i++) {
		_swapchainImages.push_back(vk::Image{ vkbImages[i] });
		_swapchainImageViews.push_back(vk::ImageView{ vkbImageViews[i] });
		std::cout << "WOAH" << "\n";
	}

}
void initCommandBuffers() {
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.setQueueFamilyIndex(_graphicsQueue.famIndex)
			   .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	_cmdPool = _device.createCommandPool(cmdPoolInfo);
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.setCommandPool(_cmdPool)
			 .setCommandBufferCount(NUM_OF_IMAGES)
			 .setLevel(vk::CommandBufferLevel::ePrimary);

	_cmdBuffers.resize(NUM_OF_IMAGES);
	_cmdBuffers = _device.allocateCommandBuffers(allocInfo);
	for (auto i : _cmdBuffers) {
		std::cout << i << "\n";
	}

}

std::vector<char> readFile(const std::string& fileName) {
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



vk::ShaderModule createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	vk::ShaderModule shaderModule;
	shaderModule = _device.createShaderModule(createInfo);
	
	return shaderModule;
}


void initSyncs(){
	
	vk::FenceCreateInfo createInfo{};
	createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
	vk::SemaphoreCreateInfo semaphInfo{};
	
	for (int i{}; i < 2; i++) {
		_imageReadySemaphores[i] = _device.createSemaphore(semaphInfo);
		_renderFinishedSemaphores[i] = _device.createSemaphore(semaphInfo);
		_fences.push_back(_device.createFence(createInfo));

	}

	

}



void initGraphicsPipeline() {

	
	vk::PipelineRenderingCreateInfo dynRenderInfo{};
	dynRenderInfo
		.setColorAttachmentCount(1)
		.setPColorAttachmentFormats(&_swapchainFormat);
			     

	auto vertexCode = readFile("shaders/vertex/firstVertex.spv");
	auto fragCode = readFile("shaders/frag/firstFrag.spv");
	_vertexShader = createShaderModule(vertexCode);
	_fragShader = createShaderModule(fragCode);
	vk::PipelineShaderStageCreateInfo vertexInfo{};
	vertexInfo
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(_vertexShader)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo fragInfo{};
	fragInfo
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(_fragShader)
		.setPName("main");
	vk::PipelineShaderStageCreateInfo shaderStages[2] = { vertexInfo,fragInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};



	vk::PipelineInputAssemblyStateCreateInfo assInfo{};
	assInfo.setTopology(vk::PrimitiveTopology::eTriangleList);


	vk::PipelineViewportStateCreateInfo viewportInfo{};
	vk::Rect2D scissor{};
	scissor
		.setOffset(vk::Offset2D{ 0,0 })
		.setExtent(_swapchainExtent);
	vk::Viewport viewport{};
	viewport
		.setX(0)
		.setY(0)
		.setWidth((float)_swapchainExtent.width)
		.setHeight((float)_swapchainExtent.height)
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


	vk::PipelineLayoutCreateInfo layoutInfo{};
	vk::PipelineLayout layout = _device.createPipelineLayout(layoutInfo);
	



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
		.setLayout(layout)
		.setBasePipelineHandle(vk::Pipeline{})
		.setBasePipelineIndex(-1);
			
	_graphicsPipeline = _device.createGraphicsPipeline({}, createPipelineInfo).value;




}
	
void initComputePipeline() {

}
	


void cleanup() {



	_device.waitIdle();
	
	

	for (int i{}; i < NUM_OF_IMAGES; i++) {
		_device.destroyFence(_fences[i]);
		_device.destroySemaphore(_renderFinishedSemaphores[i]);
		_device.destroySemaphore(_imageReadySemaphores[i]);

	}

	_device.freeCommandBuffers(_cmdPool, _cmdBuffers);
	
	_device.destroyCommandPool(_cmdPool);
	for (int i{}; i < NUM_OF_IMAGES; i++) {
		
		_device.destroyImageView(_swapchainImageViews[i]);
	}
	
	_device.destroySwapchainKHR(_swapchain); // as vkimages are owned by swapchain they are killed with it
	_instance.destroySurfaceKHR(_surface);
	
	_device.destroy();
	glfwDestroyWindow(window);
	glfwTerminate();

}

void transitionImage(vk::Image image,vk::CommandBuffer cmdBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {

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


void draw() {
	static float counter{};
	counter += 0.01;
	

	//SYNC!!!!!!!!{
	currentFrame= (currentFrame + 1) % NUM_OF_IMAGES;
	vk::Fence curFence[] = { _fences[currentFrame] };

	_device.waitForFences(1,curFence, vk::True, 1000000000);
	_device.resetFences(curFence);

	
	

	vk::Semaphore imageReadySemaph = _imageReadySemaphores[currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = _cmdBuffers[currentFrame];//this is indx currentFrame cuz the fence above 
															//guarantees buffer is done submitting, therefore choose buffer based on fences index"currentFrame"


	imageIndex = _device.acquireNextImageKHR(_swapchain, 1000000000, imageReadySemaph).value;



	//now were using imageindex as index, swapchainimages using it is obvious as that is the currently free from rendering
	//we use renderfinished semaphore to signal finished rendering the image, therefore it should be tighlty coupled to same index as swapchainimages
	// "use currently free imagiendex to the image and its semaphore, otherwise the problem might be you reuse semaphore that is already being used in waitsemaphore field in presentsrckhr eg race condition
	vk::Semaphore renderFinishedSemaph = _renderFinishedSemaphores[imageIndex];
	vk::Image curImage = _swapchainImages[imageIndex];
	
	//SYNC!!!!!!!!}


	cmdBuffer.reset();

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	cmdBuffer.begin(beginInfo);
	
	
	transitionImage(curImage,cmdBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	
	
	vk::ClearColorValue clr{};
	
	clr.setFloat32({ 0.0f, 0.0f, sinf(counter), 1.0f});

	vk::ImageSubresourceRange subRange{};
	subRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLayerCount(vk::RemainingArrayLayers)
			.setBaseMipLevel(0)
			.setLevelCount(vk::RemainingMipLevels)
			.setBaseArrayLayer(0);



	//cmdBuffer.clearColorImage(curImage, vk::ImageLayout::eGeneral, clr,subRange);

	transitionImage(curImage, cmdBuffer, vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal);
	//begin dynrendering
	vk::RenderingAttachmentInfo attachInfo{};
	attachInfo
		.setImageView(_swapchainImageViews[imageIndex])
		.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setClearValue(clr)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore);
		


	//attachInfo.set
	vk::RenderingInfoKHR renderInfo{};
	vk::Rect2D area;
	area.setOffset(vk::Offset2D(0,0)).setExtent(_swapchainExtent);
	renderInfo
		.setRenderArea(area)
		.setLayerCount(1)
		.setColorAttachments(attachInfo);

	cmdBuffer.beginRendering(renderInfo);
	

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
	//setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
	//setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
	cmdBuffer.setViewport(0,vk::Viewport(0.0f,0.0f,float(_swapchainExtent.width),float(_swapchainExtent.height),0.0f,1.0f));
	cmdBuffer.setScissor(0,vk::Rect2D(vk::Offset2D(0,0),_swapchainExtent));
	cmdBuffer.draw(3, 1, 0, 0);
	cmdBuffer.endRendering();

	transitionImage(curImage, cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);


	cmdBuffer.end();
	 
	vk::SubmitInfo2 subInfo{};
	vk::CommandBufferSubmitInfo cmdInfo{};
	vk::SemaphoreSubmitInfo waitInfo{};
	vk::SemaphoreSubmitInfo signalInfo{};

	cmdInfo.setCommandBuffer(cmdBuffer).setDeviceMask(0);

	waitInfo.setSemaphore(imageReadySemaph)
			.setValue(1)
			.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
			.setDeviceIndex(0);

	signalInfo.setSemaphore(renderFinishedSemaph)
    		  .setValue(1)
    		  .setStageMask(vk::PipelineStageFlagBits2::eAllGraphics)
    		  .setDeviceIndex(0);

	
	
	subInfo.setWaitSemaphoreInfos(waitInfo)
		   .setSignalSemaphoreInfos(signalInfo)
		   .setCommandBufferInfos(cmdInfo);
	
	
	
	_graphicsQueue.handle.submit2(subInfo,_fences[currentFrame]);

	vk::PresentInfoKHR presInfo{};
	presInfo.setWaitSemaphores({ renderFinishedSemaph})
			.setSwapchains(_swapchain)
			.setImageIndices({ imageIndex });
	
	_graphicsQueue.handle.presentKHR(presInfo);
	
}
int main() {

	//pipeline,semaphs,fences,cmdbuffer,commandpool,

	initWindow();
	initInstance();
	initDeviceAndGPU();
	initSwapchain();
	initCommandBuffers();
	initSyncs();
	initGraphicsPipeline();
	//initComputePipeline

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		draw();
	}
	cleanup();
}