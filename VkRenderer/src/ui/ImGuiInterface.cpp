#include "ui/ImGuiInterface.h"
#include "components/TransformComponent.h"
#include "components/ModelComponent.h"
#include "components/PointLightComponent.h"
#include "components/ParticleSystemComponent.h"
#include "model/Model.h"
#include "core/Descriptors.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <filesystem>
#include <algorithm>
#include <iostream>

ImGuiInterface::ImGuiInterface(Device& _device, Window& _window, Renderer& _renderer, EntityComponentSystem& _ec) : m_device(_device), m_window(_window), m_renderer(_renderer), m_ec(_ec)
{

}

ImGuiInterface::~ImGuiInterface()
{
    Cleanup();
}

void ImGuiInterface::Initialize()
{
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(m_window.GetWindow(), true);

    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_0;
    info.Instance = m_device.GetInstance();
    info.PhysicalDevice = m_device.GetPhysicalDevice();
    info.Device = m_device.GetDevice();
    info.QueueFamily = m_device.FindPhysicalQueueFamilies().graphicsFamily.value();
    info.Queue = m_device.GetGraphicsQueue();
    info.DescriptorPool = m_descriptorPool;
    info.RenderPass = m_renderer.GetSwapChainRenderPass();
    info.MinImageCount = 2;
    info.ImageCount = 2;
    info.MSAASamples = m_renderer.GetMsaaSamples();
    info.PipelineCache = VK_NULL_HANDLE;
    info.Subpass = 0;

    ImGui_ImplVulkan_Init(&info);

    ScanAvailableModels();
}

void ImGuiInterface::Cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiInterface::Render()
{
    ShowDebugWindow();
    ShowSceneHierarchy();

    if (m_showInspector)
    {
        ShowInspector();
    }
}

void ImGuiInterface::ShowDebugWindow()
{
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

    ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_None);

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Objects: %zu", m_ec.GetEntityCount());

    ImGui::Separator();

    if (m_ec.HasComponent<TransformComponent>(m_viewerEntity))
    {
        auto& transform = m_ec.GetComponent<TransformComponent>(m_viewerEntity);
        ImGui::Text("Camera Position:");
        ImGui::Text("  X: %.2f", transform.translation.x);
        ImGui::Text("  Y: %.2f", transform.translation.y);
        ImGui::Text("  Z: %.2f", transform.translation.z);
    }

    ImGui::End();
}

void ImGuiInterface::ShowSceneHierarchy()
{
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(320, 10), ImGuiCond_FirstUseEver);

    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_None);

    if (ImGui::Button("Add Object", ImVec2(-1, 0)))
    {
        CreateNewEntity();
    }

    ImGui::Separator();
    ImGui::Text("Objects in Scene:");

    ImGui::BeginChild("ObjectList", ImVec2(0, 0), true);

    m_ec.ForEach<TransformComponent>([&](Entity entity, TransformComponent& transform)
    {
        if (entity == m_viewerEntity)
        {
            return;
        }

        std::string entityInfo = "Object " + std::to_string(entity);

        if (m_ec.HasComponent<ModelComponent>(entity))
        {
            entityInfo += " [Model]";
        }
        if (m_ec.HasComponent<PointLightComponent>(entity))
        {
            entityInfo += " [Light]";
        }
        if (m_ec.HasComponent<ParticleSystemComponent>(entity))
        {
            entityInfo += " [Particles]";
        }

        bool isSelected = (m_selectedEntity == entity);
        if (ImGui::Selectable(entityInfo.c_str(), isSelected))
        {
            m_selectedEntity = entity;
            m_showInspector = true;

            if (m_ec.HasComponent<TransformComponent>(entity))
            {
                auto& t = m_ec.GetComponent<TransformComponent>(entity);
                m_editPosition[0] = t.translation.x;
                m_editPosition[1] = t.translation.y;
                m_editPosition[2] = t.translation.z;
                m_editRotation[0] = t.rotation.x;
                m_editRotation[1] = t.rotation.y;
                m_editRotation[2] = t.rotation.z;
                m_editScale[0] = t.scale.x;
                m_editScale[1] = t.scale.y;
                m_editScale[2] = t.scale.z;
            }

            if (m_ec.HasComponent<PointLightComponent>(entity))
            {
                auto& l = m_ec.GetComponent<PointLightComponent>(entity);
                m_editIntensity = l.lightIntensity;
                m_editColor[0] = l.color.r;
                m_editColor[1] = l.color.g;
                m_editColor[2] = l.color.b;
            }
            else if (m_ec.HasComponent<ModelComponent>(entity))
            {
                auto& m = m_ec.GetComponent<ModelComponent>(entity);
                m_editColor[0] = m.color.r;
                m_editColor[1] = m.color.g;
                m_editColor[2] = m.color.b;
            }
        }
    });

    ImGui::EndChild();
    ImGui::End();
}

void ImGuiInterface::ShowInspector()
{
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(680, 10), ImGuiCond_FirstUseEver);

    ImGui::Begin("Inspector", &m_showInspector, ImGuiWindowFlags_None);

    if (m_selectedEntity == UINT32_MAX || !m_ec.HasComponent<TransformComponent>(m_selectedEntity))
    {
        ImGui::Text("No object selected or entity no longer exists.");
        ImGui::End();
        return;
    }

    ImGui::Text("Object ID: %u", m_selectedEntity);
    ImGui::Separator();

    ImGui::Text("Transform Component");
    ImGui::Separator();

    if (ImGui::DragFloat3("Position", m_editPosition, 0.1f))
    {
        auto& transform = m_ec.GetComponent<TransformComponent>(m_selectedEntity);
        transform.translation = glm::vec3(m_editPosition[0], m_editPosition[1], m_editPosition[2]);
    }

    if (ImGui::DragFloat3("Rotation", m_editRotation, 0.01f))
    {
        auto& transform = m_ec.GetComponent<TransformComponent>(m_selectedEntity);
        transform.rotation = glm::vec3(m_editRotation[0], m_editRotation[1], m_editRotation[2]);
    }

    if (ImGui::DragFloat3("Scale", m_editScale, 0.1f, 0.1f, 10.0f))
    {
        auto& transform = m_ec.GetComponent<TransformComponent>(m_selectedEntity);
        transform.scale = glm::vec3(m_editScale[0], m_editScale[1], m_editScale[2]);
    }

    ImGui::Spacing();

    if (m_ec.HasComponent<ModelComponent>(m_selectedEntity))
    {
        ImGui::Text("Model Component");
        ImGui::Separator();
        ImGui::Text("Model is loaded and rendering.");

        if (ImGui::ColorEdit3("Model Color", m_editColor))
        {
            auto& modelComponent = m_ec.GetComponent<ModelComponent>(m_selectedEntity);
            modelComponent.color = glm::vec3(m_editColor[0], m_editColor[1], m_editColor[2]);
        }

        if (ImGui::Button("Remove Model Component"))
        {
            vkDeviceWaitIdle(m_device.GetDevice());

            if (m_ec.HasComponent<ModelComponent>(m_selectedEntity))
            {
                auto& modelComponent = m_ec.GetComponent<ModelComponent>(m_selectedEntity);
                modelComponent.textureDescriptorSet = VK_NULL_HANDLE; // Reset descriptor set
                modelComponent.model.reset(); // Release shared_ptr reference
            }
            m_ec.RemoveComponent<ModelComponent>(m_selectedEntity);
        }

        ImGui::Spacing();
    }


    if (m_ec.HasComponent<PointLightComponent>(m_selectedEntity))
    {
        ImGui::Text("Point Light Component");
        ImGui::Separator();

        if (ImGui::SliderFloat("Light Intensity", &m_editIntensity, 0.0f, 5.0f))
        {
            auto& lightComponent = m_ec.GetComponent<PointLightComponent>(m_selectedEntity);
            lightComponent.lightIntensity = m_editIntensity;
        }

        if (ImGui::ColorEdit3("Light Color", m_editColor))
        {
            auto& lightComponent = m_ec.GetComponent<PointLightComponent>(m_selectedEntity);
            lightComponent.color = glm::vec3(m_editColor[0], m_editColor[1], m_editColor[2]);
        }

        if (ImGui::Button("Remove Point Light Component"))
        {
            m_ec.RemoveComponent<PointLightComponent>(m_selectedEntity);
        }

        ImGui::Spacing();
    }

    if (m_ec.HasComponent<ParticleSystemComponent>(m_selectedEntity))
    {
        ImGui::Text("Particle System Component");
        ImGui::Separator();
        ImGui::Text("Particle system is active.");

        if (ImGui::Button("Remove Particle System"))
        {
            vkDeviceWaitIdle(m_device.GetDevice());
            m_ec.RemoveComponent<ParticleSystemComponent>(m_selectedEntity);
        }

        ImGui::Spacing();
    }

    ImGui::Separator();

    if (ImGui::Button("Add Component", ImVec2(-1, 0)))
    {
        m_showAddComponent = !m_showAddComponent;
    }


    //TODO refactor to be modular
    if (m_showAddComponent)
    {
        ImGui::Separator();
        ImGui::Text("Add Component:");

        if (!m_ec.HasComponent<ModelComponent>(m_selectedEntity))
        {
            if (ImGui::Button("Add Model Component"))
            {
                if (m_editSelectedModel >= 0 && m_editSelectedModel < m_availableModels.size())
                {
                    vkDeviceWaitIdle(m_device.GetDevice());
                    std::string modelPath = "models/" + m_availableModels[m_editSelectedModel];
                    std::string texturePath = "";

                    if (m_availableModels[m_editSelectedModel] == "viking_room.obj")
                    {
                        texturePath = "textures/viking_room.png";
                    }

                    static std::unique_ptr<DescriptorSetLayout> textureSetLayout = DescriptorSetLayout::Builder(m_device)
                        .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        .Build();
                    static std::unique_ptr<DescriptorPool> texturePool = DescriptorPool::Builder(m_device)
                        .SetMaxSets(100)
                        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                        .Build();

                    std::shared_ptr<Model> model;
                    if (!texturePath.empty())
                    {
                        model = Model::CreateModelWithTexture(m_device, modelPath, texturePath, *textureSetLayout, *texturePool);
                    }
                    else
                    {
                        model = Model::CreateModelFromFile(m_device, modelPath);
                    }

                    ModelComponent modelComp{};
                    modelComp.model = model;
                    modelComp.color = glm::vec3(m_editColor[0], m_editColor[1], m_editColor[2]); // Set initial color
                    if (!texturePath.empty())
                    {
                        modelComp.textureDescriptorSet = model->GetTextureDescriptorSet();
                    }
                    m_ec.AddComponent(m_selectedEntity, modelComp);
                    m_showAddComponent = false;
                }
            }

            if (!m_availableModels.empty())
            {
                std::vector<std::string> tempDisplayNames;
                for (const auto& modelFile : m_availableModels)
                {
                    std::string displayName = modelFile;
                    if (displayName.size() > 4 && displayName.substr(displayName.size() - 4) == ".obj")
                    {
                        displayName = displayName.substr(0, displayName.size() - 4);
                    }
                    tempDisplayNames.push_back(displayName);
                }

                std::string currentSelection = "Select Model";
                if (m_editSelectedModel >= 0 && m_editSelectedModel < tempDisplayNames.size())
                {
                    currentSelection = tempDisplayNames[m_editSelectedModel];
                }

                if (ImGui::BeginCombo("Model Type", currentSelection.c_str()))
                {
                    for (int i = 0; i < tempDisplayNames.size(); i++)
                    {
                        bool isSelected = (m_editSelectedModel == i);
                        if (ImGui::Selectable(tempDisplayNames[i].c_str(), isSelected))
                        {
                            m_editSelectedModel = i;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }



        if (!m_ec.HasComponent<PointLightComponent>(m_selectedEntity))
        {
            if (ImGui::Button("Add Point Light Component"))
            {
                PointLightComponent lightComp{};
                lightComp.lightIntensity = m_editIntensity;
                lightComp.color = glm::vec3(m_editColor[0], m_editColor[1], m_editColor[2]); // Set initial color
                m_ec.AddComponent(m_selectedEntity, lightComp);
                m_showAddComponent = false;
            }
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Delete Object", ImVec2(-1, 0)))
    {
        if (m_selectedEntity != m_viewerEntity)
        {
            vkDeviceWaitIdle(m_device.GetDevice());
            m_ec.DestroyEntity(m_selectedEntity);
            m_showInspector = false;
            m_selectedEntity = UINT32_MAX;
        }
        else
        {
            ImGui::OpenPopup("Cannot Delete");
        }
    }

    if (ImGui::BeginPopupModal("Cannot Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Cannot delete camera object (needed for controls)!");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void ImGuiInterface::CreateNewEntity()
{
    vkDeviceWaitIdle(m_device.GetDevice());

    Entity newEntity = m_ec.CreateEntity();

    TransformComponent transform{};
    transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    m_ec.AddComponent(newEntity, transform);

    m_selectedEntity = newEntity;
    m_showInspector = true;

    m_editPosition[0] = 0.0f;
    m_editPosition[1] = 0.0f;
    m_editPosition[2] = 0.0f;
    m_editRotation[0] = 0.0f;
    m_editRotation[1] = 0.0f;
    m_editRotation[2] = 0.0f;
    m_editScale[0] = 1.0f;
    m_editScale[1] = 1.0f;
    m_editScale[2] = 1.0f;
    m_editColor[0] = 1.0f;
    m_editColor[1] = 1.0f;
    m_editColor[2] = 1.0f;
    m_editIntensity = 1.0f;
    m_editSelectedModel = 0;
}

void ImGuiInterface::ScanAvailableModels()
{
    m_availableModels.clear();

    try
    {
        for (const auto& entry : std::filesystem::directory_iterator("models/"))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".obj")
            {
                m_availableModels.push_back(entry.path().filename().string());
            }
        }

        std::sort(m_availableModels.begin(), m_availableModels.end());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cout << "Could not scan models directory: " << e.what() << std::endl;
        m_availableModels = {};
    }
} 