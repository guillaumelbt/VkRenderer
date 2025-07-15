#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Particle 
{
    glm::vec3 startPos;
    float pad1;
    glm::vec3 position;
    float pad2;
    glm::vec3 velocity;
    float pad3;
    glm::vec3 convergenceTarget;
    float life;
    float colorLookup;
    float maxLife;
    glm::vec2 pad4;
};

struct ComputePushConstants 
{
    float deltaTime;
    float curlE;
    float curlMultiplier;
    float particleMinLife;
    float particleMaxLife;
    float emitterRadius;
    float emissionRate;
    float pad1;
    glm::vec3 emitterPos;
    float pad2;
    glm::vec3 emitterScale;
    float pad3;
    glm::vec3 emitterRot;
    float pad4;
    glm::vec3 convergencePoint;
    float convergenceStrength;
    float totalSmokeDistance;
    float updraft;
    float randSeed;
    float _pad5;
};

struct ParameterUBO 
{
    float deltaTime;
    float time;
    float curlE;
    float curlMultiplier;
    float particleMinLife;
    float particleMaxLife;
    glm::vec3 emitterPos;
    glm::vec3 emitterScale;
    glm::vec3 emitterRot;
    glm::vec3 convergencePoint;
    float convergenceStrength;
    float totalSmokeDistance;
    float updraft;
    float randSeed;
    int numVertices;
    glm::vec2 padding;
};

