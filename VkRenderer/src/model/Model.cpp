#include "model/Model.h"
#include <cassert>
#include <iostream>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "core/Utils.h"
#include <unordered_map>
#include "core/Buffer.h"
#include "core/Device.h"
#include "core/Descriptors.h"

namespace std 
{
	template <>
	struct hash<Model::Vertex> {
		size_t operator()(Model::Vertex const& _vertex) const 
		{
			size_t seed = 0;
			HashCombine(seed, _vertex.position, _vertex.color, _vertex.normal, _vertex.uv);
			return seed;
		}
	};
}

Model::Model(Device& _device, const Model::Builder& _builder) : m_device{ _device }
{
	CreateVertexBuffers(_builder.vertices);
	CreateIndexBuffers(_builder.indices);
}

Model::~Model()
{
	
}


std::unique_ptr<Model> Model::CreateModelFromFile(Device& _device, const std::string& _filepath) 
{
	Builder builder{};
	builder.LoadModel(_filepath);
	return std::make_unique<Model>(_device, builder);
}

std::shared_ptr<Model> Model::CreateModelWithTexture(
    Device& device,
    const std::string& modelPath,
    const std::string& texturePath,
    DescriptorSetLayout& textureSetLayout,
    DescriptorPool& texturePool
) {
    auto uniqueModel = Model::CreateModelFromFile(device, modelPath);
    auto model = std::shared_ptr<Model>(std::move(uniqueModel));
    auto texture = std::make_shared<Texture>();
    texture->LoadFromFile(texturePath, device, device.GetGraphicsQueue());
    model->SetTexture(texture);

    VkDescriptorSet textureDescriptorSet;
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->GetImageView();
    imageInfo.sampler = texture->GetSampler();
    DescriptorWriter(textureSetLayout, texturePool)
        .WriteImage(0, &imageInfo)
        .Build(textureDescriptorSet);
    model->SetTextureDescriptorSet(textureDescriptorSet);
    return model;
}

void Model::CreateVertexBuffers(const std::vector<Vertex>& _vertices)
{
	m_vertexCount = static_cast<uint32_t>(_vertices.size());

	assert(m_vertexCount >= 3 && "Vertex count must equal or grater than 3");

	VkDeviceSize bufferSize = sizeof(_vertices[0]) * m_vertexCount;
	uint32_t vertexSize = sizeof(_vertices[0]);

	Buffer stagingBuffer{m_device, vertexSize, m_vertexCount,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void*)_vertices.data());

	m_vertexBuffer = std::make_unique<Buffer>(m_device, vertexSize, m_vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_device.CopyBuffer(stagingBuffer.GetBuffer(), m_vertexBuffer->GetBuffer(), bufferSize);

}

void Model::CreateIndexBuffers(const std::vector<uint32_t>& _indices)
{
	m_indexCount = static_cast<uint32_t>(_indices.size());
	m_hasIndexBuffer = m_indexCount > 0;

	if (!m_hasIndexBuffer) 
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(_indices[0]) * m_indexCount;
	uint32_t indexSize = sizeof(_indices[0]);

	Buffer stagingBuffer{ m_device, indexSize, m_indexCount,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void*)_indices.data());

	m_indexBuffer = std::make_unique<Buffer>(m_device, indexSize, m_indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_device.CopyBuffer(stagingBuffer.GetBuffer(), m_indexBuffer->GetBuffer(), bufferSize);
}

void Model::Bind(VkCommandBuffer _commandBuffer)
{
	assert(m_vertexBuffer != nullptr && "Vertex buffer is null");
	
	VkBuffer buffers[] = { m_vertexBuffer->GetBuffer()};
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(_commandBuffer, 0, 1, buffers, offsets);

	if (m_hasIndexBuffer) 
	{
		assert(m_indexBuffer != nullptr && "Index buffer is null but hasIndexBuffer is true");
		vkCmdBindIndexBuffer(_commandBuffer, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Model::Draw(VkCommandBuffer _commandBuffer)
{
	assert(m_vertexBuffer != nullptr && "Vertex buffer is null");
	
	if (m_hasIndexBuffer)
	{
		assert(m_indexBuffer != nullptr && "Index buffer is null but hasIndexBuffer is true");
		vkCmdDrawIndexed(_commandBuffer, m_indexCount, 1, 0, 0, 0);
	}
	else 
	{
		vkCmdDraw(_commandBuffer, m_vertexCount, 1, 0, 0);
	}
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::GetBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriiption{ 1 };
	bindingDescriiption[0].binding = 0;
	bindingDescriiption[0].stride = sizeof(Vertex);
	bindingDescriiption[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescriiption;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::GetAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
	attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
	attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
	attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT,	offsetof(Vertex, uv) });

	return attributeDescriptions;
}

void Model::Builder::LoadModel(const std::string& _filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, _filepath.c_str())) 
		throw std::runtime_error(warn + err);

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) 
	{
		for (const auto& index : shape.mesh.indices) 
		{
		
			Vertex vertex{};

			if (index.vertex_index >= 0) 
			{
				vertex.position =
				{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};

				if (attrib.colors.size() > 0) 
				{
					vertex.color = 
					{
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}
				else 
				{
					vertex.color = { 1.0f, 1.0f, 1.0f };
				}
			}
			else 
			{
				vertex.color = { 1.0f, 1.0f, 1.0f };
			}

			if (index.normal_index >= 0) 
			{
				vertex.normal = 
				{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}
			else 
			{
				vertex.normal = { 0.0f, 0.0f, 1.0f };
			}

			if (index.texcoord_index >= 0) 
			{
				vertex.uv = 
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}
			else 
			{
				vertex.uv = { 0.0f, 0.0f };
			}

			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}