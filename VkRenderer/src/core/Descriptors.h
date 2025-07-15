#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "Device.h"

class DescriptorSetLayout
{
public:
    class Builder 
    {
    public:
        Builder(Device& _device) : m_device{ _device } {}

        Builder& AddBinding(uint32_t _binding, VkDescriptorType _descriptorType, VkShaderStageFlags _stageFlags, uint32_t _count = 1);
        std::unique_ptr<DescriptorSetLayout> Build() const;

    private:
        Device& m_device;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
    };

    DescriptorSetLayout(Device& _device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _bindings);
    ~DescriptorSetLayout();
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    Device& m_device;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

    friend class DescriptorWriter;
};

class DescriptorPool 
{
public:
    class Builder
    {
    public:
        Builder(Device& _device) : m_device{ _device } {}

        Builder& AddPoolSize(VkDescriptorType _descriptorType, uint32_t _count);
        Builder& SetPoolFlags(VkDescriptorPoolCreateFlags _flags);
        Builder& SetMaxSets(uint32_t _count);
        std::unique_ptr<DescriptorPool> Build() const;

    private:
        Device& m_device;
        std::vector<VkDescriptorPoolSize> m_poolSizes{};
        uint32_t m_maxSets = 1000;
        VkDescriptorPoolCreateFlags m_poolFlags = 0;
    };

    DescriptorPool(Device& _device, uint32_t _maxSets, VkDescriptorPoolCreateFlags _poolFlags, const std::vector<VkDescriptorPoolSize>& _poolSizes);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    bool AllocateDescriptor(const VkDescriptorSetLayout _descriptorSetLayout, VkDescriptorSet& _descriptor) const;

    void FreeDescriptors(std::vector<VkDescriptorSet>& _descriptors) const;

    void ResetPool();

    VkDescriptorPool GetVkDescriptorPool() const { return m_descriptorPool; }

private:
    Device& m_device;
    VkDescriptorPool m_descriptorPool;

    friend class DescriptorWriter;
};

class DescriptorWriter 
{
public:
    DescriptorWriter(DescriptorSetLayout& _setLayout, DescriptorPool& _pool);

    DescriptorWriter& WriteBuffer(uint32_t _binding, VkDescriptorBufferInfo* _bufferInfo);
    DescriptorWriter& WriteImage(uint32_t _binding, VkDescriptorImageInfo* _imageInfo);

    bool Build(VkDescriptorSet& _set);
    void Overwrite(VkDescriptorSet& _set);

private:
    DescriptorSetLayout& m_setLayout;
    DescriptorPool& m_pool;
    std::vector<VkWriteDescriptorSet> m_writes;
};

