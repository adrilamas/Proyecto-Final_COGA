#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Constants.h"

// ── Punto de luz ──────────────────────────────────────────────────────────────
struct PointLight {
    glm::vec3 pos;
    glm::vec3 color;
    float     intensity    = 1.0f;
    float     Kc           = 1.0f;    // atenuación constante
    float     Kl           = 0.07f;   // atenuación lineal
    float     Kq           = 0.017f;  // atenuación cuadrática
    float     flickerTimer    = 0.0f;
    float     flickerDuration = 0.0f;
    bool      isFlickering    = false;
};

// ── Banco de luces global ─────────────────────────────────────────────────────
extern PointLight gLights[MAX_LIGHTS];
extern int        gNumLights;

// Rellena el banco de luces con las posiciones e inicializa timers de parpadeo.
void resetLights(const std::vector<glm::vec3>& positions, glm::vec3 color);

// Envía todas las luces activas al shader indicado.
void uploadLights(unsigned int prog);
