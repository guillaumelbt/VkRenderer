
#include "app/Application.h"
#include <cassert>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/common.hpp>

#include <iostream>
#include "systems/RenderSystem.h"
#include "systems/PointLightSystem.h"
#include "systems/ParticleRenderSystem.h"
#include "components/ParticleSystemComponent.h"
#include "window/MovementController.h"
#include <chrono>
#include "core/Buffer.h"
#include "camera/Camera.h"
// #include "model/GameObject.h"
#include "model/Model.h"
#include "components/TransformComponent.h"
#include "components/ModelComponent.h"
#include "components/PointLightComponent.h"
#include "core/Texture.h"
#include "core/Descriptors.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>


Application::Application()
{
    m_globalPool = DescriptorPool::Builder(m_device)
        .SetMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT * 4)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT * 3)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT * 2)
        .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
		.Build();
		
	LoadGameObjects();
	
	m_imguiInterface = std::make_unique<ImGuiInterface>(m_device, m_window, m_renderer, m_ec);
	m_imguiInterface->SetDescriptorPool(m_globalPool->GetVkDescriptorPool());
	m_imguiInterface->Initialize();
	m_imguiInterface->SetViewerEntity(m_viewerEntity);
	m_imguiInterface->SetParticleEntity(m_particleEntity);
}

Application::~Application()
{
    vkDeviceWaitIdle(m_device.GetDevice());
}

void Application::Run()
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_device.GetPhysicalDevice(), &properties);
    VkDeviceSize minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
    
    std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) 
    {
        uboBuffers[i] = std::make_unique<Buffer>(m_device, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, minUboAlignment);
        uboBuffers[i]->Map();
    }

    auto globalSetLayout = DescriptorSetLayout::Builder(m_device)
        .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .Build();

    std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) 
    {
        auto bufferInfo = uboBuffers[i]->DescriptorInfo();
        DescriptorWriter(*globalSetLayout, *m_globalPool)
            .WriteBuffer(0, &bufferInfo)
            .Build(globalDescriptorSets[i]);
    }

    static std::unique_ptr<DescriptorSetLayout> textureSetLayout = DescriptorSetLayout::Builder(m_device)
        .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build();
    static std::unique_ptr<DescriptorPool> texturePool = DescriptorPool::Builder(m_device)
        .SetMaxSets(100)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
        .Build();

    RenderSystem renderSystem{m_device, m_renderer.GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout(), textureSetLayout->GetDescriptorSetLayout(), m_renderer.GetMsaaSamples() };
    PointLightSystem pointLightSystem{m_device, m_renderer.GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout(), m_renderer.GetMsaaSamples() };
    
    auto particleSetLayout = DescriptorSetLayout::Builder(m_device)
        .AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .Build();

    ParticleSystemComponent particleParams;
    particleParams.maxParticles = 3000;
    particleParams.emissionRate = 800.0f;
    particleParams.particleLifetime = 5.0f;
    particleParams.particleSize = 0.1f;
    particleParams.initialVelocity = glm::vec3(0.0f, 2.0f, 0.0f);
    particleParams.velocityVariation = glm::vec3(1.0f, 0.5f, 1.0f);
    particleParams.color = glm::vec3(1.0f, 0.3f, 0.0f);
    particleParams.colorVariation = glm::vec3(0.3f, 0.2f, 0.1f);
    
    ParticleRenderSystem particleSystem(
        m_device,
        m_renderer.GetSwapChainRenderPass(),
        particleSetLayout->GetDescriptorSetLayout(),
        m_renderer.GetMsaaSamples(),
        particleParams.maxParticles,
        particleParams.particleSize,
        particleParams.color
    );

    std::vector<VkDescriptorSet> particleDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) 
    {
        VkDescriptorBufferInfo uboInfo = uboBuffers[i]->DescriptorInfo();
        VkDescriptorBufferInfo particleBufferInfo = particleSystem.GetParticleBuffer()->DescriptorInfo();
        DescriptorWriter(*particleSetLayout, *m_globalPool)
            .WriteBuffer(0, &uboInfo)
            .WriteBuffer(1, &particleBufferInfo)
            .Build(particleDescriptorSets[i]);
    }

    Camera camera{};
    glm::vec3 cameraPosition = glm::vec3(-1.f, -2.f, -2.f);
    glm::vec3 targetPosition = glm::vec3(0.f, 0.f, 2.5f);
    camera.SetViewTarget(cameraPosition, targetPosition);

    MovementController cameraController{};
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!m_window.ShouldClose())
    {
        glfwPollEvents();
        
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        m_imguiInterface->Render();
        
        ImGui::Render();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.MoveInPlaneXZ(m_window.GetWindow(), frameTime, m_viewerEntity, m_ec);

        if (m_ec.HasComponent<TransformComponent>(m_viewerEntity))
        {
            auto& transform = m_ec.GetComponent<TransformComponent>(m_viewerEntity);
            camera.SetViewYXZ(transform.translation, transform.rotation);
        }

        float aspectRatio = m_renderer.GetAspectRatio();
        float fovY = glm::radians(50.0f);
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        camera.SetPerspectiveProjection(fovY, aspectRatio, nearPlane, farPlane);

        if (auto commandBuffer = m_renderer.BeginFrame()) 
        {
            int frameIndex = m_renderer.GetFrameIndex();
            FrameInfo frameInfo{ frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], &m_ec };

            GlobalUbo ubo{};
            ubo.projection = camera.GetProjection();
            ubo.view = camera.GetView();
            ubo.inverseView = camera.GetInverseView();

            pointLightSystem.Update(frameInfo, ubo);

            uboBuffers[frameIndex]->WriteToBuffer(&ubo, sizeof(GlobalUbo));
            uboBuffers[frameIndex]->Flush(VK_WHOLE_SIZE);

            if (m_ec.HasComponent<ParticleSystemComponent>(m_particleEntity) && 
                m_ec.HasComponent<TransformComponent>(m_particleEntity))
            {
                auto& particleTransform = m_ec.GetComponent<TransformComponent>(m_particleEntity);
                auto& particleComponent = m_ec.GetComponent<ParticleSystemComponent>(m_particleEntity);
                particleSystem.UpdateParticlesWithCompute(frameTime, commandBuffer, particleComponent, particleTransform);
            }

            m_renderer.BeginSwapChainRenderPass(commandBuffer);
            renderSystem.RenderGameObjects(frameInfo);
            pointLightSystem.Render(frameInfo);
            
            if (m_ec.HasComponent<ParticleSystemComponent>(m_particleEntity))
            {
                particleSystem.Render(frameInfo, particleDescriptorSets[frameIndex]);
            }
            
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            
            m_renderer.EndSwapChainRenderPass(commandBuffer);
            m_renderer.EndFrame();
        }
    }
    vkDeviceWaitIdle(m_device.GetDevice());
}


void Application::LoadGameObjects()
{
     std::vector<glm::vec3> lightColors{
      {1.f, .1f, .1f},
      {.1f, .1f, 1.f},
      {.1f, 1.f, .1f},
      {1.f, 1.f, .1f},
      {.1f, 1.f, 1.f},
      {1.f, 1.f, 1.f}
     };

     for (int i = 0; i < lightColors.size(); i++)
     {
         Entity light = m_ec.CreateEntity();
         auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), { 0.f, 1.f, 0.f });
         TransformComponent transform{};
         transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, 1.f, -1.f, 1.f));
         m_ec.AddComponent(light, transform);
         
         PointLightComponent lightComp{};
         lightComp.lightIntensity = 0.2f;
         lightComp.color = lightColors[i];
         m_ec.AddComponent(light, lightComp);
     }

    m_viewerEntity = m_ec.CreateEntity();
    m_ec.AddComponent(m_viewerEntity, TransformComponent{glm::vec3(0.f, 0.f, -2.5f)});
    
    m_particleEntity = m_ec.CreateEntity();
    m_ec.AddComponent(m_particleEntity, TransformComponent{glm::vec3(1.0f, 0.0f, 4.0f)});
    
        ParticleSystemComponent particleParams;
    particleParams.maxParticles = 3000;
    particleParams.emissionRate = 800.0f; 
    particleParams.particleSize = 0.05f; 
    particleParams.particleLifetime = 5.0f; 
    particleParams.initialVelocity = glm::vec3(0.0f, 2.0f, 0.0f);
    particleParams.velocityVariation = glm::vec3(1.0f, 0.5f, 1.0f);
    particleParams.color = glm::vec3(1.0f, 0.3f, 0.0f); 
    particleParams.colorVariation = glm::vec3(0.3f, 0.2f, 0.1f);

    m_ec.AddComponent(m_particleEntity, particleParams);
    
    static std::unique_ptr<DescriptorSetLayout> textureSetLayout = DescriptorSetLayout::Builder(m_device)
        .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build();
    static std::unique_ptr<DescriptorPool> texturePool = DescriptorPool::Builder(m_device)
        .SetMaxSets(100)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
        .Build();

    auto model = Model::CreateModelWithTexture(m_device, "models/viking_room.obj", "textures/viking_room.png", *textureSetLayout, *texturePool);

    Entity viking = m_ec.CreateEntity();
    m_ec.AddComponent(viking, TransformComponent
    {
        glm::vec3(0.0f, -0.5f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(-glm::half_pi<float>(), 0.0f, -glm::quarter_pi<float>() * 5)
    });
    ModelComponent vikingModelComp{};
    vikingModelComp.model = model;
    vikingModelComp.textureDescriptorSet = model->GetTextureDescriptorSet();
    vikingModelComp.color = glm::vec3(1.0f, 1.0f, 1.0f);
    m_ec.AddComponent(viking, vikingModelComp);
}