#include "GameLogic.h"
#include "Constants.h"
#include "Lighting.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>
#include <iostream>

extern Camera camera;
extern bool   firstMouse;

// =============================================================================
//  applyAnomalyColors
// =============================================================================
void applyAnomalyColors(AnomalyType a)
{
    G.currentAnomaly = a;
    switch (a) {
    default:
    case AnomalyType::NONE:
        G.roomWall  = { 0.82f, 0.80f, 0.75f };
        G.roomFloor = { 0.55f, 0.52f, 0.48f };
        G.roomCeil  = { 0.90f, 0.90f, 0.92f };
        
        // Cambiamos el 1.00f del final (azul) por 0.85f para igualarlo al pasillo
        G.lightCol  = { 0.95f, 0.92f, 0.85f }; 
        
        G.ambient   = 0.04f;
        break;

    case AnomalyType::RED_LIGHTS:
        G.roomWall  = { 0.40f, 0.08f, 0.08f };
        G.roomFloor = { 0.28f, 0.06f, 0.06f };
        G.roomCeil  = { 0.32f, 0.07f, 0.07f };
        G.lightCol  = { 1.00f, 0.05f, 0.03f };
        G.ambient   = 0.02f;
        break;
    }
    // ... Aquí podrías añadir más casos para otras anomalías
}

// =============================================================================
//  activateLights
// =============================================================================
void activateLights(const RoomVariant& V)
{
    // Luz más sucia/tenue para los corredores, separándola de la sala
    const glm::vec3 corColor = { 0.75f, 0.72f, 0.65f }; 

    gNumLights = 0;

    auto addLight = [&](glm::vec3 pos, glm::vec3 col,
                        float Kc = 1.0f, float Kl = 0.07f, float Kq = 0.017f)
    {
        if (gNumLights >= MAX_LIGHTS) return;
        PointLight& L  = gLights[gNumLights++];
        L.pos          = pos;
        L.color        = col;
        L.intensity    = 0.85f; // Bajamos la intensidad máxima un poco
        L.Kc = Kc; L.Kl = Kl; L.Kq = Kq;
        L.isFlickering = false;
        L.flickerTimer = 1.5f + (static_cast<float>(rand()) / RAND_MAX) * 4.5f;
    };

    // ── Sala principal ────────────────────────────────────────────────────────
    // Aumentamos Kl y Kq mucho para que el radio de la luz muera en el primer
    // tramo del pasillo (A1/B1) y no alcance tu punto de aparición.
    for (auto& p : roomLightPositions(V.origin))
        addLight(p, G.lightCol, 1.0f, 0.18f, 0.05f);

    // ── Tramos de corredor ────────────────────────────────────────────────────
    auto addCorridorLights = [&](glm::vec3 center, float len, bool alongZ)
    {
        // Añadimos las luces de pasillo con una caída de luz mayor,
        // lo que genera huecos oscuros inquietantes entre lámpara y lámpara.
        for (auto& p : corridorLightPositions(center, len, alongZ))
            addLight(p, corColor, 1.0f, 0.15f, 0.035f);
    };

    // Lado A
    addCorridorLights(V.corA1Center, CL1, true);
    addCorridorLights(V.corA2Center, CL2, false);
    addCorridorLights(V.corA3Center, CL3, true);
    // Lado B
    addCorridorLights(V.corB1Center, CL1, true);
    addCorridorLights(V.corB2Center, CL2, false);
    addCorridorLights(V.corB3Center, CL3, true);
}

// =============================================================================
//  spawnAtCorridor
//  Coloca al jugador al inicio del pasillo horizontal indicado (A2 o B2),
//  mirando hacia el primer codo y la sala.
// =============================================================================
void spawnAtCorridor(const RoomVariant& V, bool corA)
{
    // Margen para no aparecer demasiado pegado a la pared del Codo A1 o B1
    const float margin = 2.0f;

    if (corA) {
        // Lado A: Tramo A2. Nos colocamos en la parte más cercana a la sala (X mayor)
        float spawnX = V.corA2Center.x + (CL2 / 2.f) - margin;
        camera.Position = glm::vec3(spawnX, EYE, V.corA2Center.z);
        // Miramos hacia la derecha (+X), en dirección al codo A1 y la sala
        camera.Yaw      = 0.0f;   
    } else {
        // Lado B: Tramo B2. Nos colocamos en la parte más cercana a la sala (X menor)
        float spawnX = V.corB2Center.x - (CL2 / 2.f) + margin;
        camera.Position = glm::vec3(spawnX, EYE, V.corB2Center.z);
        // Miramos hacia la izquierda (-X), en dirección al codo B1 y la sala
        camera.Yaw      = 180.0f; 
    }

    camera.Pitch = 0.0f;
    camera.UpdateCameraVectors();
    firstMouse = true; // Evita saltos bruscos de cámara en el primer frame
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
//
//  El mapa tiene forma de Z/S.  Las colisiones se resuelven calculando en qué
//  "zona" se encuentra el jugador y aplicando los límites correspondientes.
//
//  Zonas (referenciadas al origin de la variante):
//    SALA   : X ∈ [–HW, +HW],          Z ∈ [–HD, +HD]
//    A1     : X ∈ [–HW, –HW+CW],       Z ∈ [–HD–CL1, –HD]
//    A_ELBOW1: X ∈ [–HW–CW, –HW+CW],  Z ∈ [–HD–CL1–CW, –HD–CL1]
//    A2     : X ∈ [–HW–CL2, –HW],     Z ∈ [–HD–CL1–CW, –HD–CL1]
//    A_ELBOW2: X ∈ [–HW–CL2–CW, –HW–CL2], Z ∈ [–HD–CL1–CW, –HD–CL1]
//    A3     : X ∈ [–HW–CL2–CW, –HW–CL2], Z ∈ [–HD–CL1–CW–CL3, –HD–CL1–CW]
//  (Lado B: simétrico en X positivo, Z positivo)
// =============================================================================
void applyCollisions(glm::vec3& pos, const RoomVariant& V)
{
    const float R   = 0.25f;   // radio de colisión del jugador

    // Coordenadas locales al origin de la variante
    float lx = pos.x - V.origin.x;
    float lz = pos.z - V.origin.z;

    const float HW = RW / 2.f;
    const float HD = RD / 2.f;

    // ── Límites de cada zona ──────────────────────────────────────────────────
    // Corredor A (oeste)
    const float axL  = -HW;           // límite oeste del corredor A
    const float axR  = -HW + CW;      // límite este  del corredor A
    const float zA1n = -HD;
    const float zA1f = -HD - CL1;
    const float zE1n = zA1f;
    const float zE1f = zA1f - CW;
    const float xA2f = axL - CL2;
    const float xE2f = xA2f - CW;
    const float zA3n = zE1f;
    const float zA3f = zE1f - CL3;

    // Corredor B (este) – simétrico
    const float bxR  =  HW;
    const float bxL  =  HW - CW;
    const float zB1n =  HD;
    const float zB1f =  HD + CL1;
    const float zE3n = zB1f;
    const float zE3f = zB1f + CW;
    const float xB2f =  bxR + CL2;
    const float xE4f =  xB2f + CW;
    const float zB3n = zE3f;
    const float zB3f = zE3f + CL3;

    // ── Helper: clamp con margen ──────────────────────────────────────────────
    auto clampMin = [&](float& v, float lo) { if (v < lo + R) v = lo + R; };
    auto clampMax = [&](float& v, float hi) { if (v > hi - R) v = hi - R; };
    auto clampRange = [&](float& v, float lo, float hi) {
        clampMin(v, lo); clampMax(v, hi);
    };

    // ── Determinar zona y aplicar colisiones ──────────────────────────────────

    // ---- SALA ----------------------------------------------------------------
    if (lz >= -HD && lz <= HD) {
        clampRange(lx, -HW, HW);
        // Apertura Sur: X ∈ [axL, axR]  → no hay pared
        // Apertura Norte: X ∈ [bxL, bxR] → no hay pared
        // Pared Sur sólida: X ∈ [axR, HW]
        if (lz <= -HD + R) {
            // cerca de la pared sur: bloquear si no estamos en la apertura
            if (lx > axR) clampMax(lz, -HD);      // pared sur sólida
        }
        // Pared Norte sólida: X ∈ [–HW, bxL]
        if (lz >= HD - R) {
            if (lx < bxL) clampMin(lz, HD);       // pared norte sólida
        }
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CORREDOR A1 (Z negativo, columna oeste) -----------------------------
    if (lz < -HD && lz >= zA1f && lx >= axL - R && lx <= axR + R) {
        clampRange(lx, axL, axR);
        // Sin restricciones en Z para permitir el paso a la sala y al Codo A1
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CODO A1 -------------------------------------------------------------
    if (lz < zA1f && lz >= zE1f && lx >= axL - R && lx <= axR + R) {
        clampMax(lx, axR);     // pared este cerrada
        clampMin(lz, zE1f);    // pared sur cerrada
        // Abierto hacia el norte (Corredor A1) y oeste (Corredor A2)
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CORREDOR A2 (horizontal, hacia –X) ----------------------------------
    if (lz >= zE1f && lz <= zE1n && lx >= xA2f - R && lx <= axL + R) {
        clampRange(lz, zE1f, zE1n);
        // Abierto en ambos extremos de X para conectar con Codo A1 y Codo A2
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CODO A2 -------------------------------------------------------------
    if (lz >= zE1f && lz <= zE1n && lx < xA2f && lx >= xE2f - R) {
        clampMin(lx, xE2f);    // pared oeste cerrada
        clampMax(lz, zE1n);    // pared norte cerrada
        // Abierto hacia el este (Corredor A2) y sur (Corredor A3)
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CORREDOR A3 (Z negativo, columna más al oeste) ----------------------
    if (lx >= xE2f - R && lx <= xA2f + R && lz >= zA3f && lz <= zA3n) {
        clampRange(lx, xE2f, xA2f);
        clampMin(lz, zA3f);    // pared del fondo cerrada (donde spawneas)
        // Abierto hacia el norte para poder entrar al Codo A2
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CORREDOR B1 (Z positivo, columna este) ------------------------------
    if (lz > HD && lz <= zB1f && lx >= bxL - R && lx <= bxR + R) {
        clampRange(lx, bxL, bxR);
        // Abierto en Z para paso a la Sala y al Codo B1
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CODO B1 -------------------------------------------------------------
    if (lz > zB1f && lz <= zE3f && lx >= bxL - R && lx <= bxR + R) {
        clampMin(lx, bxL);     // pared oeste cerrada
        clampMax(lz, zE3f);    // pared norte cerrada
        // Abierto hacia el sur (Corredor B1) y este (Corredor B2)
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CORREDOR B2 (horizontal, hacia +X) ----------------------------------
    if (lz >= zE3n && lz <= zE3f && lx >= bxR - R && lx <= xB2f + R) {
        clampRange(lz, zE3n, zE3f);
        // Abierto en ambos extremos de X
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CODO B2 -------------------------------------------------------------
    if (lz >= zE3n && lz <= zE3f && lx > xB2f && lx <= xE4f + R) {
        clampMax(lx, xE4f);    // pared este cerrada
        clampMin(lz, zE3n);    // pared sur cerrada
        // Abierto hacia el oeste (Corredor B2) y norte (Corredor B3)
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ---- CORREDOR B3 (Z positivo, columna más al este) -----------------------
    if (lx >= xB2f - R && lx <= xE4f + R && lz >= zB3n && lz <= zB3f) {
        clampRange(lx, xB2f, xE4f);
        clampMax(lz, zB3f);    // pared del fondo cerrada
        // Abierto hacia el sur (Codo B2)
        pos.x = V.origin.x + lx;
        pos.z = V.origin.z + lz;
        return;
    }

    // ── Fallback: si no entra en ninguna zona (no debería ocurrir), mantenemos
    //    al jugador dentro de los límites absolutos de la variante.
    clampRange(lx, xE2f, xE4f);
    float absZmin = zA3f;
    float absZmax = zB3f;
    clampRange(lz, absZmin, absZmax);
    pos.x = V.origin.x + lx;
    pos.z = V.origin.z + lz;
}
