#include "camera/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <limits>





void Camera::SetOrthographicProjection(float _left, float _right, float _top, float _bottom, float _near, float _far)
{
    m_projectionMatrix = glm::ortho(_left, _right, _bottom, _top, _near, _far);
}

void Camera::SetPerspectiveProjection(float _fovy, float _aspect, float _near, float _far)
{
    assert(glm::abs(_aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    m_projectionMatrix = glm::perspective(_fovy, _aspect, _near, _far);
    m_projectionMatrix[1][1] *= -1;
}

void Camera::SetViewDirection(glm::vec3 _position, glm::vec3 _direction, glm::vec3 _up)
{
    const glm::vec3 w{ glm::normalize(_direction) };
    const glm::vec3 u{ glm::normalize(glm::cross(w, _up)) };
    const glm::vec3 v{ glm::cross(w, u) };

    m_viewMatrix = glm::mat4{ 1.0f };
    m_viewMatrix[0][0] = u.x;
    m_viewMatrix[1][0] = u.y;
    m_viewMatrix[2][0] = u.z;
    m_viewMatrix[0][1] = v.x;
    m_viewMatrix[1][1] = v.y;
    m_viewMatrix[2][1] = v.z;
    m_viewMatrix[0][2] = w.x;
    m_viewMatrix[1][2] = w.y;
    m_viewMatrix[2][2] = w.z;
    m_viewMatrix[3][0] = -glm::dot(u, _position);
    m_viewMatrix[3][1] = -glm::dot(v, _position);
    m_viewMatrix[3][2] = -glm::dot(w, _position);

    m_inverseViewMatrix = glm::mat4{ 1.0f };
    m_inverseViewMatrix[0][0] = u.x;
    m_inverseViewMatrix[0][1] = u.y;
    m_inverseViewMatrix[0][2] = u.z;
    m_inverseViewMatrix[1][0] = v.x;
    m_inverseViewMatrix[1][1] = v.y;
    m_inverseViewMatrix[1][2] = v.z;
    m_inverseViewMatrix[2][0] = w.x;
    m_inverseViewMatrix[2][1] = w.y;
    m_inverseViewMatrix[2][2] = w.z;
    m_inverseViewMatrix[3][0] = _position.x;
    m_inverseViewMatrix[3][1] = _position.y;
    m_inverseViewMatrix[3][2] = _position.z;
}

void Camera::SetViewTarget(glm::vec3 _position, glm::vec3 _target, glm::vec3 _up)
{
    SetViewDirection(_position, _target - _position, _up);
}

void Camera::SetViewYXZ(glm::vec3 _position, glm::vec3 _rotation) 
{
    const float c3 = glm::cos(_rotation.z);
    const float s3 = glm::sin(_rotation.z);
    const float c2 = glm::cos(_rotation.x);
    const float s2 = glm::sin(_rotation.x);
    const float c1 = glm::cos(_rotation.y);
    const float s1 = glm::sin(_rotation.y);
    const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
    const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
    const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
    m_viewMatrix = glm::mat4{ 1.0f };
    m_viewMatrix[0][0] = u.x;
    m_viewMatrix[1][0] = u.y;
    m_viewMatrix[2][0] = u.z;
    m_viewMatrix[0][1] = v.x;
    m_viewMatrix[1][1] = v.y;
    m_viewMatrix[2][1] = v.z;
    m_viewMatrix[0][2] = w.x;
    m_viewMatrix[1][2] = w.y;
    m_viewMatrix[2][2] = w.z;
    m_viewMatrix[3][0] = -glm::dot(u, _position);
    m_viewMatrix[3][1] = -glm::dot(v, _position);
    m_viewMatrix[3][2] = -glm::dot(w, _position);

    m_inverseViewMatrix = glm::mat4{ 1.0f };
    m_inverseViewMatrix[0][0] = u.x;
    m_inverseViewMatrix[0][1] = u.y;
    m_inverseViewMatrix[0][2] = u.z;
    m_inverseViewMatrix[1][0] = v.x;
    m_inverseViewMatrix[1][1] = v.y;
    m_inverseViewMatrix[1][2] = v.z;
    m_inverseViewMatrix[2][0] = w.x;
    m_inverseViewMatrix[2][1] = w.y;
    m_inverseViewMatrix[2][2] = w.z;
    m_inverseViewMatrix[3][0] = _position.x;
    m_inverseViewMatrix[3][1] = _position.y;
    m_inverseViewMatrix[3][2] = _position.z;
}