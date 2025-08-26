
#include "pch.h"
#include <engine.h>
#include <pipelineBuilder.h>
#include <Vulkancontext.h>
#include <BufferContext.h>
#include <WindowContext.h>



void Engine::loadModelsGLTF(){
	
	std::vector<std::string> modPaths{"models/dragon.gltf","models/bunny.gltf"};// , "models/stanfordBunny" 
	for (auto& path : modPaths) {
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;

		std::string err;
		std::string warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

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
			uint32_t baseVertex = static_cast<uint32_t>(btx->vertices.size()- data.startVBO);


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
					btx->indices.push_back(baseVertex+indices16[i]);
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

	size_t id = btx->ComposerID.hasher(path);

	btx->ComposerID.modelMapper[id] = data;
	std::cout << "gltf!!!" << path << " ID: " << id << "--- VBO start/end: " << data.startVBO << ", " << data.endVBO << " ---IBO start/end: " << data.startIBO << ", " << data.endIBO << "\n";
	}

}
void Engine::loadModels() {


	std::vector<std::string> modPaths{ "models/cube.obj","models/viking_room.obj"};// , "models/stanfordBunny" 
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
		bool hasNormals = false;


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
					hasNormals = true;
				}


				if (btx->uniqueVertices.count(vertex) == 0) {//remember every iter of whole loop, first time if statement triggers .size() == data.startvbo
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
		std::string hasNormal = hasNormals ? " YES Normals!" : " NO Normals!";
		std::cout << "OBJ!!: " << path <<  hasNormal << " ID: " << id << "--- VBO start/end: " << data.startVBO << ", " << data.endVBO << " ---IBO start/end: " << data.startIBO << ", " << data.endIBO << "\n";

	}


}

void Engine::initGameObjects() {
	// Object 1 - Center

	std::hash<std::string> hasher;

	btx->gameObjs.resize(5);
	btx->lightObjs.resize(2);

	btx->gameObjs[0].pos = { -1.0f, 0.2f, 0.0f };
	btx->gameObjs[0].rot = { glm::radians(180.0f), 0.0f, 0.0f };
	btx->gameObjs[0].scale = { 5.0f,0.2f,5.0f };
	btx->gameObjs[0].color = { 0.0f,0.3f,0.0f };
	btx->gameObjs[0].modelID = hasher("models/cube.obj");

	btx->gameObjs[1].pos = { 0.0f, 0.0f, 0.0f };
	btx->gameObjs[1].rot = { glm::radians(180.0f), glm::radians(180.0f), 0.0f };
	btx->gameObjs[1].scale = { 1.0f,1.0f,1.0f };
	btx->gameObjs[1].color = { 0.4f,0.4f,0.4f };
	btx->gameObjs[1].modelID = hasher("models/bunny.gltf");

	// Object 2 - Left


	btx->gameObjs[2].pos = { -1.0f, 0.0f, 0.0f };
	btx->gameObjs[2].rot = { glm::radians(90.0f), 0.0f, 0.0f };
	btx->gameObjs[2].scale = { 0.5f,0.5f,0.5f };
	btx->gameObjs[2].usingTexture = 1;
	btx->gameObjs[2].modelID = hasher("models/viking_room.obj");


	btx->gameObjs[3].pos = { -1.0f, 0.2f, 1.0f };
	btx->gameObjs[3].rot = { glm::radians(180.0f), glm::radians(180.0f), 0.0f };
	btx->gameObjs[3].scale = { 4.0f,4.0f,4.0f };
	btx->gameObjs[3].color = { 0.4f,0.4f,0.4f };
	btx->gameObjs[3].modelID = hasher("models/dragon.gltf");


	btx->lightObjs[0].pos = { 5.0f, -4.0f,-10.0f };
	btx->lightObjs[0].rot = { 0.0f,0.0f, 0.0f };
	btx->lightObjs[0].scale = {1.0f,1.0f,1.0f };
	btx->lightObjs[0].color = { 1.0f,1.0f, 1.0f };
	btx->lightObjs[0].modelID = hasher("models/cube.obj");


	//btx->gameObjs[1].pos = { -1.0f, 0.0f, -1.0f };
	//btx->gameObjs[1].rot = { glm::radians(180.0f), glm::radians(180.0f), 0.0f };
	//btx->gameObjs[1].scale = { 3.0f, 3.0f,3.0f };
	for (auto& obj : btx->gameObjs) {
		std::cout << "ID: " << obj.modelID << "\n";
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
		.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	

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
		.setDescriptorCount(2)//how many of this type of descritpor pool fits
		.setType(vk::DescriptorType::eCombinedImageSampler);

	std::array<vk::DescriptorPoolSize,2> poolSizes = { uboSizes,samplerSizes };

	poolInfo
		.setMaxSets(3)//how many "DESCRIPTOR SETS" can be allcoated maximum
		.setPoolSizes(poolSizes);

	ctx->_descPool = ctx->_device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo setAlloc{};
	
	std::vector<vk::DescriptorSetLayout> layouts{ctx->_descSetLayoutUBO,ctx->_descSetLayoutSampler,ctx->_descSetLayoutSampler };
	
	setAlloc
		.setDescriptorPool(ctx->_descPool)
		.setDescriptorSetCount(3)//how many "DESCRIPTOR SETS" we choose to allocate
		.setSetLayouts(layouts);


	ctx->_descSets = ctx->_device.allocateDescriptorSets(setAlloc);
	std::cout << ctx->_descSets.size() << " amount of desc sets\n";
	

	vk::DescriptorBufferInfo info1{};
	info1
		.setBuffer(btx->_GameObjUBO.buffer)
		.setOffset(0)
		.setRange(sizeof(btx->dataUBO));
		


	vk::WriteDescriptorSet writes[3];

	
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

	vk::DescriptorImageInfo cubeInfo{};
	cubeInfo
		.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setImageView(btx->_cubeImages[0].view)
		.setSampler(ctx->_sampler);

	writes[2]
		.setImageInfo(cubeInfo)
		.setDstBinding(0)
		.setDstSet(ctx->_descSets[2])
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)

		.setDstArrayElement(0)
		.setDescriptorCount(1);


	ctx->_device.updateDescriptorSets(writes,{});


}

void Engine::initGraphicsPipeline() {


	auto bindings = btx->getVertBindings();
	auto attributes = btx->getVertAttributes();
	
	std::vector < vk::VertexInputBindingDescription> emptybindings{};

	std::vector < vk::VertexInputAttributeDescription> emptyattribs{};

	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { ctx->_descSetLayoutUBO,ctx->_descSetLayoutSampler };
	std::vector<vk::DescriptorSetLayout> skyboxSetLayout = { ctx->_descSetLayoutSampler };



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

	
	plb.resetBuild();

	plb.setDevice(ctx->_device)
		.setDynRendering(1, { ctx->_swapchainFormat }, btx->_depthImages[0].format)
		.setShaderStages("shaders/vertex/skybox_V.spv", "shaders/frag/skybox_F.spv")
		.setVertexInputInfo(emptybindings,emptyattribs)
		.setAssemblyInfo(vk::PrimitiveTopology::eTriangleList)
		.setScissorAndViewport(ctx->_swapchainExtent)
		.setRasterizerInfo()
		.setMultiSampling()//nuffin right now
		.setBlendState()
		.setDynState()
		.setPCRange(sizeof(BufferContext::skyboxPC), 0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
		.setDescLayout(skyboxSetLayout)
		.createPipeLineLayout()
		.setDepthStencilState(1U,0U)
		.createPipeline();

	ctx->_skyboxPipeline = plb.getPipeline();
	ctx->_skyboxLayout = plb.pipeLineLayout;

}

bool Engine::acquiredFrameImage() {

	vk::Fence curFence[] = { ctx->_fences[ctx->currentFrame] };

	ctx->_device.waitForFences(1, curFence, vk::True, 1000000000);


	vk::Semaphore imageReadySemaph = ctx->_imageReadySemaphores[ctx->currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = ctx->_cmdBuffers[ctx->currentFrame];//this is indx currentFrame cuz the fence above 
	//guarantees buffer is done submitting, therefore choose buffer based on fences index"currentFrame"

	auto imgResult = ctx->_device.acquireNextImageKHR(ctx->_swapchain, 1000000000, imageReadySemaph);
	if (isValidSwapchain(imgResult, imageReadySemaph) == false) {
		//swapchain recreated in func above
		rethinkDepthImages();
		return false;
	}
	//auto imageIndex = imgResult.value;
	
	ctx->currentImgIndex = imgResult.value;
	ctx->_device.resetFences(curFence);
	return true;
}

void Engine::startRecFrame() {
	//vk::Semaphore imageReadySemaph = ctx->_imageReadySemaphores[ctx->currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = ctx->_cmdBuffers[ctx->currentFrame];//this is indx currentFrame cuz the fence above 


	cmdBuffer.reset();

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	cmdBuffer.begin(beginInfo);


}
void Engine::preFX() {
	static float counter{};
	counter += 0.01f;

	vk::Semaphore imageReadySemaph = ctx->_imageReadySemaphores[ctx->currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = ctx->_cmdBuffers[ctx->currentFrame];//this is indx currentFrame cuz the fence above 
	auto imageIndex = ctx->currentImgIndex;


	vkutils::transitionImage(ctx->_swapchainImages[ctx->currentImgIndex], cmdBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

	vk::ClearColorValue clr{};	//sinf(counter)
	clr.setFloat32({ 0.0f, 0.0f, 0.3f, 1.0f });

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
		.setClearValue(vk::ClearDepthStencilValue{ 1,0 })
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

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, ctx->_skyboxPipeline);
	cmdBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, float(ctx->_swapchainExtent.width), float(ctx->_swapchainExtent.height), 0.0f, 1.0f));
	cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), ctx->_swapchainExtent));
	cmdBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		ctx->_skyboxLayout,  // pipeline layout for skybox
		0,                   // firstSet = 0
		ctx->_descSets[2],   // bind just the cubemap sampler set
		{}
	);
	//camera
	glm::mat4 view(1.0f);

	// camera position
	glm::vec3 eye = camera.eye;
	glm::mat4 model(1.0f);
	glm::mat4 proj(1.0f);
	static glm::vec3 direction{};  // where the camera looks

	direction = camera.dir;
	view = glm::lookAt(eye, eye + direction, camera.up);

	proj = glm::perspective(glm::radians(45.0f), (float)ctx->WIDTH / (float)ctx->HEIGHT, 0.1f, 100.0f);


	view[3] = glm::vec4(0, 0, 0, view[3].w);

	BufferContext::skyboxPC skyboxPushConstant{};
	skyboxPushConstant.invView = glm::inverse(view);
	skyboxPushConstant.invProj = glm::inverse(proj);
	


	cmdBuffer.pushConstants(ctx->_skyboxLayout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0, sizeof(skyboxPushConstant), &skyboxPushConstant);

	cmdBuffer.draw(3, 1, 0, 0); // <-- 3 vertices, no vertex buffer
	 
}

void Engine::recordDrawCalls() {
	//static float counter{};
	//counter += 0.01f;

	vk::Semaphore imageReadySemaph = ctx->_imageReadySemaphores[ctx->currentFrame];//read below same shtick
	vk::CommandBuffer cmdBuffer = ctx->_cmdBuffers[ctx->currentFrame];//this is indx currentFrame cuz the fence above 
	auto imageIndex = ctx->currentImgIndex;


	//vkutils::transitionImage(ctx->_swapchainImages[ctx->currentImgIndex], cmdBuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

	//vk::ClearColorValue clr{};	//sinf(counter)
	//clr.setFloat32({ 0.0f, 0.0f, 0.3f, 1.0f });

	////setup for dynrenderin
	//vk::RenderingAttachmentInfo attachInfo{};
	//attachInfo
	//	.setImageView(ctx->_swapchainImageViews[imageIndex])
	//	.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
	//	.setClearValue(clr)
	//	.setLoadOp(vk::AttachmentLoadOp::eClear)
	//	.setStoreOp(vk::AttachmentStoreOp::eStore);


	//vk::RenderingAttachmentInfo depthInfo{};
	//depthInfo
	//	.setImageView(btx->_depthImages[imageIndex].view)
	//	.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
	//	.setClearValue(vk::ClearDepthStencilValue{ 1,0 })
	//	.setLoadOp(vk::AttachmentLoadOp::eClear)
	//	.setStoreOp(vk::AttachmentStoreOp::eStore);

	//vk::RenderingInfoKHR renderInfo{};

	//vk::Rect2D area;
	//area.setOffset(vk::Offset2D(0, 0)).setExtent(ctx->_swapchainExtent);
	//renderInfo
	//	.setRenderArea(area)
	//	.setLayerCount(1)
	//	.setColorAttachments(attachInfo)
	//	.setPDepthAttachment(&depthInfo);

	//cmdBuffer.beginRendering(renderInfo);



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
	view = glm::lookAt(eye, eye + direction, camera.up);

	btx->dataUBO.view = view;
	//drawing


	proj = glm::perspective(glm::radians(45.0f), (float)ctx->WIDTH / (float)ctx->HEIGHT, 0.1f, 100.0f);
	//model  = glm::translate(model, glm::vec3(0.0,1.0f,-0.5f));
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));   // rotate around Z
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1));   // rotate around Z
	btx->dataUBO.proj = proj;




	uint8_t* base = static_cast<uint8_t*>(btx->_GameObjUBO.allocInfo.pMappedData);

	cmdBuffer.bindVertexBuffers(0, btx->_vertexBuffer.buffer, offset);
	cmdBuffer.bindIndexBuffer(btx->_indexBuffer.buffer, offset, vk::IndexType::eUint32);


	TransformUBO ubo{};
	ubo.proj = proj;
	ubo.view = view;


	for (int i{}; i < btx->lightObjs.size(); i++) {

		Lights light{};

		light.position = glm::vec4(btx->lightObjs[i].pos, 1.0f);
		light.position.x = light.position.x;
		light.color = glm::vec4(1.0f);
		ubo.lights[i] = light;

	}


	size_t offsetUBO = (ctx->currentFrame * btx->strideUBO);
	std::memcpy(base + offsetUBO, &ubo, sizeof(ubo));

	
	uint32_t dynOffset = static_cast<uint32_t>(offsetUBO);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		ctx->_layout, 0, 2, ctx->_descSets.data(), 1, &dynOffset);
	


	for (int i{}; i < btx->gameObjs.size(); i++) {
		auto& obj = btx->gameObjs[i];

		BufferContext::indexDataModels modData = btx->ComposerID.modelMapper[obj.modelID];

		BufferContext::pushConstants pc{};




		pc.model = obj.getModelMatrix();
		pc.color = obj.color;
		pc.useTexture = obj.usingTexture;

		cmdBuffer.pushConstants(ctx->_layout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			0, sizeof(BufferContext::pushConstants), &pc);

		auto indexCount = static_cast<uint32_t>(modData.endIBO - modData.startIBO);
		cmdBuffer.drawIndexed(
			indexCount, //how many indices
			1, //instances
			modData.startIBO,//where to start in ibo 
			modData.startVBO, //where to start in vbo
			0);//instance shit 

	}





	cmdBuffer.endRendering();

}
void Engine::postFX() {
	//nothing right now

}

void Engine::concludeFrame() {
	auto imageIndex = ctx->currentImgIndex;

	vk::Semaphore imageReadySemaph = ctx->_imageReadySemaphores[ctx->currentFrame];//read below same shtick
	vk::Semaphore renderFinishedSemaph = ctx->_renderFinishedSemaphores[imageIndex];
	vk::CommandBuffer cmdBuffer = ctx->_cmdBuffers[ctx->currentFrame];//this is indx currentFrame cuz the fence above 


	vkutils::transitionImage(ctx->_swapchainImages[imageIndex], cmdBuffer, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);
	cmdBuffer.end();

	vkutils::submitHelper(cmdBuffer, imageReadySemaph, renderFinishedSemaph, vk::PipelineStageFlagBits2::eColorAttachmentOutput
		, vk::PipelineStageFlagBits2::eAllGraphics, ctx->_graphicsQueue.handle, ctx->_fences[ctx->currentFrame]);

	vk::PresentInfoKHR presInfo{};
	presInfo.setWaitSemaphores({ renderFinishedSemaph })
		.setSwapchains(ctx->_swapchain)
		.setImageIndices({ imageIndex });
	ctx->_graphicsQueue.handle.presentKHR(presInfo);

	ctx->currentFrame = (ctx->currentFrame + 1) % ctx->NUM_OF_IMAGES;
}

void Engine::run() {
	startup();

		while (!glfwWindowShouldClose(wtx->window)) {
			glfwPollEvents();
			HandlerIO(camera);
			
			//gamelogic

			if (acquiredFrameImage() == true) {
				startRecFrame();
				preFX();
				recordDrawCalls();
				postFX();
				concludeFrame();
			}


		}
		cleanup(); 

}