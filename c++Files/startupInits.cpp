#include <pch.h>
#include <engine.h>


void Engine::startup(){
	initWindow();
	initInstance();
	initDevice();
	createSwapchain();
	initCommands();
	initSyncs();
	createDepthImages();
	createRenderTargetImages();
	createTextureImage();
	createCubeImage();
	createTextureSampler();
	loadModelsGLTF();
	loadModels();
	initGameObjects();
	initVertexBuffer();
	initIndexBuffer();
	initUniformBuffer();
	initDescriptors();
	initGraphicsPipeline();

}