#pragma once
#include "core/Device.h"
#include "core/Pipeline.h"
#include "core/Descriptors.h"
#include "core/FrameInfo.h"
#include "core/Particle.h"
#include "components/ParticleSystemComponent.h"
#include "components/TransformComponent.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class ParticleRenderSystem {
public:
    ParticleRenderSystem(Device& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout, VkSampleCountFlagBits _msaaSamples, uint32_t _maxParticles, float _particleSize, glm::vec3 _particleColor);
    ~ParticleRenderSystem();

    ParticleRenderSystem(const ParticleRenderSystem&) = delete;
    ParticleRenderSystem& operator=(const ParticleRenderSystem&) = delete;

    void Render(FrameInfo& _frameInfo, VkDescriptorSet _descriptorSet);
    void UpdateParticlesWithCompute(float _deltaTime, VkCommandBuffer _commandBuffer, const ParticleSystemComponent& _params, const TransformComponent& _transform);

    Buffer* GetParticleBuffer() { return m_particleBuffer.get(); }

private:

    void CreatePipelineLayout(VkDescriptorSetLayout _globalSetLayout);
    void CreatePipeline(VkRenderPass _renderPass, VkSampleCountFlagBits _msaaSamples);
    void CreateVertexBuffer();
    void CreateParticleBuffer();
    void CreateComputePipeline();
    void CreateComputeDescriptorSet();

    Device& m_device;
    
    std::unique_ptr<Pipeline> m_pipeline;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    
    VkPipeline m_computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
    std::unique_ptr<DescriptorSetLayout> m_computeDescriptorSetLayout;
    VkDescriptorSet m_computeDescriptorSet = VK_NULL_HANDLE;
    std::unique_ptr<DescriptorPool> m_computeDescriptorPool;
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    std::unique_ptr<Buffer> m_particleBuffer;
    uint32_t m_maxParticles = 0;
    
    VkSampleCountFlagBits m_msaaSamples;
    float m_particleSize = 0.01f;
    glm::vec3 m_particleColor = glm::vec3(1.0f, 0.3f, 0.0f);
}; 