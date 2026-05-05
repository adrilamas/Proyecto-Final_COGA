#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

// ── Estructura de una malla OpenGL ────────────────────────────────────────────
// Almacena un VAO/VBO con vértices en formato: pos(3) | normal(3) | uv(2)
struct Mesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int          count = 0;
};

// Construye un quad de 6 vértices (2 triángulos) con normales y UVs.
//   o  = esquina de origen en world-space
//   r  = eje "ancho",  rl = su longitud
//   u  = eje "alto",   ul = su longitud
//   n  = normal del quad
//   su, sv = escala UV (por defecto 1)
Mesh buildQuad(glm::vec3 o,
               glm::vec3 r, float rl,
               glm::vec3 u, float ul,
               glm::vec3 n,
               float su = 1.f, float sv = 1.f);
