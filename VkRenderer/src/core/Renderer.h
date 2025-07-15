#pragma once
#include "window/Window.h"
#include "core/Device.h"
#include "core/SwapChain.h"
#include <vulkan/vulkan.h>
#include <cassert>
#include <memory>
#include <vector>

class Renderer
{
public:
    Renderer(Window& _window, Device& _device);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    VkRenderPass GetSwapChainRenderPass() const { return m_swapChain->GetRenderPass(); }
    bool IsFrameInProgress() const { return m_isFrameStarted; }
    float GetAspectRatio() const { return m_swapChain->GetExtentAspectRatio(); };

    VkCommandBuffer GetCurrentCommandBuffer() const 
    {
        assert(m_isFrameStarted && "Cannot get command buffer when frame not in progress");
        return m_commandBuffers[m_currentFrameIndex];
    }

    int GetFrameIndex() const 
    {
        assert(m_isFrameStarted && "Cannot get frame index when frame not in progress");
        return m_currentFrameIndex;
    }

    VkCommandBuffer BeginFrame();

    void EndFrame();
    void BeginSwapChainRenderPass(VkCommandBuffer _commandBuffer);
    void EndSwapChainRenderPass(VkCommandBuffer _commandBuffer);
    VkSampleCountFlagBits GetMsaaSamples() const { return m_swapChain->GetMsaaSamples(); }

private:
    void CreateCommandBuffers();
    void FreeCommandBuffers();
    void RecreateSwapChain();

    Window& m_window;
    Device& m_device;


    std::unique_ptr<SwapChain> m_swapChain;
    std::vector<VkCommandBuffer> m_commandBuffers;

    uint32_t m_currentImageIndex;
    int m_currentFrameIndex;
    bool m_isFrameStarted;
};

