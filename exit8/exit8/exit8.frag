#version 330 core

// ── Entradas del vertex shader ────────────────────────────────────────────────
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;

// ── Salida: color final del pixel ─────────────────────────────────────────────
out vec4 FragColor;

// ── Uniforms ──────────────────────────────────────────────────────────────────
uniform vec3 uColor; // color base del objeto (mandado desde C++)

void main()
{
    // ── Iluminacion simple tipo Phong ─────────────────────────────────────────
    // Luz ambiental: simula luz indirecta del entorno (pasillo fluorescente)
    float ambientStrength = 0.4;
    vec3  lightColor      = vec3(1.0, 0.97, 0.90); // blanco ligeramente calido
    vec3  lightDir        = normalize(vec3(0.2, 1.0, 0.3)); // direccion de luz

    vec3 ambient = ambientStrength * lightColor;

    // Luz difusa: depende del angulo entre la normal y la direccion de luz
    vec3  norm    = normalize(vNormal);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  diffuse = diff * lightColor;

    // ── Patron de cuadricula en el suelo (sin textura) ────────────────────────
    // Genera un tablero de ajedrez usando las UV
    float gridScale = 1.0; // tamanio de cada cuadrado en unidades UV
    vec2  grid      = floor(vUV / gridScale);
    float checker   = mod(grid.x + grid.y, 2.0); // 0.0 o 1.0 alternado

    // Alterna entre dos variantes del color base
    vec3 color = uColor * mix(0.8, 1.15, checker);

    // ── Color final = iluminacion * color del objeto ───────────────────────────
    vec3 result = (ambient + diffuse) * color;

    // Ligero viñeteo (oscurece los bordes, da sensacion de tunel angustiante)
    // -- se puede ampliar mas adelante --

    FragColor = vec4(result, 1.0);
}
