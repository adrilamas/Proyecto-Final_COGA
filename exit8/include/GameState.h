#pragma once

#include <glm/glm.hpp>
#include "AnomalyType.h"

// ── Estados de la máquina de juego ────────────────────────────────────────────
enum class GameState { PLAYING };

// Dirección por la que el jugador entró a la habitación actual.
enum class EntryDir { FROM_A, FROM_B };

// ── Datos globales del juego ──────────────────────────────────────────────────
struct GameData {
    GameState   state          = GameState::PLAYING;
    int         score          = 0;
    int         highScore      = 0;

    int         currentVariant = 0;
    AnomalyType currentAnomaly = AnomalyType::NONE;
    EntryDir    entryDir       = EntryDir::FROM_A;

    // Colores actuales de la habitación
    glm::vec3 roomWall  = { 0.82f, 0.80f, 0.75f };
    glm::vec3 roomFloor = { 0.55f, 0.52f, 0.48f };
    glm::vec3 roomCeil  = { 0.90f, 0.90f, 0.92f };
    glm::vec3 lightCol  = { 0.95f, 0.95f, 1.00f };
    float     ambient   = 0.04f;
};

extern GameData G;
