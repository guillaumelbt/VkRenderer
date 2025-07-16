#pragma once
#include <memory>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "model/Model.h"

struct ModelComponent 
{
    std::shared_ptr<Model> model;
    VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
    glm::vec3 color{1.0f, 1.0f, 1.0f}; // Default white color
}; 