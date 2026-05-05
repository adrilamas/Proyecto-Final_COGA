#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "AnomalyType.h"
#include "Room.h"
#include "Mesh.h"

// =============================================================================
//  RoomVariant
//  Una variante = habitación + dos corredores con sus codos, desplazada en X.
//  El VARIANT_STRIDE garantiza que nunca sean visibles dos variantes a la vez.
// =============================================================================
struct RoomVariant {
    AnomalyType anomaly;
    glm::vec3   origin;       // centro de la habitación en world-space

    Room        room;

    Corridor    corA;         // pasillo recto hacia -Z (esquina oeste)
    Corridor    corB;         // pasillo recto hacia +Z (esquina este)

    Corridor    corATurn;     // codo horizontal del pasillo A
    Corridor    corBTurn;     // codo horizontal del pasillo B

    Mesh        endWallA;     // pared de fondo del codo A
    Mesh        endWallB;     // pared de fondo del codo B
    Mesh        cornerWallA;  // pared de esquina interior del codo A
    Mesh        cornerWallB;  // pared de esquina interior del codo B

    glm::vec3   corACenter;   // centro del corredor A (para las luces)
    glm::vec3   corBCenter;   // centro del corredor B
};

// ── Factory ───────────────────────────────────────────────────────────────────
RoomVariant buildVariant(int variantIdx, AnomalyType anomaly);

// ── Posiciones de luz ─────────────────────────────────────────────────────────
std::vector<glm::vec3> roomLightPositions    (glm::vec3 origin);
std::vector<glm::vec3> corridorLightPositions(glm::vec3 corridorCenter, float len);
