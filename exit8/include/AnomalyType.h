#pragma once
#include <cstdlib>

// =============================================================================
//  Tipos de anomalía disponibles.
//  Para añadir una nueva anomalía:
//    1. Añade una entrada aquí (antes de ANOMALY_COUNT).
//    2. Añade el caso en applyAnomalyColors() en GameLogic.cpp.
//    3. Opcionalmente añade geometría/lógica extra.
// =============================================================================
enum class AnomalyType {
    NONE = 0,       // Habitación normal, luces blancas
    RED_LIGHTS,     // Todas las luces se vuelven rojas
    // DARK_ROOM,   // ejemplo: sala casi a oscuras (descomenta y añade caso)
    // BLUE_LIGHTS, // ejemplo: luces azules frías
    ANOMALY_COUNT   // ← siempre al final, sirve como centinela
};

// Devuelve una anomalía aleatoria uniforme entre todas las disponibles.
// Con 50 % de probabilidad de NONE para que no siempre haya anomalía.
inline AnomalyType randomAnomaly()
{
    // 50 % ninguna
    if (rand() % 2 == 0) return AnomalyType::NONE;

    // 50 % restante: cualquiera de las anomalías reales (excluye NONE y COUNT)
    int numReal = static_cast<int>(AnomalyType::ANOMALY_COUNT) - 1; // sin NONE
    if (numReal <= 0) return AnomalyType::NONE;

    int pick = 1 + (rand() % numReal); // 1 .. ANOMALY_COUNT-1
    return static_cast<AnomalyType>(pick);
}
