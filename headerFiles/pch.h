#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "vk_mem_alloc.h"
#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // <- rotate, translate, scale, lookAt, perspective
#include <stb_image.h>
#include<tiny_obj_loader.h>
#include <pipelineBuilder.h>
#include <tinygltf.h>
#include <json.hpp>
#include<stb_image_write.h>