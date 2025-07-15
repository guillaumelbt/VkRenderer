#include "SwapChain.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

SwapChain::SwapChain(Device& _device, VkExtent2D _extent) : m_device{ _device }, m_windowExtent{ _extent }
{
    m_msaaSamples = m_device.GetMaxUsableSampleCount();
    Init();
}

SwapChain::SwapChain(Device& _device, VkExtent2D _extent, std::shared_ptr<SwapChain> _previous) : m_device{ _device }, m_windowExtent{ _extent }, m_oldSwapChain { _previous }
{
    m_msaaSamples = m_device.GetMaxUsableSampleCount();
    Init();
    m_oldSwapChain = nullptr;
}

void SwapChain::Init()
{
    CreateSwapChain();
    CreateImageViews();
    CreateColorResources();
    CreateDepthResources();
    CreateRenderPass();
    CreateFramebuffers();
    CreateSyncObjects();
}

SwapChain::~SwapChain() 
{
    if (m_colorImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device.GetDevice(), m_colorImageView, nullptr);
    }
    if (m_colorImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device.GetDevice(), m_colorImage, nullptr);
    }
    if (m_colorImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device.GetDevice(), m_colorImageMemory, nullptr);
    }
    for (auto imageView : m_swapChainImageViews) 
    {
        vkDestroyImageView(m_device.GetDevice(), imageView, nullptr);
    }
    m_swapChainImageViews.clear();

    if (m_swapChain != nullptr) 
    {
        vkDestroySwapchainKHR(m_device.GetDevice(), m_swapChain, nullptr);
        m_swapChain = nullptr;
    }

    for (int i = 0; i < m_depthImages.size(); i++) 
    {
        vkDestroyImageView(m_device.GetDevice(), m_depthImageViews[i], nullptr);
        vkDestroyImage(m_device.GetDevice(), m_depthImages[i], nullptr);
        vkFreeMemory(m_device.GetDevice(), m_depthImageMemorys[i], nullptr);
    }

    for (auto framebuffer : m_swapChainFramebuffers) 
    {
        vkDestroyFramebuffer(m_device.GetDevice(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(m_device.GetDevice(), m_renderPass, nullptr);

    for (size_t i = 0; i < m_imageAvailableSemaphores.size(); i++) 
    {
        vkDestroySemaphore(m_device.GetDevice(), m_imageAvailableSemaphores[i], nullptr);
    }
    
    for (size_t i = 0; i < m_renderFinishedSemaphores.size(); i++) 
    {
        vkDestroySemaphore(m_device.GetDevice(), m_renderFinishedSemaphores[i], nullptr);
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vkDestroyFence(m_device.GetDevice(), m_inFlightFences[i], nullptr);
    }
}

VkResult SwapChain::AcquireNextImage(uint32_t* _imageIndex) 
{
    vkWaitForFences(m_device.GetDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR( m_device.GetDevice(), m_swapChain, std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, _imageIndex);
    return result;
}

VkResult SwapChain::SubmitCommandBuffers( const VkCommandBuffer* buffers, uint32_t* imageIndex) 
{

    if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_device.GetDevice(), 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[*imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_device.GetDevice(), 1, &m_inFlightFences[m_currentFrame]);

    if (vkQueueSubmit(m_device.GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) 
        throw std::runtime_error("failed to submit draw command buffer");



    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;

    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(m_device.GetPresentQueue(), &presentInfo);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void SwapChain::CreateSwapChain() 
{
    SwapChainSupportDetails swapChainSupport = m_device.GetSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_device.GetSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = m_device.FindPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) 
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      
        createInfo.pQueueFamilyIndices = nullptr;  
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_oldSwapChain == nullptr ? VK_NULL_HANDLE : m_oldSwapChain->m_swapChain;
    
    if (vkCreateSwapchainKHR(m_device.GetDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) 
        throw std::runtime_error("failed to create swap chain");

    vkGetSwapchainImagesKHR(m_device.GetDevice(), m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device.GetDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void SwapChain::CreateImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) 
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_swapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device.GetDevice(), &viewInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture image view");
    }
}

void SwapChain::CreateColorResources() {
    VkFormat colorFormat = m_swapChainImageFormat;
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_swapChainExtent.width;
    imageInfo.extent.height = m_swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = m_msaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    m_device.CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_colorImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = colorFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_device.GetDevice(), &viewInfo, nullptr, &m_colorImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create color image view!");
    }
}

void SwapChain::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = m_msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat();
    depthAttachment.samples = m_msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = m_swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device.GetDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("failed to create render pass");
}

void SwapChain::CreateFramebuffers()
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            m_colorImageView,
            m_depthImageViews[i],
            m_swapChainImageViews[i]
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(m_device.GetDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void SwapChain::CreateDepthResources() 
{

    VkFormat depthFormat = FindDepthFormat();
    m_swapChainDepthFormat = depthFormat;
    VkExtent2D swapChainExtent = GetSwapChainExtent();

    m_depthImages.resize(GetImageCount());
    m_depthImageMemorys.resize(GetImageCount());
    m_depthImageViews.resize(GetImageCount());

    for (int i = 0; i < m_depthImages.size(); i++) 
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = m_msaaSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;


        m_device.CreateImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImages[i], m_depthImageMemorys[i]);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device.GetDevice(), &viewInfo, nullptr, &m_depthImageViews[i]) != VK_SUCCESS) 
            throw std::runtime_error("failed to craete texture image view");
        
    }
}

void SwapChain::CreateSyncObjects() 
{
    // Create semaphores for each swapchain image
    m_imageAvailableSemaphores.resize(GetImageCount());
    m_renderFinishedSemaphores.resize(GetImageCount());
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create semaphores for each swapchain image
    for (size_t i = 0; i < GetImageCount(); i++) 
    {
        if (vkCreateSemaphore(m_device.GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device.GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create synchronization objects for swapchain image");
        }
    }

    // Create fences for frame synchronization
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        if (vkCreateFence(m_device.GetDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create synchronization objects for a frame");
        }
    }
}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats) 
{
    
    for (const auto& availableFormat : _availableFormats) 
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return _availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes) 
{
    for (const auto& availablePresentMode : _availablePresentModes) 
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
        {
            std::cout << "Present mode : mailbox" << std::endl;
            return availablePresentMode;
        }
    }
    std::cout << "Present mode : fifo" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities) 
{
    if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return _capabilities.currentExtent;
    }
    else 
    {
        VkExtent2D actualExtent = m_windowExtent;
        actualExtent.width = std::max(_capabilities.minImageExtent.width, std::min(_capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(_capabilities.minImageExtent.height, std::min(_capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

VkFormat SwapChain::FindDepthFormat() 
{
    return m_device.FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}