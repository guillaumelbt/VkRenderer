#pragma once
#include <vulkan/vulkan.h>
#include "Device.h"
#include <string>
#include <memory>
#include <cmath>

class Texture 
{
public:
    Texture();
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool LoadFromFile(const std::string& _filename, Device& _device, VkQueue _graphicsQueue);

    VkImageView GetImageView() const { return m_imageView; }
    VkSampler GetSampler() const { return m_sampler; }
    VkImage GetImage() const { return m_image; }
    uint32_t GetMipLevels() const { return m_mipLevels; }

private:
    void CreateImage(Device& _device, uint32_t _width, uint32_t _height, uint32_t _mipLevels, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageMemory);
    void TransitionImageLayout(Device& _device, VkQueue _graphicsQueue, VkImage _image, VkFormat _format, VkImageLayout _oldLayout, VkImageLayout _newLayout, uint32_t _mipLevels);
    void CopyBufferToImage(Device& _device, VkQueue _graphicsQueue, VkBuffer _buffer, VkImage _image, uint32_t _width, uint32_t _height);
    void CreateImageView(Device& _device, VkImage _image, VkFormat _format, uint32_t _mipLevels);
    void CreateSampler(Device& _device);
    void GenerateMipmaps(Device& _device, VkImage _image, VkFormat _imageFormat, int32_t _texWidth, int32_t _texHeight, uint32_t _mipLevels);
    void Cleanup(Device& _device);

    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_mipLevels = 1;
    bool m_loaded = false;
};
