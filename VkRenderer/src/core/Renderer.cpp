#include "Renderer.h"
#include <array>
#include <cassert>
#include <stdexcept>


Renderer::Renderer(Window& _window, Device& _device): m_window{ _window }, m_device{ _device } {
    RecreateSwapChain();
    CreateCommandBuffers();
}

Renderer::~Renderer() 
{ 
    FreeCommandBuffers(); 
}

void Renderer::RecreateSwapChain() 
{
    auto extent = m_window.GetExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = m_window.GetExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(m_device.GetDevice());

    if (m_swapChain == nullptr)
    {
        m_swapChain = std::make_unique<SwapChain>(m_device, extent);
    }
    else 
    {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
        m_swapChain = std::make_unique<SwapChain>(m_device, extent, oldSwapChain);

        if (!oldSwapChain->CompareSwapFormats(*m_swapChain.get())) 
            throw std::runtime_error("swap chain image(or depth) format has changed");
    }
}

void Renderer::CreateCommandBuffers() 
{
    m_commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device.GetCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device.GetDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) 
        throw std::runtime_error("failed to allocate command buffers!");
}


void Renderer::FreeCommandBuffers() 
{
    vkFreeCommandBuffers(m_device.GetDevice(), m_device.GetCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

    m_commandBuffers.clear();
}

VkCommandBuffer Renderer::BeginFrame()
{
    assert(!m_isFrameStarted && "Can't call beginFrame while already in progress");

    auto result = m_swapChain->AcquireNextImage(&m_currentImageIndex);
   
    if (result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        RecreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("failed to aquire swap chain image");

    m_isFrameStarted = true;

    auto commandBuffer = GetCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
        throw std::runtime_error("failed to begin recordind command buffer");
    
    return commandBuffer;
}

void Renderer::EndFrame() 
{
    assert(m_isFrameStarted && "Can't call endFrame while frame is not in progress");
    auto commandBuffer = GetCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
        throw std::runtime_error("fiailed to record command buffer");
    

    auto result = m_swapChain->SubmitCommandBuffers(&commandBuffer, &m_currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.WasWindowResized())
    { 
        m_window.ResetWindowResizedFlag();
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to present swapchain image");
    }

    m_isFrameStarted = false;
    m_currentFrameIndex = (m_currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::BeginSwapChainRenderPass(VkCommandBuffer _commandBuffer)
{
    assert(m_isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert( _commandBuffer == GetCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain->GetRenderPass();
    renderPassInfo.framebuffer = m_swapChain->GetFrameBuffer(m_currentImageIndex);

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChain->GetSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
    viewport.height = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{ {0, 0}, m_swapChain->GetSwapChainExtent() };
    
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
}

void Renderer::EndSwapChainRenderPass(VkCommandBuffer _commandBuffer) 
{
    assert(m_isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
    assert(_commandBuffer == GetCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

    vkCmdEndRenderPass(_commandBuffer);
}