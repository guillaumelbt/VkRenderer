#include "core/Pipeline.h"
#include "core/Device.h"
#include "model/Model.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>

Pipeline::Pipeline(Device& _device, const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo) : m_device{ _device }
{ 
	CreateGraphicsPipeline(_vertFilePath, _fragFilePath, _configInfo);
}

Pipeline::~Pipeline()
{
	vkDestroyShaderModule(m_device.GetDevice(), m_fragShaderModule,nullptr);
	vkDestroyShaderModule(m_device.GetDevice(), m_vertShaderModule,nullptr);
	vkDestroyPipeline(m_device.GetDevice(), m_graphicsPipeline, nullptr);
}

std::vector<char> Pipeline::ReadFile(const std::string& _filePath)
{
	std::ifstream file(_filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file" + _filePath);
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;

}

void Pipeline::CreateGraphicsPipeline(const std::string& _vertFilePath, const std::string& _fragFilePath, const PipelineConfigInfo& _configInfo)
{
	auto vertCode = ReadFile(_vertFilePath);
	auto fragCode = ReadFile(_fragFilePath);

	CreateShaderModule(vertCode, &m_vertShaderModule);
	CreateShaderModule(fragCode, &m_fragShaderModule);

	VkPipelineShaderStageCreateInfo shaderStages[2];
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = m_vertShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[0].flags = 0;
	shaderStages[0].pNext = nullptr;
	shaderStages[0].pSpecializationInfo = nullptr;

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = m_fragShaderModule;
	shaderStages[1].pName = "main";
	shaderStages[1].flags = 0;
	shaderStages[1].pNext = nullptr;
	shaderStages[1].pSpecializationInfo = nullptr;


	auto& bindingDescs = _configInfo.bindingDescriptions;
	auto& attributeDescs = _configInfo.attributeDescriptions;


	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();

	VkPipelineViewportStateCreateInfo viewportInfo{};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &_configInfo.viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &_configInfo.scissor;
	viewportInfo.pNext = nullptr;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_configInfo.inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &_configInfo.rasterizationInfo;
	pipelineInfo.pMultisampleState = &_configInfo.multisampleInfo;
	pipelineInfo.pDepthStencilState = &_configInfo.depthStencilInfo;
	pipelineInfo.pColorBlendState = &_configInfo.colorBlendInfo;
	pipelineInfo.pDynamicState = &_configInfo.dynamicStateInfo;

	pipelineInfo.layout = _configInfo.pipelineLayout;
	pipelineInfo.renderPass = _configInfo.renderPass;
	pipelineInfo.subpass = _configInfo.subpass;

	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics pipelines");

}

void Pipeline::CreateShaderModule(const std::vector<char>& _code, VkShaderModule* _shaderModule)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = _code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());

	if (vkCreateShaderModule(m_device.GetDevice(), &createInfo, nullptr, _shaderModule) != VK_SUCCESS)
		throw std::runtime_error("failed to create shader module");
}


void  Pipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& _configInfo)
{
	_configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	_configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	_configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	_configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	_configInfo.viewportInfo.viewportCount = 1;
	_configInfo.viewportInfo.pViewports = nullptr;
	_configInfo.viewportInfo.scissorCount = 1;
	_configInfo.viewportInfo.pScissors = nullptr;

	_configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	_configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
	_configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	_configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	_configInfo.rasterizationInfo.lineWidth = 1.0f;
	_configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	_configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	_configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
	_configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; 
	_configInfo.rasterizationInfo.depthBiasClamp = 0.0f;         
	_configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;   

	_configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	_configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	_configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	_configInfo.multisampleInfo.minSampleShading = 1.0f;           
	_configInfo.multisampleInfo.pSampleMask = nullptr;             
	_configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	_configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;      

	_configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
	_configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   
	_configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  
	_configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             
	_configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  
	_configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  
	_configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              

	_configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	_configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	_configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; 
	_configInfo.colorBlendInfo.attachmentCount = 1;
	_configInfo.colorBlendInfo.pAttachments = &_configInfo.colorBlendAttachment;
	_configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  
	_configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  
	_configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  
	_configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  

	_configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	_configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
	_configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
	_configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	_configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	_configInfo.depthStencilInfo.minDepthBounds = 0.0f;  
	_configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  
	_configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
	_configInfo.depthStencilInfo.front = {};  
	_configInfo.depthStencilInfo.back = {};   

	_configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	_configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	_configInfo.dynamicStateInfo.pDynamicStates = _configInfo.dynamicStateEnables.data();
	_configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(_configInfo.dynamicStateEnables.size());
	_configInfo.dynamicStateInfo.flags = 0;

	_configInfo.bindingDescriptions = Model::Vertex::GetBindingDescriptions();
	_configInfo.attributeDescriptions = Model::Vertex::GetAttributeDescriptions();
}

void Pipeline::EnableAlphaBlending(PipelineConfigInfo& _configInfo) 
{
	_configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
	_configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	_configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	_configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	_configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	_configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	_configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void Pipeline::EnableFireParticleBlending(PipelineConfigInfo& _configInfo) 
{
	_configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
	_configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	_configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	_configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	_configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	_configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	_configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	_configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void Pipeline::Bind(VkCommandBuffer _commandBuffer)
{
	vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
}
