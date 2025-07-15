#include "Buffer.h"
#include <cassert>
#include <cstring>

VkDeviceSize Buffer::GetAlignment(VkDeviceSize _instanceSize, VkDeviceSize _minOffsetAlignment) 
{
    if (_minOffsetAlignment > 0) 
    {
        return (_instanceSize + _minOffsetAlignment - 1) & ~(_minOffsetAlignment - 1);
    }
    return _instanceSize;
}

Buffer::Buffer(Device& _device, VkDeviceSize _instanceSize, uint32_t _instanceCount, VkBufferUsageFlags _usageFlags, VkMemoryPropertyFlags _memoryPropertyFlags, VkDeviceSize _minOffsetAlignment, VkDeviceSize _allocationSize)
    : m_device{ _device },  m_instanceSize{ _instanceSize }, m_instanceCount{ _instanceCount }, m_usageFlags{ _usageFlags }, m_memoryPropertyFlags{ _memoryPropertyFlags }
{
    m_alignmentSize = GetAlignment(_instanceSize, _minOffsetAlignment);
    m_bufferSize = m_alignmentSize * _instanceCount;
    VkDeviceSize realAllocSize = 0;
    _device.CreateBuffer(m_bufferSize, _usageFlags, _memoryPropertyFlags, m_buffer, m_memory, realAllocSize);
    m_allocationSize = realAllocSize;
}

Buffer::Buffer(Device& _device, VkDeviceSize _instanceSize, uint32_t _instanceCount, VkBufferUsageFlags _usageFlags, VkMemoryPropertyFlags _memoryPropertyFlags, VkDeviceSize _minOffsetAlignment)
    : Buffer(_device, _instanceSize, _instanceCount, _usageFlags, _memoryPropertyFlags, _minOffsetAlignment, _instanceSize * _instanceCount) 
{
}

Buffer::~Buffer() 
{
    Unmap();
    vkDestroyBuffer(m_device.GetDevice(), m_buffer, nullptr);
    vkFreeMemory(m_device.GetDevice(), m_memory, nullptr);
}


VkResult Buffer::Map(VkDeviceSize _size, VkDeviceSize _offset)
{
    assert(m_buffer && m_memory && "Called map on buffer before create");
    if (_size == VK_WHOLE_SIZE)
    {
        return vkMapMemory(m_device.GetDevice(), m_memory, 0, m_bufferSize, 0, &m_mapped);
    }
    return vkMapMemory(m_device.GetDevice(), m_memory, _offset, _size, 0, &m_mapped);
}


void Buffer::Unmap()
{
    if (m_mapped) 
    {
        vkUnmapMemory(m_device.GetDevice(), m_memory);
        m_mapped = nullptr;
    }
}


void Buffer::WriteToBuffer(void* _data, VkDeviceSize _size, VkDeviceSize _offset)
{
    assert(m_mapped && "Cannot copy to unmapped buffer");

    if (_size == VK_WHOLE_SIZE) 
    {
        memcpy(m_mapped, _data, m_bufferSize);
    }
    else 
    {
        char* memOffset = (char*)m_mapped;
        memOffset += _offset;
        memcpy(memOffset, _data, _size);
    }
}


VkResult Buffer::Flush(VkDeviceSize _size, VkDeviceSize _offset)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_device.GetPhysicalDevice(), &properties);
    VkDeviceSize atomSize = properties.limits.nonCoherentAtomSize;

    if (_size == VK_WHOLE_SIZE || _offset + _size > m_allocationSize) 
    {
        _size = m_allocationSize - _offset;
    }
    else 
    {
        if ((_size % atomSize) != 0)
        {
            if ((_offset + _size) != m_allocationSize) 
            {
                VkDeviceSize alignedSize = ((_size + atomSize - 1) / atomSize) * atomSize;
                if (_offset + alignedSize > m_allocationSize) {
                    alignedSize = m_allocationSize - _offset;
                }
                _size = alignedSize;
            }
        }
    }
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_memory;
    mappedRange.offset = _offset;
    mappedRange.size = _size;
    return vkFlushMappedMemoryRanges(m_device.GetDevice(), 1, &mappedRange);
}


VkResult Buffer::Invalidate(VkDeviceSize _size, VkDeviceSize _offset) 
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_memory;
    mappedRange.offset = _offset;
    mappedRange.size = _size;
    return vkInvalidateMappedMemoryRanges(m_device.GetDevice(), 1, &mappedRange);
}


VkDescriptorBufferInfo Buffer::DescriptorInfo(VkDeviceSize _size, VkDeviceSize _offset)
{
    return VkDescriptorBufferInfo{ m_buffer, _offset, _size,};
}


void Buffer::WriteToIndex(void* _data, int _index) 
{
    WriteToBuffer(_data, m_instanceSize, _index * m_alignmentSize);
}


VkResult Buffer::FlushIndex(int _index) 
{ 
    return Flush(m_alignmentSize, _index * m_alignmentSize); 
}


VkDescriptorBufferInfo Buffer::DescriptorInfoForIndex(int _index)
{
    return DescriptorInfo(m_alignmentSize, _index * m_alignmentSize);
}


VkResult Buffer::InvalidateIndex(int _index) 
{
    return Invalidate(m_alignmentSize, _index * m_alignmentSize);
}