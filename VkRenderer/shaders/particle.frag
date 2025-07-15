#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in float inDistance;
layout (location = 3) in float inColorLookup;

layout (location = 0) out vec4 outColor;

void main() {
    vec2 coord = inTexCoord * 2.0 - 1.0;
    float dist = length(coord);
    
    float radialAlpha = 1.0 - smoothstep(0.3, 1.0, dist);
    
    float lifeAlpha = 1.1 - inColorLookup;
    lifeAlpha = clamp(lifeAlpha, 0.0, 1.0);
    
    float finalAlpha = radialAlpha * lifeAlpha;
    
    if (finalAlpha < 0.01) {
        discard;
    }
    
    vec3 finalColor = inColor * finalAlpha;
    
    outColor = vec4(finalColor, finalAlpha);
}