#pragma once
#include "core/Device.h"
#include "core/FrameInfo.h"
#include "model/GameObject.h"
#include "core/Pipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "camera/Camera.h"

class RenderSystem
{
public:
    RenderSystem(Device& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout, VkDescriptorSetLayout _textureSetLayout, VkSampleCountFlagBits _msaaSamples);
    ~RenderSystem();

    RenderSystem(const RenderSystem&) = delete;
    RenderSystem& operator=(const RenderSystem&) = delete;

    void RenderGameObjects(FrameInfo& _frameInfo);

private:
    void CreatePipelineLayout(VkDescriptorSetLayout _globalSetLayout);
    void CreatePipeline(VkRenderPass renderPass);
    void CreatePipelineLayoutTextured(VkDescriptorSetLayout _globalSetLayout, VkDescriptorSetLayout _textureSetLayout);
    void CreatePipelineTextured(VkRenderPass _renderPass);

    Device& m_device;

    std::unique_ptr<Pipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout;

    std::unique_ptr<Pipeline> m_pipelineTextured;
    VkPipelineLayout m_pipelineLayoutTextured;
    VkSampleCountFlagBits m_msaaSamples;
};

