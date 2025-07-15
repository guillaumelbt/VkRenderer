#version 450

struct Particle {
    vec3 startPos;
    float _pad1;
    vec3 position;
    float _pad2;
    vec3 velocity;
    float _pad3;
    vec3 convergenceTarget;
    float life;
    float colorLookup;
    float maxLife;
    vec2 _pad4;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor;
} ubo;

layout(std430, set = 0, binding = 1) readonly buffer Particles {
    Particle particles[];
};

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out float outDistance;
layout(location = 3) out float outColorLookup;

void main() {
    Particle p = particles[gl_InstanceIndex];
    
    // Ne pas rendre les particules mortes
    if (p.life <= 0.0) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0); // Position invalide
        return;
    }
    
    // Taille variable basée sur la distance parcourue (comme dans le shader Unity)
    float distance = length(p.position - p.startPos);
    float sizeByLifeMin = 0.01;
    float sizeByLifeMax = 0.035;
    float totalSmokeDistance = 1.5; // Cohérent avec les push constants
    
    // Interpolation de taille basée sur la distance (comme _SizeByLifeMin/_SizeByLifeMax)
    float size = mix(sizeByLifeMin, sizeByLifeMax, clamp(distance / totalSmokeDistance, 0.0, 1.0));
    
    // Calcul des vecteurs de la caméra pour billboard
    vec3 cameraRight = vec3(ubo.view[0][0], ubo.view[1][0], ubo.view[2][0]);
    vec3 cameraUp    = vec3(ubo.view[0][1], ubo.view[1][1], ubo.view[2][1]);
    
    // Position du vertex du quad dans l'espace monde
    vec3 worldPos = p.position + cameraRight * inPosition.x * size + cameraUp * inPosition.y * size;
    
    gl_Position = ubo.projection * ubo.view * vec4(worldPos, 1.0);
    
    // Gradient de couleur réaliste : blanc chaud -> jaune -> orange -> rouge -> noir/gris
    float t = clamp(p.colorLookup, 0.0, 1.0);
    
    // Gradient de feu réaliste de la base chaude vers le sommet froid
    vec3 fireColor;
    if (t < 0.2) {
        // Blanc chaud vers jaune brillant
        fireColor = mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 0.9, 0.1), t / 0.2);
    } else if (t < 0.5) {
        // Jaune vers orange
        fireColor = mix(vec3(1.0, 0.9, 0.1), vec3(1.0, 0.5, 0.0), (t - 0.2) / 0.3);
    } else if (t < 0.8) {
        // Orange vers rouge
        fireColor = mix(vec3(1.0, 0.5, 0.0), vec3(0.8, 0.1, 0.0), (t - 0.5) / 0.3);
    } else {
        // Rouge vers noir/gris foncé (extinction)
        fireColor = mix(vec3(0.8, 0.1, 0.0), vec3(0.1, 0.05, 0.05), (t - 0.8) / 0.2);
    }
    
    outColor = fireColor;
    
    outTexCoord = inPosition * 0.5 + 0.5;
    outDistance = length(p.position - p.startPos);
    outColorLookup = p.colorLookup;
}