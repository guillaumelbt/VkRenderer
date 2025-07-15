#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>


class Camera
{
public:
    void SetOrthographicProjection(float _left, float _right, float _top, float _bottom, float _near, float _far);

    void SetPerspectiveProjection(float _fovy, float _aspect, float _near, float _far);

    void SetViewDirection(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up = glm::vec3{ 0.f, -1.0f, 0.f });

    void SetViewTarget(glm::vec3 _position, glm::vec3 _target, glm::vec3 _up = glm::vec3{ 0.f, -1.0f, 0.f });

    void SetViewYXZ(glm::vec3 _position, glm::vec3 _rotation);

    const glm::mat4& GetProjection() const { return m_projectionMatrix; }
    const glm::mat4& GetView() const { return m_viewMatrix; }
    const glm::mat4& GetInverseView() const { return m_inverseViewMatrix; }
    const glm::vec3  GetPosition() const { return glm::vec3(m_inverseViewMatrix[3]); }

private:
    glm::mat4 m_projectionMatrix{ 1.0f };
    glm::mat4 m_viewMatrix{ 1.0f };
    glm::mat4 m_inverseViewMatrix{ 1.f };
};

