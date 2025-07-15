#pragma once

#include "model/Model.h"
#include <memory>
#include <unordered_map>

// struct TransformComponent
// {
// 	glm::vec3 translation{};
// 	glm::vec3 scale{ 1.f, 1.f, 1.f };
// 	glm::vec3 rotation{};
//
// 	glm::mat4 Mat4();
//
// 	glm::mat3 NormalMatrix();
// };
//
// struct PointLightComponent 
// {
// 	float lightIntensity = 1.0f;
// };

class GameObject
{
public:
	using id_t = unsigned int;
	using Map = std::unordered_map<id_t, GameObject>;

	static GameObject CreateGameObject() 
	{
		static id_t currentId = 0;
		return GameObject{ currentId++ };
	}

	static GameObject MakePointLight(float _intensity = 10.f, float _radius = 0.1f, glm::vec3 _color = glm::vec3(1.f));

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	id_t GetId() { return m_id; }

	std::shared_ptr<Model> model{};
	// std::unique_ptr<PointLightComponent> pointLight = nullptr;
	glm::vec3 color{};
	// TransformComponent transform{};

private:
	GameObject(id_t _objId) : m_id{ _objId } {}

	id_t m_id;
};