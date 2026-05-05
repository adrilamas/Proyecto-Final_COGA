#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Mesh.h"

// ── Habitación ────────────────────────────────────────────────────────────────
// Seis quads: suelo, techo y cuatro paredes (N/S/E/O).
// Las paredes N y S tienen un hueco parcial alineado con los corredores;
// por simplicidad las paredes se dibujan sólidas y la "apertura" es geométrica.
struct Room {
    Mesh floor, ceiling;
    Mesh wN;  // Norte (+Z)
    Mesh wS;  // Sur   (-Z)
    Mesh wE;  // Este  (+X)
    Mesh wW;  // Oeste (-X)
};

// ── Corredor ──────────────────────────────────────────────────────────────────
// Tubo rectangular en el eje Z: suelo, techo, pared izq. y pared dcha.
// Los extremos quedan abiertos.
struct Corridor {
    Mesh floor, ceiling;
    Mesh wLeft, wRight;
};

// ── Constructores ─────────────────────────────────────────────────────────────
Room     buildRoom      (glm::vec3 center, float w, float d, float h);
Corridor buildCorridorZ (glm::vec3 center, float len, float w, float h);

// ── Helpers de dibujo ─────────────────────────────────────────────────────────
void drawMesh    (unsigned int prog, int locModel, int locColor,
                  const Mesh& m,     glm::vec3 col);

void drawRoom    (unsigned int prog, int locModel, int locColor,
                  const Room& R,
                  glm::vec3 wallCol, glm::vec3 floorCol, glm::vec3 ceilCol);

void drawCorridor(unsigned int prog, int locModel, int locColor,
                  const Corridor& C,
                  glm::vec3 wallCol, glm::vec3 floorCol, glm::vec3 ceilCol);
