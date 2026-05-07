#pragma once

#include <glm/glm.hpp>
#include "GameState.h"
#include "RoomVariant.h"

// ── Anomalías ─────────────────────────────────────────────────────────────────
// Aplica los colores de sala y luces que corresponden a la anomalía dada,
// actualizando los campos de G (GameData global).
void applyAnomalyColors(AnomalyType a);

// ── Luces ─────────────────────────────────────────────────────────────────────
// Recalcula y sube al banco de luces todas las fuentes de la variante dada.
void activateLights(const RoomVariant& V);

// ── Spawn ─────────────────────────────────────────────────────────────────────
// Coloca la cámara al inicio del corredor indicado mirando hacia la habitación.
//   corA = true  → spawn en pasillo A (Z negativo), mirando hacia +Z
//   corA = false → spawn en pasillo B (Z positivo), mirando hacia -Z
void spawnAtCorridor(const RoomVariant& V, bool corA);

// ── Variantes ─────────────────────────────────────────────────────────────────
// Devuelve el índice de la siguiente variante, evitando repetir la actual.
int nextVariantIdx(int current);

// ── Colisiones ────────────────────────────────────────────────────────────────
// Restringe pos a los límites geométricos de la variante V.
void applyCollisions(glm::vec3& pos, const RoomVariant& V);
