#pragma once
#include <string>
#include <vector>

#include "Device.h"

struct PipelineConfigInfo 
{
	PipelineConfigInfo() = default;
	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};

class Pipeline
{
public:

	Pipeline (Device& _device, const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo);
	~Pipeline();

	Pipeline(const Pipeline&) = delete;
	Pipeline& operator =(const Pipeline&) = delete;

	void Bind(VkCommandBuffer _commandBuffer);

	static void DefaultPipelineConfigInfo(PipelineConfigInfo& _configInfo);
	static void EnableAlphaBlending(PipelineConfigInfo& _configInfo);
	static void EnableFireParticleBlending(PipelineConfigInfo& _configInfo);

private:
	static std::vector<char> ReadFile(const std::string& _filePath);

	void CreateGraphicsPipeline(const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo);
	void CreateShaderModule(const std::vector<char>& _code, VkShaderModule* _shaderModule);

	Device& m_device;
	//std::shared_ptr<Device> m_device;

	VkPipeline m_graphicsPipeline;
	VkShaderModule m_vertShaderModule;
	VkShaderModule m_fragShaderModule;

};

