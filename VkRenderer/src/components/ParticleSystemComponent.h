#pragma once
#include <glm/glm.hpp>

struct ParticleSystemComponent 
{
    uint32_t maxParticles = 10000;
    float emissionRate = 100.0f;
    float particleLifetime = 2.0f;
    float particleSize = 0.1f;
    glm::vec3 initialVelocity = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 velocityVariation = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 color = glm::vec3(1.0f, 0.3f, 0.0f);
    glm::vec3 colorVariation = glm::vec3(0.2f, 0.2f, 0.2f);
    bool active = true;
}; 