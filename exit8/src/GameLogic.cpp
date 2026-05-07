#include "GameLogic.h"
#include "Constants.h"
#include "Lighting.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <iostream>


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

    auto addLight = [&](glm::vec3 pos, glm::vec3 col,
                        float Kc = 1.0f, float Kl = 0.09f, float Kq = 0.032f) {
        if (gNumLights >= MAX_LIGHTS) return;
        PointLight& L = gLights[gNumLights++];
        L.pos       = pos;
        L.color     = col;
        L.intensity = 1.0f;
        L.Kc = Kc; L.Kl = Kl; L.Kq = Kq;
        L.isFlickering = false;
        L.flickerTimer = 1.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.5f;
    };

    // ── Habitación ────────────────────────────────────────────────────────────
    for (auto& p : roomLightPositions(V.origin))
        addLight(p, G.lightCol);

    // ── Pasillos rectos ───────────────────────────────────────────────────────
    for (auto& p : corridorLightPositions(V.corACenter, CL)) {
        glm::vec3 lp = p; lp.y = RH - 0.3f;
        addLight(lp, corColor, 1.0f, 0.14f, 0.07f);
    }
    for (auto& p : corridorLightPositions(V.corBCenter, CL)) {
        glm::vec3 lp = p; lp.y = RH - 0.3f;
        addLight(lp, corColor, 1.0f, 0.14f, 0.07f);
    }

    // ── Focos sobre las esquinas de los codos ─────────────────────────────────
    addLight(V.origin + glm::vec3( 1.0f, RH - 0.3f, -14.5f), corColor, 1.0f, 0.14f, 0.07f);
    addLight(V.origin + glm::vec3(-1.0f, RH - 0.3f,  14.5f), corColor, 1.0f, 0.14f, 0.07f);
}

// =============================================================================
//  spawnAtCorridor
// =============================================================================
// Necesitas añadir #include "GameLogic.h" o donde tengas activateLights y applyAnomalyColors
void spawnAtCorridor(const RoomVariant& newV, const RoomVariant& oldV, bool toCorA, glm::vec3 currentCamPos, float main_triggerDist)
{
    float hw = RW / 2.f;
    float CW_half = CW / 2.0f;

    // Calculamos los centros exactos basados en la matemática de tu mapa:
    float local_corCenterA_x = -hw + CW_half; // Pared Izquierda + Medio Ancho Pasillo
    float local_corCenterB_x = hw - CW_half; // Pared Derecha - Medio Ancho Pasillo

    float prevCenter = toCorA ? local_corCenterB_x : local_corCenterA_x;
    float newCenter = toCorA ? local_corCenterA_x : local_corCenterB_x;

    // ── 1. CÁLCULO DE X (Seamless Absoluta) ──
    float localX_Old = currentCamPos.x - oldV.origin.x;
    float offsetX = localX_Old - prevCenter;

    // Mantenemos la suma (+) para teletransporte seamless (mismo lado de la pared)
    float targetX = newCenter + offsetX;

    // ── 2. CÁLCULO DE Z (Trigger + Overshoot + Margen de 0.02) ──
    float overshootZ = 0.0f;
    float margin = 0.02f; // Margen de seguridad para no quedar atrapado en el trigger

    if (toCorA) {
        // Venimos de B (moviéndonos hacia adelante, +Z)
        overshootZ = currentCamPos.z - (oldV.origin.z + main_triggerDist);

        // Aparecemos en el trigger de A (-main_triggerDist)
        // Sumamos el overshoot y el margen en dirección +Z (hacia el interior del nivel)
        camera.Position = newV.origin + glm::vec3(targetX, EYE, -main_triggerDist + overshootZ + margin);
    }
    else {
        // Venimos de A (moviéndonos hacia adelante, -Z)
        overshootZ = currentCamPos.z - (oldV.origin.z - main_triggerDist);

        // Aparecemos en el trigger de B (+main_triggerDist)
        // Sumamos el overshoot y restamos el margen en dirección -Z (hacia el interior del nivel)
        camera.Position = newV.origin + glm::vec3(targetX, EYE, main_triggerDist + overshootZ - margin);
    }

    camera.UpdateCameraVectors();
}

void spawnAtCorridor(const RoomVariant& V, bool corA)
{
    float hw      = RW / 2.f;
    float cor_len = CL - CW;
    float zDistNear     = (RD / 2.f) + cor_len;
    float triggerMargin = 3.5f;
    // Spawneamos 2 unidades más cerca de la habitación que el trigger
    // para que el jugador no se teletransporte nada más aparecer.
    float spawnOffsetZ  = zDistNear - (triggerMargin + 2.0f);

    float xCenter_A = -hw + (CW / 2.f);   // centro X del pasillo A (izquierda)
    float xCenter_B =  hw - (CW / 2.f);   // centro X del pasillo B (derecha)

    if (corA) {
        // Pasillo A (Sur, Z negativo) → el jugador camina hacia +Z
        camera.Position = V.origin + glm::vec3(xCenter_A, EYE, -spawnOffsetZ);
        camera.Yaw      =  90.0f;   // mirando hacia +Z
    } else {
        // Pasillo B (Norte, Z positivo) → el jugador camina hacia -Z
        camera.Position = V.origin + glm::vec3(xCenter_B, EYE, spawnOffsetZ);
        camera.Yaw      = -90.0f;   // mirando hacia -Z
    }

    camera.Pitch = 0.0f;
    camera.UpdateCameraVectors();
    firstMouse = true;
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
    constexpr float R = 0.2f;

    float lx = pos.x - V.origin.x;
    float lz = pos.z - V.origin.z;

    // --- Geometría dinámica ---
    float hw        = RW / 2.0f;             // Mitad del ancho de la sala
    float hd        = RD / 2.0f;             // Mitad de la profundidad de la sala
    float cor_len   = CL - CW;               // Longitud del pasillo recto
    float zDistNear = hd + cor_len;          // Inicio del codo (antiguo 13.0f)
    float zDistFar  = zDistNear + CW;        // Fondo del codo (antiguo 16.0f)

    // Coordenadas X de las paredes de los pasillos
    float xLeftWall_A  = -hw;                // Antiguo -4.0f
    float xRightWall_A = -hw + CW;           // Antiguo -1.0f
    float xRightWall_B = hw;                 // Antiguo 4.0f
    float xLeftWall_B  = hw - CW;            // Antiguo 1.0f

    // Punto medio Z para las colisiones de esquinas cóncavas
    float midZ = (hd + zDistNear) / 2.0f;    // Antiguo 8.5f

    // Techo absoluto de los fondos de los codos

    lz = glm::clamp(lz, -zDistFar + R, zDistFar - R);

    if (lz < 0.0f) {
        // ── Lado Sur (−Z): habitación → pasillo A → codo A ───────────────────
        lx = glm::max(lx, xLeftWall_A + R);   // pared oeste continua

        // Esquina cóncava a la derecha del pasillo A
        if (lx > xRightWall_A - R && lz < -hd + R && lz > -zDistNear - R) {
            float dX = lx - (xRightWall_A - R);
            if (lz > -midZ) {
                float dZ = (-hd + R) - lz;
                if (dX < dZ) lx = xRightWall_A - R; else lz = -hd + R;
            } else {
                float dZ = lz - (-zDistNear - R);
                if (dX < dZ) lx = xRightWall_A - R; else lz = -zDistNear - R;
            }
        }

        if      (lz < -zDistNear) lx = glm::min(lx, xRightWall_A - R);
        else if (lz <  -hd)       lx = glm::min(lx, xRightWall_A - R);
        else                      lx = glm::min(lx, hw - R);

    } else {
        // ── Lado Norte (+Z): habitación → pasillo B → codo B ─────────────────
        lx = glm::min(lx, xRightWall_B - R);  // pared este continua

        // Esquina cóncava a la izquierda del pasillo B
        if (lx < xLeftWall_B + R && lz > hd - R && lz < zDistNear + R) {
            float dX = (xLeftWall_B + R) - lx;
            if (lz < midZ) {
                float dZ = lz - (hd - R);
                if (dX < dZ) lx = xLeftWall_B + R; else lz = hd - R;
            } else {
                float dZ = (zDistNear + R) - lz;
                if (dX < dZ) lx = xLeftWall_B + R; else lz = zDistNear + R;
            }
        }

        if      (lz > zDistNear) lx = glm::max(lx, xLeftWall_B + R);
        else if (lz >  hd)       lx = glm::max(lx, xLeftWall_B + R);
        else                     lx = glm::max(lx, -hw + R);
    }

    pos.x = V.origin.x + lx;
    pos.z = V.origin.z + lz;
}
