#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "AnomalyType.h"
#include "Room.h"
#include "Mesh.h"

// =============================================================================
//  RoomVariant – habitación principal + pasillo en forma de Z/S
//
//  Recorrido del jugador (desde el pasillo de entrada hasta el trigger):
//
//    [SALA PRINCIPAL]
//         │  (Tramo 1: CL1 hacia ±Z, por el lado del corredor)
//    [GIRO 1]  (codo 90°, hacia ±X)
//         │  (Tramo 2: CL2 perpendicular)
//    [GIRO 2]  (codo 90°, de vuelta a ±Z)
//         │  (Tramo 3: CL3 – trigger a mitad)
//    [TRIGGER → teleport]
//
//  El layout es simétrico: lado A sale hacia -Z, lado B hacia +Z.
//  Cada lado desplaza el corredor al borde opuesto de la sala en X,
//  de modo que al teletransportarse el jugador puede girar y ver
//  "el mismo pasillo" detrás de él.
// =============================================================================
struct RoomVariant {
    AnomalyType anomaly;
    glm::vec3   origin;       // centro de la sala principal en world-space

    // ── Sala principal ────────────────────────────────────────────────────────
    Room        room;

    // ── Lado A (sale hacia –Z desde la esquina oeste de la sala) ─────────────
    Corridor    corA1;        // Tramo 1: recto hacia –Z
    Mesh        elbowA1floor, elbowA1ceil; // Codo 1 (suelo y techo)
    Mesh        elbowA1wallOuter, elbowA1wallInner; // paredes del codo 1
    Corridor    corA2;        // Tramo 2: perpendicular (hacia –X)
    Mesh        elbowA2floor, elbowA2ceil;
    Mesh        elbowA2wallOuter, elbowA2wallInner;
    Corridor    corA3;        // Tramo 3: de vuelta hacia –Z (trigger aquí)
    Mesh        endWallA;     // pared de cierre al final del tramo 3

    // ── Lado B (sale hacia +Z desde la esquina este de la sala) ──────────────
    Corridor    corB1;
    Mesh        elbowB1floor, elbowB1ceil;
    Mesh        elbowB1wallOuter, elbowB1wallInner;
    Corridor    corB2;
    Mesh        elbowB2floor, elbowB2ceil;
    Mesh        elbowB2wallOuter, elbowB2wallInner;
    Corridor    corB3;
    Mesh        endWallB;

    // ── Paredes de cierre que tapan los huecos entre sala y pasillos ─────────
    Mesh        gapWallAN;    // tapa el lado N de la sala junto al corredor A
    Mesh        gapWallBS;    // tapa el lado S de la sala junto al corredor B

    // ── Posiciones de luz para cada tramo (usadas por activateLights) ─────────
    glm::vec3   corA1Center, corA2Center, corA3Center;
    glm::vec3   corB1Center, corB2Center, corB3Center;

    // ── Distancia del trigger desde el origin (en Z) ──────────────────────────
    // El trigger se activa a mitad del Tramo 3.
    // Se calcula en buildVariant y se guarda aquí para usarlo en main y spawn.
    float       triggerDist;  // positivo; lado A usa –triggerDist, lado B +triggerDist
};

// ── Factory ───────────────────────────────────────────────────────────────────
RoomVariant buildVariant(int variantIdx, AnomalyType anomaly);

// ── Posiciones de luz ─────────────────────────────────────────────────────────
std::vector<glm::vec3> roomLightPositions    (glm::vec3 origin);
std::vector<glm::vec3> corridorLightPositions(glm::vec3 center, float len, bool alongZ);
