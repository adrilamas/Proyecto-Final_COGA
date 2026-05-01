#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm> // std::clamp

// ── Valores por defecto ──────────────────────────────────────────────────────
static constexpr float DEFAULT_YAW         = -90.0f; // mira hacia -Z al inicio
static constexpr float DEFAULT_PITCH       =   0.0f;
static constexpr float DEFAULT_SPEED       =   5.0f; // unidades/segundo
static constexpr float DEFAULT_SENSITIVITY =   0.08f;
static constexpr float DEFAULT_FOV         =  70.0f;
static constexpr float EYE_HEIGHT          =   1.7f; // metros

// ── Constructor ──────────────────────────────────────────────────────────────
Camera::Camera(glm::vec3 startPosition)
    : Position(startPosition),
      WorldUp(0.0f, 1.0f, 0.0f),
      Yaw(DEFAULT_YAW),
      Pitch(DEFAULT_PITCH),
      MovementSpeed(DEFAULT_SPEED),
      MouseSensitivity(DEFAULT_SENSITIVITY),
      Fov(DEFAULT_FOV),
      EyeHeight(EYE_HEIGHT)
{
    UpdateCameraVectors();
}

// ── Matriz View ──────────────────────────────────────────────────────────────
glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

// ── Matriz Projection ────────────────────────────────────────────────────────
glm::mat4 Camera::GetProjectionMatrix(float screenWidth, float screenHeight) const
{
    return glm::perspective(
        glm::radians(Fov),
        screenWidth / screenHeight,
        0.1f,    // near plane
        200.0f   // far plane (pasillo largo del Exit 8)
    );
}

// ── Teclado ──────────────────────────────────────────────────────────────────
void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;

    // Movimiento en el plano XZ (ignoramos Y del vector Front para no "volar")
    glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));

    switch (direction)
    {
        case CameraMovement::FORWARD:  Position += flatFront * velocity; break;
        case CameraMovement::BACKWARD: Position -= flatFront * velocity; break;
        case CameraMovement::LEFT:     Position -= flatRight * velocity; break;
        case CameraMovement::RIGHT:    Position += flatRight * velocity; break;
    }

    // Bloquear altura del ojo: el jugador siempre camina en Y = EyeHeight
    Position.y = EyeHeight;
}

// ── Raton ────────────────────────────────────────────────────────────────────
void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    xOffset *= MouseSensitivity;
    yOffset *= MouseSensitivity;

    Yaw   += xOffset;
    Pitch += yOffset;

    // Evitar gimbal lock: limitar Pitch a +-89 grados
    if (constrainPitch)
        Pitch = std::clamp(Pitch, -89.0f, 89.0f);

    UpdateCameraVectors();
}

// ── Recalcular vectores internos ─────────────────────────────────────────────
void Camera::UpdateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    Front = glm::normalize(front);
    // Right: producto cruzado de Front y WorldUp
    Right = glm::normalize(glm::cross(Front, WorldUp));
    // Up: producto cruzado de Right y Front (ortogonal a ambos)
    Up    = glm::normalize(glm::cross(Right, Front));
}
