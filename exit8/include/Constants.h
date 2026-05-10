#pragma once

// ── Ventana ───────────────────────────────────────────────────────────────────
static constexpr int   SCR_WIDTH       = 1280;
static constexpr int   SCR_HEIGHT      = 720;

// ── Geometría de habitación ───────────────────────────────────────────────────
static constexpr float RW              = 20.0f;   // ancho  (X)  – sala principal
static constexpr float RD              = 4.0f;    // fondo  (Z)  – sala principal
static constexpr float RH              = 4.5f;    // altura (Y)  – toda la escena
static constexpr float CW              = 4.0f;    // ancho del corredor (más amplio)

// Longitudes de los tramos del camino (en unidades de mundo):
//   Tramo 1: pasillo recto desde la sala hacia el primer giro
static constexpr float CL1             = 10.0f;
//   Tramo 2: pasillo tras el primer giro (perpendicular)
static constexpr float CL2             = 8.0f;
//   Tramo 3: pasillo tras el segundo giro (parallel de vuelta)
static constexpr float CL3             = 10.0f;   // el trigger está a mitad de este

static constexpr float EYE             = 1.8f;    // altura de los ojos del jugador

// ── Variantes ─────────────────────────────────────────────────────────────────
static constexpr float VARIANT_STRIDE  = 300.0f;  // separación en X entre variantes
static constexpr int   NUM_VARIANTS    = 2;

// ── Luces ─────────────────────────────────────────────────────────────────────
static constexpr int MAX_LIGHTS = 16;
