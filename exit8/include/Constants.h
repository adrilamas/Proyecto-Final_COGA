#pragma once

// ── Ventana ───────────────────────────────────────────────────────────────────
static constexpr int   SCR_WIDTH       = 1280;
static constexpr int   SCR_HEIGHT      = 720;

// ── Geometría de habitación ───────────────────────────────────────────────────
static constexpr float RW              = 8.0f;   // ancho  (X)
static constexpr float RD              = 8.0f;   // fondo  (Z)
static constexpr float RH              = 3.5f;   // altura (Y)
static constexpr float CW              = 3.0f;   // ancho del corredor
static constexpr float CL              = 12.0f;  // longitud total de cada corredor
static constexpr float EYE             = 1.7f;   // altura de los ojos del jugador

// ── Variantes ─────────────────────────────────────────────────────────────────
// Desplazamiento en X entre variantes (debe ser >> RW + 2*CL)
static constexpr float VARIANT_STRIDE  = 200.0f;
static constexpr int   NUM_VARIANTS    = 2;

// ── Offsets de corredor (relativo al centro de la habitación) ─────────────────
static constexpr float COR_A_DX = -(RW / 2.0f - CW / 2.0f);  // -2.5 (esquina oeste)
static constexpr float COR_B_DX =  (RW / 2.0f - CW / 2.0f);  // +2.5 (esquina este)

// ── Triggers (relativo al centro de la habitación) ────────────────────────────
static constexpr float TRIG_A_DZ = -(RD / 2.0f + CL * 0.5f); // -10.0
static constexpr float TRIG_B_DZ =  (RD / 2.0f + CL * 0.5f); // +10.0

// ── Spawns (relativo al centro de la habitación) ──────────────────────────────
static constexpr float SPAWN_A_DZ = -(RD / 2.0f + CL * 0.15f); // -5.2
static constexpr float SPAWN_B_DZ =  (RD / 2.0f + CL * 0.15f); // +5.2

// ── Luces ─────────────────────────────────────────────────────────────────────
static constexpr int MAX_LIGHTS = 16;
