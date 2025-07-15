#pragma once

#include "Device.h"

class Buffer
{
public:
    Buffer(Device& _device, VkDeviceSize _instanceSize, uint32_t _instanceCount, VkBufferUsageFlags _usageFlags, VkMemoryPropertyFlags _memoryPropertyFlags, VkDeviceSize _minOffsetAlignment = 1);
    Buffer(Device& _device, VkDeviceSize _instanceSize, uint32_t _instanceCount, VkBufferUsageFlags _usageFlags, VkMemoryPropertyFlags _memoryPropertyFlags, VkDeviceSize _minOffsetAlignment, VkDeviceSize allocationSize);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    VkResult Map(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
    void Unmap();

    void WriteToBuffer(void* _data, VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
    VkResult Flush(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
    VkDescriptorBufferInfo DescriptorInfo(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
    VkResult Invalidate(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);

    void WriteToIndex(void* _data, int _index);
    VkResult FlushIndex(int _index);
    VkDescriptorBufferInfo DescriptorInfoForIndex(int _index);
    VkResult InvalidateIndex(int _index);

    VkBuffer GetBuffer() const { return m_buffer; }
    void* GetMappedMemory() const { return m_mapped; }
    uint32_t GetInstanceCount() const { return m_instanceCount; }
    VkDeviceSize GetInstanceSize() const { return m_instanceSize; }
    VkDeviceSize GetAlignmentSize() const { return m_instanceSize; }
    VkBufferUsageFlags GetUsageFlags() const { return m_usageFlags; }
    VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_memoryPropertyFlags; }
    VkDeviceSize GetBufferSize() const { return m_bufferSize; }
    VkDeviceSize GetAllocationSize() const { return m_allocationSize; }

private:
    static VkDeviceSize GetAlignment(VkDeviceSize _instanceSize, VkDeviceSize _minOffsetAlignment);

    Device& m_device;
    void* m_mapped = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

    VkDeviceSize m_bufferSize;
    uint32_t m_instanceCount;
    VkDeviceSize m_instanceSize;
    VkDeviceSize m_alignmentSize;
    VkBufferUsageFlags m_usageFlags;
    VkMemoryPropertyFlags m_memoryPropertyFlags;
    VkDeviceSize m_allocationSize = 0;
};

