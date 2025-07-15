#pragma once
#include "window/Window.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

struct QueueFamilyIndices 
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()  {    return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device
{
public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    Device(Window &_window);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    VkCommandPool GetCommandPool() { return m_commandPool; }
    VkDevice GetDevice() { return m_device; }
    VkSurfaceKHR GetSurface() { return m_surface; }
    VkQueue GetGraphicsQueue() { return m_graphicsQueue; }
    VkQueue GetPresentQueue() { return m_presentQueue; }
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }

    SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_physicalDevice); }
    uint32_t FindMemoryType(uint32_t _typeFilter, VkMemoryPropertyFlags _properties);
    QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_physicalDevice); }
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features);

    void CreateBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferMemory);
    void CreateBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferMemory, VkDeviceSize& _allocationSize);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer _commandBuffer);
    void CopyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size);
    void CopyBufferToImage(VkBuffer _buffer, VkImage _image, uint32_t _width, uint32_t _height, uint32_t _layerCount);

    void CreateImageWithInfo( const VkImageCreateInfo& _imageInfo, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory);

    VkPhysicalDeviceProperties properties;
     VkSampleCountFlagBits  GetMaxUsableSampleCount();

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();

    bool IsDeviceSuitable(VkPhysicalDevice _device);
    std::vector<const char*> GetRequiredExtensions();
    bool CheckValidationLayerSupport();
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice _device);
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
    void HasGflwRequiredInstanceExtensions();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice _device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice _device);
   


    Window& m_window;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool;

    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
};

