#include "systems/ParticleRenderSystem.h"
#include "core/Utils.h"
#include "core/SwapChain.h"
#include "core/FrameInfo.h"
#include "systems/EntityComponentSystem.h"
#include "components/ParticleSystemComponent.h"
#include "components/TransformComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <random>
#include <ctime>
#include <chrono>

struct RenderPushConstants 
{
    float particleSize;
    glm::vec3 particleColor;
};

ParticleRenderSystem::ParticleRenderSystem(Device& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout, VkSampleCountFlagBits _msaaSamples, uint32_t _maxParticles, float _particleSize, glm::vec3 _particleColor)
    : m_device{ _device }, m_msaaSamples{ _msaaSamples }, m_maxParticles{ _maxParticles }, m_particleSize{ _particleSize }, m_particleColor{ _particleColor }
{
    CreatePipelineLayout(_globalSetLayout);
    CreatePipeline(_renderPass, _msaaSamples);
    CreateVertexBuffer();
    CreateParticleBuffer();
    CreateComputePipeline();
    CreateComputeDescriptorSet();
}

ParticleRenderSystem::~ParticleRenderSystem()
{
    vkDestroyBuffer(m_device.GetDevice(), m_vertexBuffer, nullptr);
    vkFreeMemory(m_device.GetDevice(), m_vertexBufferMemory, nullptr);
    
    if (m_computePipeline != VK_NULL_HANDLE) 
    {
        vkDestroyPipeline(m_device.GetDevice(), m_computePipeline, nullptr);
    }
    if (m_computePipelineLayout != VK_NULL_HANDLE) 
    {
        vkDestroyPipelineLayout(m_device.GetDevice(), m_computePipelineLayout, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) 
    {
        vkDestroyPipelineLayout(m_device.GetDevice(), m_pipelineLayout, nullptr);
    }
}

void ParticleRenderSystem::CreateParticleBuffer()
{
    std::vector<Particle> particles(m_maxParticles);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> velDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> lifeDist(2.5f, 7.5f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);

    for (uint32_t i = 0; i < m_maxParticles; ++i) 
    {
        auto& p = particles[i];
        
        float angle = angleDist(gen);
        float radius = 1.0f;
        float height = posDist(gen) * 0.3f;
        
        glm::vec3 pos = glm::vec3(
            cos(angle) * radius,
            height - 0.5f,
            sin(angle) * radius + 1.0f
        );
        p.position = pos;
        p.startPos = pos;
        
        p.velocity = glm::vec3(velDist(gen) * 0.2f, 1.0f + velDist(gen) * 0.3f, velDist(gen) * 0.2f);
        p.convergenceTarget = glm::vec3(0.0f, 3.0f, 1.0f);
        
        float maxLife = lifeDist(gen);
        p.maxLife = maxLife;
        p.life = 0.0f;
        p.colorLookup = 0.0f;
        
        p.pad1 = 0.0f;
        p.pad2 = 0.0f;
        p.pad3 = 0.0f;
        p.pad4 = glm::vec2(0.0f);
    }
    m_particleBuffer = std::make_unique<Buffer>(m_device,sizeof(Particle),m_maxParticles,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    Buffer stagingBuffer(m_device, sizeof(Particle), m_maxParticles,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    
    stagingBuffer.Map();
    stagingBuffer.WriteToBuffer(particles.data());
    stagingBuffer.Flush(VK_WHOLE_SIZE);
    m_device.CopyBuffer(stagingBuffer.GetBuffer(), m_particleBuffer->GetBuffer(), m_particleBuffer->GetBufferSize());
    stagingBuffer.Unmap();
}

void ParticleRenderSystem::Render(FrameInfo& _frameInfo, VkDescriptorSet _descriptorSet)
{
    if (!m_pipeline) return;
    
    m_pipeline->Bind(_frameInfo.commandBuffer);
    
    vkCmdBindDescriptorSets(_frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
    
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_frameInfo.commandBuffer, 0, 1, &m_vertexBuffer, &offset);
    
    vkCmdDraw(_frameInfo.commandBuffer, 6, m_maxParticles, 0, 0);
}

void ParticleRenderSystem::CreatePipelineLayout(VkDescriptorSetLayout _globalSetLayout)
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ _globalSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) 
        throw std::runtime_error("failed to create particle pipeline layout");
    
}

void ParticleRenderSystem::CreatePipeline(VkRenderPass _renderPass, VkSampleCountFlagBits _msaaSamples)
{
    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
    Pipeline::EnableFireParticleBlending(pipelineConfig);
    
    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    
    pipelineConfig.renderPass = _renderPass;
    pipelineConfig.pipelineLayout = m_pipelineLayout;
    pipelineConfig.multisampleInfo.rasterizationSamples = _msaaSamples;

    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.attributeDescriptions.clear();

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float) * 2;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    pipelineConfig.bindingDescriptions.push_back(bindingDescription);

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = 0;
    attributeDescription.location = 0;
    attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription.offset = 0;
    pipelineConfig.attributeDescriptions.push_back(attributeDescription);

    m_pipeline = std::make_unique<Pipeline>(m_device, "shaders/particle_vert.spv", "shaders/particle_frag.spv", pipelineConfig);
}

void ParticleRenderSystem::CreateVertexBuffer()
{
    std::vector<float> vertices = 
    {
        -1.0f, -1.0f,  
         1.0f, -1.0f,
        -1.0f,  1.0f, 
        -1.0f,  1.0f, 
         1.0f, -1.0f, 
         1.0f,  1.0f  
    };

    VkDeviceSize bufferSize = sizeof(float) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_device.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device.GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device.GetDevice(), stagingBufferMemory);

    m_device.CreateBuffer(bufferSize,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,m_vertexBuffer,m_vertexBufferMemory);

    m_device.CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_device.GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_device.GetDevice(), stagingBufferMemory, nullptr);
} 

void ParticleRenderSystem::CreateComputePipeline()
{
    m_computeDescriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        .Build();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ComputePushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    std::vector<VkDescriptorSetLayout> setLayouts{ m_computeDescriptorSetLayout->GetDescriptorSetLayout() };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS) 
   
        throw std::runtime_error("failed to create compute pipeline layout");
    

    auto computeShaderCode = Utils::ReadFile("shaders/particle_comp.spv");
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = computeShaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(computeShaderCode.data());

    VkShaderModule computeShaderModule;
    if (vkCreateShaderModule(m_device.GetDevice(), &createInfo, nullptr, &computeShaderModule) != VK_SUCCESS) 
   
        throw std::runtime_error("failed to create compute shader module");
    

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = computeShaderModule;
    shaderStageInfo.pName = "main";


    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = m_computePipelineLayout;

    if (vkCreateComputePipelines(m_device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS) 
        throw std::runtime_error("failed to create compute pipeline");
    


    vkDestroyShaderModule(m_device.GetDevice(), computeShaderModule, nullptr);
}

void ParticleRenderSystem::CreateComputeDescriptorSet()
{
    m_computeDescriptorPool = DescriptorPool::Builder(m_device)
        .SetMaxSets(1)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1)
        .Build();

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_particleBuffer->GetBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = m_particleBuffer->GetBufferSize();

    DescriptorWriter(*m_computeDescriptorSetLayout, *m_computeDescriptorPool)
        .WriteBuffer(0, &bufferInfo)
        .Build(m_computeDescriptorSet);
}

void ParticleRenderSystem::UpdateParticlesWithCompute(float _deltaTime, VkCommandBuffer _commandBuffer, const ParticleSystemComponent& _params, const TransformComponent& _transform)
{
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSet, 0, nullptr);
    
    ComputePushConstants pushConstants = {};
    pushConstants.deltaTime = _deltaTime;
    pushConstants.curlE = 0.1f;
    pushConstants.curlMultiplier = 0.8f; 
    pushConstants.particleMinLife = _params.particleLifetime * 0.5f; 
    pushConstants.particleMaxLife = _params.particleLifetime * 1.5f;
    pushConstants.emitterRadius = 0.2f; 
    pushConstants.emissionRate = _params.emissionRate;
    pushConstants.pad1 = 0.0f;
    pushConstants.emitterPos = _transform.translation; 
    pushConstants.pad2 = 0.0f;
    pushConstants.emitterScale = glm::vec3(1.0f, 1.0f, 1.0f);
    pushConstants.pad3 = 0.0f;
    pushConstants.emitterRot = glm::vec3(0.0f, 0.0f, 0.0f); 
    pushConstants.pad4 = 0.0f;
    pushConstants.convergencePoint = glm::vec3(0.0f, 3.0f, 0.0f);
    pushConstants.convergenceStrength = 1.0f; 
    pushConstants.totalSmokeDistance = 1.5f; 
    pushConstants.updraft = 1.2f;
    pushConstants.randSeed = static_cast<float>(std::chrono::high_resolution_clock::now().time_since_epoch().count() % 100000);
    pushConstants._pad5 = 0.0f;
    
    vkCmdPushConstants(_commandBuffer, m_computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pushConstants);
    uint32_t groupCount = (_params.maxParticles + 255) / 256;
    vkCmdDispatch(_commandBuffer, groupCount, 1, 1);
    
    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    
    vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 1, &memoryBarrier,  0, nullptr, 0, nullptr);

} 