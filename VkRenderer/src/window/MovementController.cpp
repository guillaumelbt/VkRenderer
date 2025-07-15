#include "window/MovementController.h"
#include <limits>
#include <glm/gtc/constants.hpp>
#include <glm/common.hpp>
#include "components/TransformComponent.h"

void MovementController::MoveInPlaneXZ(GLFWwindow* _window, float _dt, Entity _entity, EntityComponentSystem& _ec)
{
    if (!_ec.HasComponent<TransformComponent>(_entity)) return;
    auto& transform = _ec.GetComponent<TransformComponent>(_entity);

    if (glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) 
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        double mouseX, mouseY;
        glfwGetCursorPos(_window, &mouseX, &mouseY);
        if (firstMouse) 
        {
            lastMouseX = static_cast<float>(mouseX);
            lastMouseY = static_cast<float>(mouseY);
            firstMouse = false;
        }
        
        float deltaX = static_cast<float>(mouseX) - lastMouseX;
        float deltaY = static_cast<float>(mouseY) - lastMouseY;
        
        lastMouseX = static_cast<float>(mouseX);
        lastMouseY = static_cast<float>(mouseY);
        
        transform.rotation.y += deltaX * mouseSensitivity;
        transform.rotation.x += deltaY * mouseSensitivity;
    } 
    else 
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }
    
    transform.rotation.x = glm::clamp(transform.rotation.x, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
    transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<float>());

    float yaw = transform.rotation.y;
    const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
    const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
    const glm::vec3 upDir{ 0.f, 1.f, 0.f };

    glm::vec3 moveDir{ 0.f };
    if (glfwGetKey(_window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
    if (glfwGetKey(_window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
    if (glfwGetKey(_window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
    if (glfwGetKey(_window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
    if (glfwGetKey(_window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
    if (glfwGetKey(_window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
    {
        transform.translation += moveSpeed * _dt * glm::normalize(moveDir);
    }
}
