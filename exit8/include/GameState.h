#pragma once

#include <glm/glm.hpp>
#include "AnomalyType.h"

// ── Estados de la máquina de juego ────────────────────────────────────────────
enum class GameState { PLAYING, FADING_OUT, FADING_IN };

// Dirección por la que el jugador entró a la habitación actual.
// Determina si "vuelve atrás" o "avanza" al cruzar el trigger.
enum class EntryDir { FROM_A, FROM_B };

// ── Datos globales del juego ──────────────────────────────────────────────────
struct GameData {
    // Estado general
    GameState   state          = GameState::PLAYING;
    int         score          = 0;
    int         highScore      = 0;

    // Variante y anomalía activas
    int         currentVariant = 0;
    AnomalyType currentAnomaly = AnomalyType::NONE;
    EntryDir    entryDir       = EntryDir::FROM_A;

    // Fade a negro
    float       fadeAlpha      = 0.0f;
    static constexpr float FADE_SPEED = 3.0f;

    // Teleport pendiente (se ejecuta al terminar el fade)
    struct PendingTeleport {
        bool active    = false;
        int  toVariant = 0;
        bool toCorA    = true;  // true = spawn en corredor A, false = corredor B
        bool isCorrect = true;
    } pending;

    // Colores actuales de la habitación (los cambia applyAnomalyColors)
    glm::vec3 roomWall  = { 0.82f, 0.80f, 0.75f };
    glm::vec3 roomFloor = { 0.55f, 0.52f, 0.48f };
    glm::vec3 roomCeil  = { 0.90f, 0.90f, 0.92f };
    glm::vec3 lightCol  = { 0.95f, 0.95f, 1.00f };
    float     ambient   = 0.04f;
};

// Instancia global (definida en main.cpp)
extern GameData G;
