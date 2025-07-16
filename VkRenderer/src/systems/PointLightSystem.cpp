#include "systems/PointLightSystem.h"
#include "components/PointLightComponent.h"
#include "components/TransformComponent.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <array>
#include <cassert>
#include <stdexcept>
#include <glm/ext/matrix_transform.hpp>
#include <map>

struct PointLightPushConstants 
{
    glm::vec4 position{};
    glm::vec4 color{};
    float radius;
};


PointLightSystem::PointLightSystem(Device& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout, VkSampleCountFlagBits msaaSamples) 
    : m_device{ _device }, m_msaaSamples{ msaaSamples }
{
    CreatePipelineLayout(_globalSetLayout);
    CreatePipeline(_renderPass);
}

PointLightSystem::~PointLightSystem() 
{
    vkDestroyPipelineLayout(m_device.GetDevice(), m_pipelineLayout, nullptr);
}

void PointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout _globalSetLayout) 
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PointLightPushConstants);

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

void PointLightSystem::CreatePipeline(VkRenderPass _renderPass) 
{
    assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
    Pipeline::EnableAlphaBlending(pipelineConfig);
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.renderPass = _renderPass;
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.multisampleInfo.rasterizationSamples = m_msaaSamples;

    m_pipeline = std::make_unique<Pipeline>(m_device, "shaders/pointLight_vert.spv", "shaders/pointLight_frag.spv",pipelineConfig);


}

void PointLightSystem::Render(FrameInfo& _frameInfo) 
{
    m_pipeline->Bind(_frameInfo.commandBuffer);
    vkCmdBindDescriptorSets(_frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,  &_frameInfo.globalDescriptorSet, 0,nullptr);

    if (_frameInfo.ec) {
        _frameInfo.ec->ForEach<PointLightComponent>([&](Entity id, PointLightComponent& light) {
            if (!_frameInfo.ec->HasComponent<TransformComponent>(id)) return;
            auto& transform = _frameInfo.ec->GetComponent<TransformComponent>(id);
            PointLightPushConstants push{};
            push.position = glm::vec4(transform.translation, 1.f);
            push.color = glm::vec4(light.color, light.lightIntensity);
            push.radius = 0.1f; 
            vkCmdPushConstants(_frameInfo.commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstants), &push);
            vkCmdDraw(_frameInfo.commandBuffer, 6, 1, 0, 0);
        });
    }
}

void PointLightSystem::Update(FrameInfo& _frameInfo, GlobalUbo& _ubo) 
{
    auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * _frameInfo.frameTime, { 0.f, -1.f, 0.f });
    int lightIndex = 0;
    // for (auto& pair : _frameInfo.gameObjects)
    // {
    //     auto& obj = pair.second;
    //     if (obj.pointLight == nullptr) continue;
    //     assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");
    //     obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));
    //     _ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
    //     _ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
    //     lightIndex += 1;
    // }
    // _ubo.numLights = lightIndex;

    if (_frameInfo.ec) {
        _frameInfo.ec->ForEach<PointLightComponent>([&](Entity id, PointLightComponent& light) {
            if (!_frameInfo.ec->HasComponent<TransformComponent>(id)) return;
            auto& transform = _frameInfo.ec->GetComponent<TransformComponent>(id);
            assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");
            transform.translation = glm::vec3(rotateLight * glm::vec4(transform.translation, 1.f));
            _ubo.pointLights[lightIndex].position = glm::vec4(transform.translation, 1.f);
            _ubo.pointLights[lightIndex].color = glm::vec4(light.color, light.lightIntensity);
            lightIndex += 1;
        });
    }
    _ubo.numLights = lightIndex;
}