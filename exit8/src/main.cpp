// main.cpp  ─  Exit 8 | Iluminación base del pasillo
// ─────────────────────────────────────────────────────────────────────────────
//  Sistema de luces de techo (point lights fluorescentes) con soporte para:
//    · Múltiples luces a lo largo del pasillo
//    · Parpadeo aleatorio por anomalía (tecla F para forzarlo)
//    · Apagón total (tecla L)
//    · Cada luz tiene sus propios coeficientes de atenuación
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <BibliotecasCurso/lecturaShader_0_9.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include "Camera.h"

// ── Configuración de ventana ──────────────────────────────────────────────────
static constexpr int  SCR_WIDTH  = 1280;
static constexpr int  SCR_HEIGHT = 720;

// ── Cámara global ─────────────────────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 1.7f, 0.0f));
float lastX      = SCR_WIDTH  / 2.0f;
float lastY      = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;

// ── DeltaTime ─────────────────────────────────────────────────────────────────
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ── Prototipos ────────────────────────────────────────────────────────────────
void framebufferSizeCallback(GLFWwindow*, int, int);
void mouseCallback          (GLFWwindow*, double, double);
void scrollCallback         (GLFWwindow*, double, double);
void processInput           (GLFWwindow*);
static void openGlInit();

// =============================================================================
//  SISTEMA DE LUCES DE TECHO
// =============================================================================

static constexpr int MAX_LIGHTS = 8;

// Estructura que se mapea 1:1 con el struct del fragment shader
struct PointLight {
    glm::vec3 position;
    float     intensity;   // 0.0 = apagada, 1.0 = plena
    glm::vec3 color;
    // Atenuación: 1 / (Kc + Kl*d + Kq*d²)
    float Kc = 1.0f;
    float Kl = 0.07f;  // rango ~30 unidades
    float Kq = 0.017f;

    // --- Estado de parpadeo (solo CPU, no se envía al shader) ---
    float flickerTimer    = 0.0f;  // temporizador hasta el próximo evento
    float flickerDuration = 0.0f;  // duración del parpadeo activo
    bool  isFlickering    = false;
};

// Colocamos las luces a lo largo del pasillo (eje Z negativo = hacia adentro)
// Ajusta la separación y cantidad según el tamaño de tu pasillo.
static PointLight ceilingLights[MAX_LIGHTS] = {
    // position                    intensity  color (blanco frío fluorescente)
    { glm::vec3( 0.0f, 3.0f,  -2.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f,  -8.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f, -14.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f, -20.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f, -26.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f, -32.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f, -38.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
    { glm::vec3( 0.0f, 3.0f, -44.0f), 1.0f, glm::vec3(0.95f, 0.95f, 1.00f) },
};
static int numLights = 8;

// ── Modos de anomalía ─────────────────────────────────────────────────────────
static bool anomalyFlickerActive = false;  // tecla F: activa parpadeo aleatorio
static bool anomalyBlackout      = false;  // tecla L: apagón total

// Tiempo mínimo entre parpadeos por luz (segundos)
static constexpr float FLICKER_MIN_INTERVAL = 1.5f;
static constexpr float FLICKER_MAX_INTERVAL = 6.0f;
// Duración del parpadeo (rápido, como un fluorescente fallando)
static constexpr float FLICKER_ON_TIME      = 0.05f;
static constexpr float FLICKER_OFF_TIME     = 0.08f;

// Teclas F y L con debounce
static bool keyF_prev = false;
static bool keyL_prev = false;

// ── Actualización de parpadeos ────────────────────────────────────────────────
void updateLights(float dt)
{
    for (int i = 0; i < numLights; i++) {
        PointLight& L = ceilingLights[i];

        // Apagón: todas a cero, sin tocar temporizadores
        if (anomalyBlackout) {
            L.intensity = 0.0f;
            continue;
        }

        // Sin anomalía: todas encendidas al máximo
        if (!anomalyFlickerActive) {
            L.intensity    = 1.0f;
            L.isFlickering = false;
            continue;
        }

        // ── Modo parpadeo: cada luz tiene su propio reloj ────────────────────
        L.flickerTimer -= dt;

        if (L.isFlickering) {
            // Durante el parpadeo: alterna rápidamente encendido/apagado
            // Usamos una onda cuadrada de alta frecuencia (15 Hz)
            float phase = fmod(static_cast<float>(glfwGetTime()), 1.0f / 15.0f);
            L.intensity = (phase < (1.0f / 30.0f)) ? 1.0f : 0.0f;

            L.flickerDuration -= dt;
            if (L.flickerDuration <= 0.0f) {
                L.isFlickering = false;
                L.intensity    = 1.0f;
                // Programar el próximo parpadeo
                L.flickerTimer = FLICKER_MIN_INTERVAL +
                    (static_cast<float>(rand()) / RAND_MAX) *
                    (FLICKER_MAX_INTERVAL - FLICKER_MIN_INTERVAL);
            }
        } else {
            // Esperando: si el timer llega a cero, iniciamos un parpadeo
            if (L.flickerTimer <= 0.0f) {
                L.isFlickering    = true;
                // Duración aleatoria entre 0.2 y 0.8 segundos
                L.flickerDuration = 0.2f + (static_cast<float>(rand()) / RAND_MAX) * 0.6f;
            } else {
                L.intensity = 1.0f;
            }
        }
    }
}

// ── Envío de luces al shader ──────────────────────────────────────────────────
void uploadLightsToShader(unsigned int program)
{
    for (int i = 0; i < numLights; i++) {
        const PointLight& L = ceilingLights[i];

        // Construimos el nombre del uniform como "uLights[i].campo"
        std::string base = "uLights[" + std::to_string(i) + "].";

        glUniform3fv(glGetUniformLocation(program, (base + "position").c_str()),
                     1, glm::value_ptr(L.position));
        glUniform3fv(glGetUniformLocation(program, (base + "color").c_str()),
                     1, glm::value_ptr(L.color));
        glUniform1f (glGetUniformLocation(program, (base + "intensity").c_str()),
                     L.intensity);
        glUniform1f (glGetUniformLocation(program, (base + "Kc").c_str()), L.Kc);
        glUniform1f (glGetUniformLocation(program, (base + "Kl").c_str()), L.Kl);
        glUniform1f (glGetUniformLocation(program, (base + "Kq").c_str()), L.Kq);
    }
    glUniform1i(glGetUniformLocation(program, "uNumLights"), numLights);
}

// =============================================================================
//  GEOMETRÍA DE PRUEBA: pasillo simple (suelo + 2 paredes + techo)
// =============================================================================
//
// Vértices: pos(3) + normal(3) + UV(2) = stride 8 floats
//
// El pasillo corre a lo largo del eje -Z.
// Dimensiones: 5 u de ancho, 3.5 u de alto, 50 u de largo.

struct Mesh {
    unsigned int VAO, VBO;
    int          count;
};

Mesh buildQuad(glm::vec3 origin,
               glm::vec3 right, float rLen,
               glm::vec3 up,    float uLen,
               glm::vec3 normal,
               float uvScaleU = 1.0f, float uvScaleV = 1.0f)
{
    // Dos triángulos que forman un quad
    float verts[48]; // 6 vértices × 8 floats
    glm::vec3 p0 = origin;
    glm::vec3 p1 = origin + right * rLen;
    glm::vec3 p2 = origin + right * rLen + up * uLen;
    glm::vec3 p3 = origin + up * uLen;

    auto fill = [&](int idx, glm::vec3 p, float u, float v) {
        int b = idx * 8;
        verts[b+0] = p.x; verts[b+1] = p.y; verts[b+2] = p.z;
        verts[b+3] = normal.x; verts[b+4] = normal.y; verts[b+5] = normal.z;
        verts[b+6] = u; verts[b+7] = v;
    };

    fill(0, p0, 0.0f,       0.0f      );
    fill(1, p1, uvScaleU,   0.0f      );
    fill(2, p2, uvScaleU,   uvScaleV  );
    fill(3, p2, uvScaleU,   uvScaleV  );
    fill(4, p3, 0.0f,       uvScaleV  );
    fill(5, p0, 0.0f,       0.0f      );

    Mesh m;
    m.count = 6;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return m;
}

// =============================================================================
//  MAIN
// =============================================================================
int main()
{
    srand(42); // Semilla fija para que los parpadeos sean reproducibles

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "The Exit 8", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback      (window, mouseCallback);
    glfwSetScrollCallback         (window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    openGlInit();

    // ── Inicializar timers de parpadeo aleatoriamente para que no sean síncronos ─
    for (int i = 0; i < numLights; i++) {
        ceilingLights[i].flickerTimer =
            FLICKER_MIN_INTERVAL +
            (static_cast<float>(rand()) / RAND_MAX) *
            (FLICKER_MAX_INTERVAL - FLICKER_MIN_INTERVAL);
    }

    // ── Cargar shader ─────────────────────────────────────────────────────────
    unsigned int shader = setShaders("exit8.vert", "exit8.frag");

    // Locations que usamos frecuentemente (precacheadas)
    int locModel  = glGetUniformLocation(shader, "uModel");
    int locView   = glGetUniformLocation(shader, "uView");
    int locProj   = glGetUniformLocation(shader, "uProjection");
    int locViewPos= glGetUniformLocation(shader, "uViewPos");
    int locAmbient= glGetUniformLocation(shader, "uAmbient");
    int locUseTex = glGetUniformLocation(shader, "uUsarTextura");
    int locColor  = glGetUniformLocation(shader, "uColor");

    // ── Construir el pasillo ──────────────────────────────────────────────────
    // Pasillo: 5u ancho, 3.5u alto, 50u largo (corre hacia -Z)
    constexpr float W = 5.0f;   // semi-ancho total
    constexpr float H = 3.5f;   // altura
    constexpr float L = 50.0f;  // longitud

    // Suelo: Y=0, se extiende en X[-W/2, W/2] y Z[0, -L]
    Mesh floor = buildQuad(
        glm::vec3(-W/2, 0.0f,  0.0f),
        glm::vec3( 1.0f, 0.0f,  0.0f), W,
        glm::vec3( 0.0f, 0.0f, -1.0f), L,
        glm::vec3( 0.0f, 1.0f,  0.0f),   // normal hacia arriba
        5.0f, 25.0f  // UV scale: repetir textura
    );

    // Techo: Y=H
    Mesh ceiling = buildQuad(
        glm::vec3(-W/2, H, -L),
        glm::vec3( 1.0f, 0.0f,  0.0f), W,
        glm::vec3( 0.0f, 0.0f,  1.0f), L,
        glm::vec3( 0.0f,-1.0f,  0.0f),   // normal hacia abajo
        5.0f, 25.0f
    );

    // Pared izquierda: X=-W/2
    Mesh wallLeft = buildQuad(
        glm::vec3(-W/2, 0.0f,  0.0f),
        glm::vec3( 0.0f, 0.0f, -1.0f), L,
        glm::vec3( 0.0f, 1.0f,  0.0f), H,
        glm::vec3( 1.0f, 0.0f,  0.0f),   // normal hacia +X (interior)
        12.0f, 2.0f
    );

    // Pared derecha: X=+W/2
    Mesh wallRight = buildQuad(
        glm::vec3( W/2, 0.0f, -L),
        glm::vec3( 0.0f, 0.0f,  1.0f), L,
        glm::vec3( 0.0f, 1.0f,  0.0f), H,
        glm::vec3(-1.0f, 0.0f,  0.0f),   // normal hacia -X (interior)
        12.0f, 2.0f
    );

    // Pared del fondo: Z=-L
    Mesh wallBack = buildQuad(
        glm::vec3(-W/2, 0.0f, -L),
        glm::vec3( 1.0f, 0.0f,  0.0f), W,
        glm::vec3( 0.0f, 1.0f,  0.0f), H,
        glm::vec3( 0.0f, 0.0f,  1.0f),   // normal hacia +Z (interior)
        5.0f, 2.0f
    );

    // ── Bucle principal ───────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // ── Actualizar luces ──────────────────────────────────────────────────
        updateLights(deltaTime);

        // ── Render ───────────────────────────────────────────────────────────
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        // Matrices de cámara
        glm::mat4 proj  = camera.GetProjectionMatrix((float)SCR_WIDTH, (float)SCR_HEIGHT);
        glm::mat4 view  = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        glUniformMatrix4fv(locView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj,  1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv      (locViewPos, 1, glm::value_ptr(camera.Position));

        // Luz ambiente: muy tenue para simular un pasillo interior
        // En anomalía de apagón bajamos a casi negro
        float ambientLevel = anomalyBlackout ? 0.01f : 0.04f;
        glUniform3f(locAmbient, ambientLevel, ambientLevel, ambientLevel * 1.05f);

        // Subir todas las luces de techo al shader
        uploadLightsToShader(shader);

        // Sin textura por ahora (se añadirán en la siguiente fase)
        glUniform1i(locUseTex, 0);

        // ── Dibujar suelo ──────────────────────────────────────────────────────
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(locColor, 0.55f, 0.52f, 0.48f); // beige sucio
        glBindVertexArray(floor.VAO);
        glDrawArrays(GL_TRIANGLES, 0, floor.count);

        // ── Dibujar techo ──────────────────────────────────────────────────────
        glUniform3f(locColor, 0.90f, 0.90f, 0.92f); // blanco/gris claro
        glBindVertexArray(ceiling.VAO);
        glDrawArrays(GL_TRIANGLES, 0, ceiling.count);

        // ── Dibujar paredes ────────────────────────────────────────────────────
        glUniform3f(locColor, 0.82f, 0.80f, 0.75f); // blanco roto
        glBindVertexArray(wallLeft.VAO);
        glDrawArrays(GL_TRIANGLES, 0, wallLeft.count);

        glBindVertexArray(wallRight.VAO);
        glDrawArrays(GL_TRIANGLES, 0, wallRight.count);

        glUniform3f(locColor, 0.75f, 0.73f, 0.70f); // pared del fondo ligeramente más oscura
        glBindVertexArray(wallBack.VAO);
        glDrawArrays(GL_TRIANGLES, 0, wallBack.count);

        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Limpieza ──────────────────────────────────────────────────────────────
    // (Para simplificar, GLFW limpiará el contexto)
    glfwTerminate();
    return 0;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void framebufferSizeCallback(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);
}

void mouseCallback(GLFWwindow*, double xIn, double yIn)
{
    float x = static_cast<float>(xIn);
    float y = static_cast<float>(yIn);
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    float xOff = x - lastX;
    float yOff = lastY - y;
    lastX = x; lastY = y;
    camera.ProcessMouseMovement(xOff, yOff);
}

void scrollCallback(GLFWwindow*, double, double yOff)
{
    camera.Fov -= static_cast<float>(yOff);
    if (camera.Fov < 20.0f) camera.Fov = 20.0f;
    if (camera.Fov > 90.0f) camera.Fov = 90.0f;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::FORWARD,  deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::LEFT,     deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::RIGHT,    deltaTime);

    // ── F: alternar modo parpadeo ─────────────────────────────────────────────
    bool keyF = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (keyF && !keyF_prev) {
        anomalyFlickerActive = !anomalyFlickerActive;
        if (!anomalyFlickerActive)
            for (int i = 0; i < numLights; i++)
                ceilingLights[i].intensity = 1.0f;
        std::cout << "[F] Parpadeo: " << (anomalyFlickerActive ? "ON" : "OFF") << "\n";
    }
    keyF_prev = keyF;

    // ── L: alternar apagón total ──────────────────────────────────────────────
    bool keyL = (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS);
    if (keyL && !keyL_prev) {
        anomalyBlackout = !anomalyBlackout;
        std::cout << "[L] Apagón: " << (anomalyBlackout ? "ON" : "OFF") << "\n";
    }
    keyL_prev = keyL;
}

static void openGlInit()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}
