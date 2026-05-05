#pragma once

// Tipos de anomalía que puede presentar una habitación.
enum class AnomalyType {
    NONE,       // Habitación normal, luces blancas
    RED_LIGHTS  // Todas las luces se vuelven rojas
};

// Devuelve una anomalía aleatoria (50 % ninguna, 50 % luces rojas)
inline AnomalyType randomAnomaly() {
    return (rand() % 2 == 0) ? AnomalyType::NONE : AnomalyType::RED_LIGHTS;
}
