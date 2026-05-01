#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// ── Salidas para el Fragment Shader (AHORA COINCIDEN) ──
out vec3 vFragPos;
out vec3 vNormal;
out vec2 vUV;

// ── Uniforms desde C++ (AHORA COINCIDEN) ──
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    // Calculamos la posicion en el mundo y la mandamos al Fragment
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    
    // Calculamos la normal adaptada a las transformaciones del modelo
    vNormal = mat3(uModel) * aNormal; 
    
    // Pasamos las coordenadas UV directo al Fragment
    vUV = aTexCoord;

    // Posición final en la pantalla
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}