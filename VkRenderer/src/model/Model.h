#pragma once
#include "core/Device.h"
#include "core/Texture.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <memory>
#include "core/Buffer.h"
#include <vulkan/vulkan.h>
#include "core/Descriptors.h"

class Model
{
public:
	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

		bool operator==(const Vertex& _other) const { return position == _other.position && color == _other.color && normal == _other.normal &&uv == _other.uv;
		}
	};

public:
	struct Builder 
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		void LoadModel(const std::string& _filePath);
	};

	Model(Device& _device, const Model::Builder& _builder);
	~Model();

	Model(const Model&) = delete;
	Model operator =(const Model&) = delete;

	void Bind(VkCommandBuffer _commandBuffer);
	void Draw(VkCommandBuffer _commandBuffer);

	static std::unique_ptr<Model> CreateModelFromFile(Device& _device, const std::string& _filePath);

	void SetTexture(std::shared_ptr<Texture> _texture) { m_texture = _texture; }
	std::shared_ptr<Texture> GetTexture() const { return m_texture; }

	void SetTextureDescriptorSet(VkDescriptorSet set) { m_textureDescriptorSet = set; }
	VkDescriptorSet GetTextureDescriptorSet() const { return m_textureDescriptorSet; }

	static std::shared_ptr<Model> CreateModelWithTexture(Device& _device,const std::string& _modelPath,const std::string& _texturePath,DescriptorSetLayout& _textureSetLayout,DescriptorPool& _texturePool);

private:
	void CreateVertexBuffers(const std::vector<Vertex>& _vertices);
	void CreateIndexBuffers(const std::vector<uint32_t>& _indices);

	Device& m_device;
	std::unique_ptr<Buffer> m_vertexBuffer;
	uint32_t m_vertexCount;

	bool m_hasIndexBuffer = false;
	std::unique_ptr<Buffer> m_indexBuffer;
	uint32_t m_indexCount;
	std::shared_ptr<Texture> m_texture = nullptr;
	VkDescriptorSet m_textureDescriptorSet = VK_NULL_HANDLE;
};

