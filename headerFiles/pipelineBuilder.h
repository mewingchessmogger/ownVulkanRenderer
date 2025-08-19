#pragma once
#include <vulkan/vulkan.hpp>
#include <vkutils.h>
#include <vector>
class PipelineBuilder {
	
public:

	
	PipelineBuilder& setDevice(vk::Device dev) {
		device = dev;
		return *this;
	}
	PipelineBuilder& setDynRendering(int clrCount, std::vector<vk::Format> clrFs, vk::Format dphFormat);
	PipelineBuilder& setShaderStages(const std::string vertPath, const std::string fragPath);
	PipelineBuilder& setVertexInputInfo(std::vector<vk::VertexInputBindingDescription>& bindings,
		std::vector<vk::VertexInputAttributeDescription>& attribs);
	PipelineBuilder& setAssemblyInfo(vk::PrimitiveTopology top = vk::PrimitiveTopology::eTriangleList);
	PipelineBuilder& setScissorAndViewport(vk::Extent2D ext);

	PipelineBuilder& setRasterizerInfo(vk::PolygonMode poly = vk::PolygonMode::eFill, vk::FrontFace frontFace = vk::FrontFace::eClockwise);
	PipelineBuilder& setMultiSampling();
	PipelineBuilder& setBlendState();
	PipelineBuilder& setDynState();
	PipelineBuilder& setPCRange(size_t sizePC, int offset = 0, vk::ShaderStageFlags stage = vk::ShaderStageFlagBits::eVertex);
	PipelineBuilder& setDescLayout(std::vector<vk::DescriptorSetLayout> descLayouts);
	PipelineBuilder& createPipeLineLayout();
	PipelineBuilder& setDepthStencilState();
	PipelineBuilder& createPipeline();
	vk::Pipeline getPipeline();
	vk::PipelineLayout pipeLineLayout;
	vk::Viewport viewport{};
	
private:
	vk::Device device;
	std::vector<vk::Format> clrFormats{};
	vk::Format depthFormat{};

	std::vector<vk::VertexInputBindingDescription> bindings = {};
	std::vector<vk::VertexInputAttributeDescription> attribs = {};

	bool pipelineCreated{ false };
	
	vk::PipelineRenderingCreateInfo dynRenderInfo{};
	
	
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{};
	vk::ShaderModule vertShader{};
	vk::ShaderModule fragShader{};


	//setvertexinput
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	
	//setassembly
	vk::PipelineInputAssemblyStateCreateInfo assInfo{};


	//setscissor
	vk::PipelineViewportStateCreateInfo viewportInfo{};
	vk::Rect2D scissor{};

	vk::PipelineRasterizationStateCreateInfo razInfo{};

	vk::PipelineMultisampleStateCreateInfo multisampleInfo{};

	vk::PipelineDepthStencilStateCreateInfo stencilInfo{};//nullptr

	vk::PipelineColorBlendStateCreateInfo blendInfo{};
	vk::PipelineColorBlendAttachmentState attachment{};

	vk::PipelineDynamicStateCreateInfo dynamicState{};
	std::vector<vk::DynamicState> dynamicStates = {
	vk::DynamicState::eViewport,
	vk::DynamicState::eScissor
	};

	vk::PipelineLayoutCreateInfo layoutInfo{};
	vk::PushConstantRange  pcRange{};
	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{};

	vk::PipelineDepthStencilStateCreateInfo depthInfo{};

	vk::GraphicsPipelineCreateInfo createPipelineInfo{};
	
	vk::Pipeline pipeline;
};