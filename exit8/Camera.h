#pragma once
#include <glm/glm.hpp>

// Direcciones de movimiento para el teclado
enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera
{
public:
    // ── Posicion y orientacion ──────────────────────────────────────────────
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // ── Angulos de Euler ────────────────────────────────────────────────────
    float Yaw;    // rotacion horizontal (izq/dcha)
    float Pitch;  // rotacion vertical   (arriba/abajo)

    // ── Parametros configurables ────────────────────────────────────────────
    float MovementSpeed;
    float MouseSensitivity;
    float Fov;                // campo de vision en grados
    float EyeHeight;          // altura del ojo sobre el suelo (metros)

    // Constructor: posicion inicial opcional
    Camera(glm::vec3 startPosition = glm::vec3(0.0f, 1.7f, 0.0f));

    // Devuelve la matriz View para enviar al shader
    glm::mat4 GetViewMatrix() const;

    // Devuelve la matriz Projection (necesita ancho y alto de la ventana)
    glm::mat4 GetProjectionMatrix(float screenWidth, float screenHeight) const;

    // Procesa input de teclado con deltaTime para velocidad constante
    void ProcessKeyboard(CameraMovement direction, float deltaTime);

    // Procesa movimiento del raton (offsets ya calculados)
    void ProcessMouseMovement(float xOffset, float yOffset,
                              bool constrainPitch = true);

private:
    // Recalcula Front, Right y Up a partir de Yaw y Pitch
    void UpdateCameraVectors();
};
