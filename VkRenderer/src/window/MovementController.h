#pragma once

#include "model/GameObject.h"
#include "window/Window.h"
#include "systems/EntityComponentSystem.h"
#include "components/TransformComponent.h"

class MovementController
{

public:
    struct KeyMappings
    {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_E;
        int moveDown = GLFW_KEY_Q;
    };

    // void MoveInPlaneXZ(GLFWwindow* _window, float dt, GameObject& _gameObject);
    void MoveInPlaneXZ(GLFWwindow* _window, float dt, Entity entity, EntityComponentSystem& ecs);

    KeyMappings keys{};
    float moveSpeed{ 3.f };
    float mouseSensitivity{ 0.002f };
    bool firstMouse{ true };
    float lastMouseX{ 0.0f };
    float lastMouseY{ 0.0f };
};

