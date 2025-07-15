#include "systems/RenderSystem.h"
#include "core/Pipeline.h"
#include "components/ModelComponent.h"
#include "components/TransformComponent.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <cassert>
#include <stdexcept>


struct SimplePushConstantData 
{
    glm::mat4 modelMatrix{ 1.0f };
    glm::mat4 normalMatrix{ 1.0f };
};

RenderSystem::RenderSystem(Device& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout, VkDescriptorSetLayout _textureSetLayout, VkSampleCountFlagBits msaaSamples)
    : m_device{ _device }, m_msaaSamples{ msaaSamples }
{
    CreatePipelineLayout(_globalSetLayout);
    CreatePipeline(_renderPass);
    CreatePipelineLayoutTextured(_globalSetLayout, _textureSetLayout);
    CreatePipelineTextured(_renderPass);
}

RenderSystem::~RenderSystem() 
{
    vkDestroyPipelineLayout(m_device.GetDevice(), m_pipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_device.GetDevice(), m_pipelineLayoutTextured, nullptr);
}

void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout _globalSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ _globalSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) 
        throw std::runtime_error("failed to create pipeline layout");
}

void RenderSystem::CreatePipelineLayoutTextured(VkDescriptorSetLayout _globalSetLayout, VkDescriptorSetLayout _textureSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ _globalSetLayout, _textureSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayoutTextured) != VK_SUCCESS) 
        throw std::runtime_error("failed to create pipeline layout (textured)");
}

void RenderSystem::CreatePipeline(VkRenderPass _renderPass) 
{
    assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{}; 
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = _renderPass;
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.multisampleInfo.rasterizationSamples = m_msaaSamples;
    m_pipeline = std::make_unique<Pipeline>(m_device, "shaders/shader_vert.spv", "shaders/shader_frag.spv", pipelineConfig);
}

void RenderSystem::CreatePipelineTextured(VkRenderPass _renderPass)
{
    assert(m_pipelineLayoutTextured != nullptr && "Cannot create pipeline before pipeline layout (textured)");

    PipelineConfigInfo pipelineConfig{}; 
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = _renderPass;
    pipelineConfig.pipelineLayout = m_pipelineLayoutTextured;
    pipelineConfig.multisampleInfo.rasterizationSamples = m_msaaSamples;
    m_pipelineTextured = std::make_unique<Pipeline>(m_device, "shaders/texture_vert.spv", "shaders/texture_frag.spv", pipelineConfig);
}

void RenderSystem::RenderGameObjects(FrameInfo& _frameInfo)
{
    if (_frameInfo.ec) 
    {
        _frameInfo.ec->ForEach<ModelComponent>([&](Entity id, ModelComponent& modelComp) 
            {
            if (!_frameInfo.ec->HasComponent<TransformComponent>(id)) return;

            auto& transform = _frameInfo.ec->GetComponent<TransformComponent>(id);

            if (!modelComp.model) 

            {
                return;
            }

            SimplePushConstantData push{};
            push.modelMatrix = transform.Mat4();
            push.normalMatrix = transform.NormalMatrix();
            if (modelComp.textureDescriptorSet != VK_NULL_HANDLE) 
            {
                m_pipelineTextured->Bind(_frameInfo.commandBuffer);
                vkCmdBindDescriptorSets(_frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutTextured, 0, 1, &_frameInfo.globalDescriptorSet, 0, nullptr);
                vkCmdBindDescriptorSets(_frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayoutTextured, 1, 1, &modelComp.textureDescriptorSet, 0, nullptr);
            } else 
            {
                m_pipeline->Bind(_frameInfo.commandBuffer);
                vkCmdBindDescriptorSets(_frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &_frameInfo.globalDescriptorSet, 0, nullptr);
            }
            vkCmdPushConstants(_frameInfo.commandBuffer, modelComp.textureDescriptorSet != VK_NULL_HANDLE ? m_pipelineLayoutTextured : m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
            modelComp.model->Bind(_frameInfo.commandBuffer);
            modelComp.model->Draw(_frameInfo.commandBuffer);
        });
    }
}

