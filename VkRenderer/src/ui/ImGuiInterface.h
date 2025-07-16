#pragma once
#include "core/Device.h"
#include "core/Renderer.h"
#include "systems/EntityComponentSystem.h"
#include "window/Window.h"
#include <vector>
#include <string>

class ImGuiInterface
{
public:
    ImGuiInterface(Device& _device, Window& _window, Renderer& _renderer, EntityComponentSystem& _ec);
    ~ImGuiInterface();

    ImGuiInterface(const ImGuiInterface&) = delete;
    void operator=(const ImGuiInterface&) = delete;

    void Initialize();
    void Render();
    void Cleanup();

    void SetDescriptorPool(VkDescriptorPool _pool) 
    { 
        m_descriptorPool = _pool; 
    }

    void SetViewerEntity(Entity entity) 
    {
        m_viewerEntity = entity; 
    }

    void SetParticleEntity(Entity entity)
    { 
        m_particleEntity = entity;
    }

private:
    void ShowDebugWindow();
    void ShowSceneHierarchy();
    void ShowInspector();
    void CreateNewEntity();
    void ScanAvailableModels();

    Device& m_device;
    Window& m_window;
    Renderer& m_renderer;
    EntityComponentSystem& m_ec;
    Entity m_viewerEntity = UINT32_MAX;
    Entity m_particleEntity = UINT32_MAX;
    Entity m_selectedEntity = UINT32_MAX;

    bool m_showInspector = false;

    float m_editPosition[3] = {0.0f, 0.0f, 0.0f};
    float m_editRotation[3] = {0.0f, 0.0f, 0.0f};
    float m_editScale[3] = {1.0f, 1.0f, 1.0f};
    float m_editColor[3] = {1.0f, 1.0f, 1.0f};
    float m_editIntensity = 1.0f;

    int m_editSelectedModel = 0;

    bool m_showAddComponent = false;

    std::vector<std::string> m_availableModels;
    
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
}; 