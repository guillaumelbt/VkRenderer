#pragma once
#include <glm/glm.hpp>

struct PointLightComponent 
{
    float lightIntensity = 1.0f;
    glm::vec3 color{1.0f, 1.0f, 1.0f}; // Default white light
}; 