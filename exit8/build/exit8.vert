#version 330 core

// ─── Atributos del vértice ────────────────────────────────────────────────────
layout (location = 0) in vec3 aPos;      // Posición en espacio local
layout (location = 1) in vec3 aNormal;   // Normal en espacio local
layout (location = 2) in vec2 aTexCoord; // Coordenadas UV

// ─── Salidas al fragment shader ───────────────────────────────────────────────
out vec3 FragPos;   // Posición en world-space (para cálculos de iluminación)
out vec3 Normal;    // Normal en world-space
out vec2 TexCoord;  // UVs interpoladas

// ─── Uniforms de transformación ───────────────────────────────────────────────
// Separamos model de MVP:
//   - 'uModel'      → solo world-space, lo necesita el fragment shader para luz
//   - 'uTransform'  → MVP completo, lo necesita gl_Position
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // Posición del fragmento en world-space
    FragPos = vec3(uModel * vec4(aPos, 1.0));

    // Normal corregida con la transpuesta de la inversa para soportar
    // escalados no uniformes sin deformar las normales
    Normal = mat3(transpose(inverse(uModel))) * aNormal;

    TexCoord = aTexCoord;

    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
