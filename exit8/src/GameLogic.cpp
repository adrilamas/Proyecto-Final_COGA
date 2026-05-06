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
/*void spawnAtCorridor(const RoomVariant& V, bool corA)
{
    if (corA) {
        // Zona segura en X=+3.5, mira hacia –X (pasillo recto A)
        camera.Position = V.origin + glm::vec3( -2.5f, EYE, -11.5f);
        //camera.Yaw      = 180.0f;
    } else {
        // Zona segura en X=-3.5, mira hacia +X (pasillo recto B)
        camera.Position = V.origin + glm::vec3(2.5f, EYE,  11.5f);
        //camera.Yaw      = 0.0f;
    }
    camera.Pitch = 0.0f;
    camera.UpdateCameraVectors();
    firstMouse = true;
}*/

/*void spawnAtCorridor(const RoomVariant& V, bool corA)
{
    if (corA) {
        // --- SPAWN EN PASILLO NORTE (A) ---
        // El pasillo está en X = 2.5. Aparecemos un poco antes del trigger de Z.
        // Nos posicionamos en Z = -11.0 para caminar hacia el centro (+Z).
        camera.Position = V.origin + glm::vec3(2.5f, EYE, -8.0f);
    }
    else {
        // --- SPAWN EN PASILLO SUR (B) ---
        // El pasillo está en X = -2.5.
        // Nos posicionamos en Z = 11.0 para caminar hacia el centro (-Z).
        camera.Position = V.origin + glm::vec3(-2.5f, EYE, 8.0f);
    }

    camera.UpdateCameraVectors();
    firstMouse = true; // Evita el salto brusco de la cámara al procesar el primer movimiento del ratón
}*/

void spawnAtCorridor(const RoomVariant& V, bool corA)
{
    float hw = RW / 2.f;
    float cor_len = CL - CW;
    float zDistNear = (RD / 2.f) + cor_len;

    // Mismos valores que usamos arriba para mantener la coherencia
    float triggerMargin = 3.5f;

    // Spawneamos 2 unidades MÁS CERCA de la habitación que el trigger, 
    // así el jugador está 100% a salvo de auto-teletransportarse al aparecer.
    float spawnOffsetZ = zDistNear - (triggerMargin + 2.0f);

    // Centros X corregidos (A está en la Izquierda, B está en la Derecha)
    float xCenter_A = -hw + (CW / 2.f);
    float xCenter_B = hw - (CW / 2.f);

    if (corA) {
        // Pasillo A (Sur). Z es negativo.
        camera.Position = V.origin + glm::vec3(xCenter_A, EYE, -spawnOffsetZ);
    }
    else {
        // Pasillo B (Norte). Z es positivo.
        camera.Position = V.origin + glm::vec3(xCenter_B, EYE, spawnOffsetZ);
    }

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

/*
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
}*/

void applyCollisions(glm::vec3& pos, const RoomVariant& V)
{
    constexpr float R = 0.2f; // radio del jugador

    float lx = pos.x - V.origin.x;
    float lz = pos.z - V.origin.z;

    // --- Geometría dinámica ---
    float hw = RW / 2.0f;                    // Mitad del ancho de la sala
    float hd = RD / 2.0f;                    // Mitad de la profundidad de la sala
    float cor_len = CL - CW;                 // Longitud del pasillo recto
    float zDistNear = hd + cor_len;          // Inicio del codo (antiguo 13.0f)
    float zDistFar = zDistNear + CW;         // Fondo del codo (antiguo 16.0f)

    // Coordenadas X de las paredes de los pasillos
    float xLeftWall_A = -hw;                // Antiguo -4.0f
    float xRightWall_A = -hw + CW;           // Antiguo -1.0f
    float xRightWall_B = hw;                 // Antiguo 4.0f
    float xLeftWall_B = hw - CW;            // Antiguo 1.0f

    // Punto medio Z para las colisiones de esquinas cóncavas
    float midZ = (hd + zDistNear) / 2.0f;    // Antiguo 8.5f

    // Techo absoluto de los fondos de los codos
    lz = glm::clamp(lz, -zDistFar + R, zDistFar - R);

    if (lz < 0.0f) {
        // ─── Lado Sur (–Z): habitación → corredor A → codo A ──────────────────
        lx = glm::max(lx, xLeftWall_A + R); // pared oeste continua

        // Esquina cóncava a la derecha del pasillo A
        if (lx > xRightWall_A - R && lz < -hd + R && lz > -zDistNear - R) {
            float dX = lx - (xRightWall_A - R);
            if (lz > -midZ) {
                float dZ = (-hd + R) - lz;
                if (dX < dZ) lx = xRightWall_A - R; else lz = -hd + R;
            }
            else {
                float dZ = lz - (-zDistNear - R);
                if (dX < dZ) lx = xRightWall_A - R; else lz = -zDistNear - R;
            }
        }

        // Topes en X según el sector Z
        if (lz < -zDistNear) lx = glm::min(lx, xRightWall_A - R); // codo A (tope en la pared interior)
        else if (lz < -hd)        lx = glm::min(lx, xRightWall_A - R); // pasillo recto A
        else                      lx = glm::min(lx, hw - R);           // habitación (sur)

    }
    else {
        // ─── Lado Norte (+Z): habitación → corredor B → codo B ────────────────
        lx = glm::min(lx, xRightWall_B - R); // pared este continua

        // Esquina cóncava a la izquierda del pasillo B
        if (lx < xLeftWall_B + R && lz > hd - R && lz < zDistNear + R) {
            float dX = (xLeftWall_B + R) - lx;
            if (lz < midZ) {
                float dZ = lz - (hd - R);
                if (dX < dZ) lx = xLeftWall_B + R; else lz = hd - R;
            }
            else {
                float dZ = (zDistNear + R) - lz;
                if (dX < dZ) lx = xLeftWall_B + R; else lz = zDistNear + R;
            }
        }

        // Topes en X según el sector Z
        if (lz > zDistNear) lx = glm::max(lx, xLeftWall_B + R); // codo B (tope en la pared interior)
        else if (lz > hd)        lx = glm::max(lx, xLeftWall_B + R); // pasillo recto B
        else                     lx = glm::max(lx, -hw + R);         // habitación (norte)
    }

    pos.x = V.origin.x + lx;
    pos.z = V.origin.z + lz;
}
