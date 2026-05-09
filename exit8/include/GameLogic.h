#pragma once

#include <glm/glm.hpp>
#include "GameState.h"
#include "RoomVariant.h"

// Aplica colores y ambiente según la anomalía.
void applyAnomalyColors(AnomalyType a);

// Recalcula y sube al banco de luces todas las fuentes de la variante.
void activateLights(const RoomVariant& V);

// Coloca la cámara al inicio del corredor indicado mirando hacia la sala.
//   corA = true  → spawn en pasillo A (Z negativo), mirando hacia +Z
//   corA = false → spawn en pasillo B (Z positivo), mirando hacia -Z
void spawnAtCorridor(const RoomVariant& V, bool corA);

// Devuelve el índice de la siguiente variante (diferente a current).
int nextVariantIdx(int current);

// Restringe la posición a los límites geométricos de la variante.
void applyCollisions(glm::vec3& pos, const RoomVariant& V);
