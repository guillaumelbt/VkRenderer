#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in float inDistance;
layout (location = 3) in float inColorLookup;

layout (location = 0) out vec4 outColor;

void main() {
    // Calcul d'un masque circulaire pour donner une forme ronde aux particules
    vec2 coord = inTexCoord * 2.0 - 1.0; // Coordonnées [-1, 1]
    float dist = length(coord);
    
    // Fondu doux vers les bords (masque circulaire)
    float radialAlpha = 1.0 - smoothstep(0.3, 1.0, dist);
    
    // Alpha variable basé sur colorLookup (comme dans Unity: 1.1 - colorLookup)
    float lifeAlpha = 1.1 - inColorLookup;
    lifeAlpha = clamp(lifeAlpha, 0.0, 1.0);
    
    // Combiner les deux alphas
    float finalAlpha = radialAlpha * lifeAlpha;
    
    // Si complètement transparent, discard le fragment
    if (finalAlpha < 0.01) {
        discard;
    }
    
    // Pré-multiplier l'alpha dans le RGB (comme dans Unity: c.rgb *= c.a)
    vec3 finalColor = inColor * finalAlpha;
    
    outColor = vec4(finalColor, finalAlpha);
}