#pragma once
#include "camera/Camera.h"
#include "model/GameObject.h"
#include "core/Device.h"
#include "core/FrameInfo.h"
#include "core/Pipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>


class PointLightSystem
{
public:
	PointLightSystem(Device& _device, VkRenderPass _renderPass, VkDescriptorSetLayout _globalSetLayout, VkSampleCountFlagBits msaaSamples);
	~PointLightSystem();

	PointLightSystem(const PointLightSystem&) = delete;
	PointLightSystem& operator=(const PointLightSystem&) = delete;

	void Update(FrameInfo& _frameInfo, GlobalUbo& _globalUbo);
	void Render(FrameInfo& _frameInfo);

private:
	void CreatePipelineLayout(VkDescriptorSetLayout _globalSetLayout);
	void CreatePipeline(VkRenderPass _renderPass);

	Device& m_device;

	std::unique_ptr<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;
	VkSampleCountFlagBits m_msaaSamples;
};

