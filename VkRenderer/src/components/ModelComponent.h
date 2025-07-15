#pragma once
#include <memory>
#include <vulkan/vulkan.h>
#include "model/Model.h"

struct ModelComponent 
{
    std::shared_ptr<Model> model;
    VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;
}; 