#include "Lighting.h"
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <cstdlib>

// Definiciones de los globales declarados en Lighting.h
PointLight gLights[MAX_LIGHTS];
int        gNumLights = 0;

// =============================================================================
//  resetLights
// =============================================================================
void resetLights(const std::vector<glm::vec3>& positions, glm::vec3 color)
{
    gNumLights = static_cast<int>(positions.size());
    if (gNumLights > MAX_LIGHTS) gNumLights = MAX_LIGHTS;

    for (int i = 0; i < gNumLights; i++) {
        PointLight& L  = gLights[i];
        L.pos          = positions[i];
        L.color        = color;
        L.intensity    = 1.0f;
        L.isFlickering = false;
        // Timer aleatorio para el primer parpadeo (1.5 – 6 s)
        L.flickerTimer = 1.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.5f;
    }
}

// =============================================================================
//  uploadLights
// =============================================================================
void uploadLights(unsigned int prog)
{
    for (int i = 0; i < gNumLights; i++) {
        const PointLight& L  = gLights[i];
        std::string       b  = "uLights[" + std::to_string(i) + "].";

        glUniform3fv(glGetUniformLocation(prog, (b + "position").c_str()),  1, glm::value_ptr(L.pos));
        glUniform3fv(glGetUniformLocation(prog, (b + "color").c_str()),     1, glm::value_ptr(L.color));
        glUniform1f (glGetUniformLocation(prog, (b + "intensity").c_str()), L.intensity);
        glUniform1f (glGetUniformLocation(prog, (b + "Kc").c_str()),        L.Kc);
        glUniform1f (glGetUniformLocation(prog, (b + "Kl").c_str()),        L.Kl);
        glUniform1f (glGetUniformLocation(prog, (b + "Kq").c_str()),        L.Kq);
    }
    glUniform1i(glGetUniformLocation(prog, "uNumLights"), gNumLights);
}
