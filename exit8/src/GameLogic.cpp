#include "GameLogic.h"
#include "Constants.h"
#include "Lighting.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

// Globals externos definidos en main.cpp
extern Camera camera;
extern bool   firstMouse;

// =============================================================================
//  applyAnomalyColors
// =============================================================================
void applyAnomalyColors(AnomalyType a)
{
    G.currentAnomaly = a;
    switch (a) {
    case AnomalyType::NONE:
        G.roomWall  = { 0.82f, 0.80f, 0.75f };
        G.roomFloor = { 0.55f, 0.52f, 0.48f };
        G.roomCeil  = { 0.90f, 0.90f, 0.92f };
        G.lightCol  = { 0.95f, 0.95f, 1.00f };
        G.ambient   =   0.04f;
        break;

    case AnomalyType::RED_LIGHTS:
        G.roomWall  = { 0.40f, 0.10f, 0.10f };
        G.roomFloor = { 0.30f, 0.07f, 0.07f };
        G.roomCeil  = { 0.35f, 0.08f, 0.08f };
        G.lightCol  = { 1.00f, 0.06f, 0.04f };
        G.ambient   =   0.02f;
        break;
    }
}

// =============================================================================
//  activateLights
// =============================================================================
void activateLights(const RoomVariant& V)
{
    const glm::vec3 corColor = { 0.95f, 0.95f, 1.0f };
    gNumLights = 0;

    auto addLight = [&](glm::vec3 pos, glm::vec3 col) {
        if (gNumLights >= MAX_LIGHTS) return;
        PointLight& L = gLights[gNumLights++];
        L.pos       = pos;
        L.color     = col;
        L.intensity = 1.0f;
        L.Kc = 1.0f; L.Kl = 0.09f; L.Kq = 0.032f;
    };

    // Habitación
    for (auto& p : roomLightPositions(V.origin))
        addLight(p, G.lightCol);

    // Pasillos rectos
    for (auto& p : corridorLightPositions(V.corACenter, CL)) {
        glm::vec3 lp = p; lp.y = RH - 0.3f;
        addLight(lp, corColor);
    }
    for (auto& p : corridorLightPositions(V.corBCenter, CL)) {
        glm::vec3 lp = p; lp.y = RH - 0.3f;
        addLight(lp, corColor);
    }

    // Focos sobre las esquinas de los codos
    addLight(V.origin + glm::vec3( 1.0f, RH - 0.3f, -14.5f), corColor);
    addLight(V.origin + glm::vec3(-1.0f, RH - 0.3f,  14.5f), corColor);
}

// =============================================================================
//  spawnAtCorridor
// =============================================================================
void spawnAtCorridor(const RoomVariant& V, bool corA)
{
    if (corA) {
        // Zona segura en X=+3.5, mira hacia –X (pasillo recto A)
        camera.Position = V.origin + glm::vec3( 3.5f, EYE, -14.5f);
        camera.Yaw      = 180.0f;
    } else {
        // Zona segura en X=-3.5, mira hacia +X (pasillo recto B)
        camera.Position = V.origin + glm::vec3(-3.5f, EYE,  14.5f);
        camera.Yaw      = 0.0f;
    }
    camera.Pitch = 0.0f;
    camera.UpdateCameraVectors();
    firstMouse = true;
}

// =============================================================================
//  triggerTeleport
// =============================================================================
void triggerTeleport(int toVariant, bool toCorA, bool isCorrect)
{
    if (G.state != GameState::PLAYING) return;
    G.state             = GameState::FADING_OUT;
    G.fadeAlpha         = 0.0f;
    G.pending.active    = true;
    G.pending.toVariant = toVariant;
    G.pending.toCorA    = toCorA;
    G.pending.isCorrect = isCorrect;
}

// =============================================================================
//  nextVariantIdx
// =============================================================================
int nextVariantIdx(int current)
{
    if (NUM_VARIANTS <= 1) return 0;
    int next = rand() % NUM_VARIANTS;
    if (next == current) next = (current + 1) % NUM_VARIANTS;
    return next;
}

// =============================================================================
//  applyCollisions
// =============================================================================
void applyCollisions(glm::vec3& pos, const RoomVariant& V)
{
    constexpr float R = 0.2f; // radio del jugador

    float lx = pos.x - V.origin.x;
    float lz = pos.z - V.origin.z;

    // Techo absoluto de los fondos de los codos
    lz = glm::clamp(lz, -16.0f + R, 16.0f - R);

    if (lz < 0.0f) {
        // ─── Lado Sur (–Z): habitación → corredor A → codo A ──────────────────
        lx = glm::max(lx, -4.0f + R); // pared oeste continua

        // Esquina cóncava a la derecha del pasillo A
        if (lx > -1.0f - R && lz < -4.0f + R && lz > -13.0f - R) {
            float dX = lx - (-1.0f - R);
            if (lz > -8.5f) {
                float dZ = (-4.0f + R) - lz;
                if (dX < dZ) lx = -1.0f - R; else lz = -4.0f + R;
            } else {
                float dZ = lz - (-13.0f - R);
                if (dX < dZ) lx = -1.0f - R; else lz = -13.0f - R;
            }
        }

        if      (lz < -13.0f) lx = glm::min(lx,  6.0f - R); // codo A
        else if (lz <  -4.0f) lx = glm::min(lx, -1.0f - R); // pasillo recto A
        else                  lx = glm::min(lx,  4.0f - R); // habitación (sur)

    } else {
        // ─── Lado Norte (+Z): habitación → corredor B → codo B ────────────────
        lx = glm::min(lx, 4.0f - R); // pared este continua

        // Esquina cóncava a la izquierda del pasillo B
        if (lx < 1.0f + R && lz > 4.0f - R && lz < 13.0f + R) {
            float dX = (1.0f + R) - lx;
            if (lz < 8.5f) {
                float dZ = lz - (4.0f - R);
                if (dX < dZ) lx = 1.0f + R; else lz = 4.0f - R;
            } else {
                float dZ = (13.0f + R) - lz;
                if (dX < dZ) lx = 1.0f + R; else lz = 13.0f + R;
            }
        }

        if      (lz > 13.0f) lx = glm::max(lx, -6.0f + R); // codo B
        else if (lz >  4.0f) lx = glm::max(lx,  1.0f + R); // pasillo recto B
        else                 lx = glm::max(lx, -4.0f + R); // habitación (norte)
    }

    pos.x = V.origin.x + lx;
    pos.z = V.origin.z + lz;
}
