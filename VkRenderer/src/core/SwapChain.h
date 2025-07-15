#pragma once
#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include "Device.h";
#include <memory>

class SwapChain
{
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChain(Device& _device, VkExtent2D _extent);
    SwapChain(Device& _device, VkExtent2D _extent, std::shared_ptr<SwapChain> _previous);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    void operator=(const SwapChain&) = delete;

    VkFramebuffer GetFrameBuffer(int _index) { return m_swapChainFramebuffers[_index]; }
    VkRenderPass GetRenderPass() { return m_renderPass; }
    VkImageView GetImageView(int _index) { return m_swapChainImageViews[_index]; }
    size_t GetImageCount() { return m_swapChainImages.size(); }
    VkFormat GetSwapChainImageFormat() { return m_swapChainImageFormat; }
    VkExtent2D GetSwapChainExtent() { return m_swapChainExtent; }
    uint32_t GetWidth() { return m_swapChainExtent.width; }
    uint32_t GetHeight() { return m_swapChainExtent.height; }
    float GetExtentAspectRatio() { return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height); }

    VkFormat FindDepthFormat();

    VkResult AcquireNextImage(uint32_t* _imageIndex);
    VkResult SubmitCommandBuffers(const VkCommandBuffer* _buffers, uint32_t* _imageIndex);

    bool CompareSwapFormats(const SwapChain& _swapChain) const { return _swapChain.m_swapChainDepthFormat == m_swapChainDepthFormat &&  _swapChain.m_swapChainImageFormat == m_swapChainImageFormat;}

    VkSampleCountFlagBits GetMsaaSamples() const { return m_msaaSamples; }

private:
    void Init();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateColorResources();
    void CreateDepthResources();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateSyncObjects();

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);

    VkFormat m_swapChainImageFormat;
    VkFormat m_swapChainDepthFormat; 
    VkExtent2D m_swapChainExtent;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    VkRenderPass m_renderPass;

    std::vector<VkImage> m_depthImages;
    std::vector<VkDeviceMemory> m_depthImageMemorys;
    std::vector<VkImageView> m_depthImageViews;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;

    Device& m_device;
    VkExtent2D m_windowExtent;

    VkSwapchainKHR m_swapChain;
    std::shared_ptr<SwapChain> m_oldSwapChain;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    size_t m_currentFrame = 0;

    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage m_colorImage = VK_NULL_HANDLE;
    VkDeviceMemory m_colorImageMemory = VK_NULL_HANDLE;
    VkImageView m_colorImageView = VK_NULL_HANDLE;
};

