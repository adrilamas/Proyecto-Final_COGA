// =============================================================================
//  main.cpp  ─  Exit 8 | Punto de entrada y bucle principal
// =============================================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <BibliotecasCurso/lecturaShader_0_9.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <array>

#include "Constants.h"
#include "AnomalyType.h"
#include "GameState.h"
#include "Lighting.h"
#include "Mesh.h"
#include "Room.h"
#include "RoomVariant.h"
#include "GameLogic.h"
#include "Camera.h"

// =============================================================================
//  Globales (compartidos con otros módulos mediante extern)
// =============================================================================
GameData G;

Camera camera(glm::vec3(0.0f, EYE, 0.0f));
float  lastX      = SCR_WIDTH  / 2.0f;
float  lastY      = SCR_HEIGHT / 2.0f;
bool   firstMouse = true;
float  deltaTime  = 0.0f;
float  lastFrame  = 0.0f;

// =============================================================================
//  Prototipos de callbacks
// =============================================================================
void framebufferSizeCallback(GLFWwindow*, int w, int h);
void mouseCallback           (GLFWwindow*, double xI, double yI);
void scrollCallback          (GLFWwindow*, double, double yO);
void processInput            (GLFWwindow*);
void openGlInit              ();

// =============================================================================
//  Quad de fade (NDC, cubre toda la pantalla)
// =============================================================================
static unsigned int buildFadeQuad()
{
    float fv[] = {
        -1,-1,0, 0,0,1, 0,0,
         1,-1,0, 0,0,1, 1,0,
         1, 1,0, 0,0,1, 1,1,
         1, 1,0, 0,0,1, 1,1,
        -1, 1,0, 0,0,1, 0,1,
        -1,-1,0, 0,0,1, 0,0,
    };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fv), fv, GL_STATIC_DRAW);
    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);                    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));  glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));  glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return VAO;
}

// =============================================================================
//  Colores neutros del corredor (usados en todas las variantes)
// =============================================================================
static const glm::vec3 CWC = { 0.78f, 0.76f, 0.72f };
static const glm::vec3 CFC = { 0.48f, 0.46f, 0.43f };
static const glm::vec3 CCC = { 0.86f, 0.86f, 0.88f };

// =============================================================================
//  Dibuja una variante completa
// =============================================================================
static void drawVariant(unsigned int prog, int locModel, int locColor,
                        const RoomVariant& V, bool isActive)
{
    glm::vec3 wc, fc, cc;
    if (isActive) {
        wc = G.roomWall; fc = G.roomFloor; cc = G.roomCeil;
    } else {
        wc = { 0.82f,0.80f,0.75f }; fc = { 0.55f,0.52f,0.48f }; cc = { 0.90f,0.90f,0.92f };
    }

    drawRoom    (prog, locModel, locColor, V.room,      wc,  fc,  cc );
    drawCorridor(prog, locModel, locColor, V.corA,      CWC, CFC, CCC);
    drawCorridor(prog, locModel, locColor, V.corB,      CWC, CFC, CCC);
    drawCorridor(prog, locModel, locColor, V.corATurn,  CWC, CFC, CCC);
    drawCorridor(prog, locModel, locColor, V.corBTurn,  CWC, CFC, CCC);
    drawMesh    (prog, locModel, locColor, V.endWallA,    CWC);
    drawMesh    (prog, locModel, locColor, V.endWallB,    CWC);
    drawMesh    (prog, locModel, locColor, V.cornerWallA, CWC);
    drawMesh    (prog, locModel, locColor, V.cornerWallB, CWC);
}

// =============================================================================
//  MAIN
// =============================================================================
int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    // ── Inicialización GLFW ───────────────────────────────────────────────────
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

    // ── Shader ────────────────────────────────────────────────────────────────
    unsigned int shader  = setShaders("exit8.vert", "exit8.frag");
    int locModel  = glGetUniformLocation(shader, "uModel");
    int locView   = glGetUniformLocation(shader, "uView");
    int locProj   = glGetUniformLocation(shader, "uProjection");
    int locVP     = glGetUniformLocation(shader, "uViewPos");
    int locAmb    = glGetUniformLocation(shader, "uAmbient");
    int locUseTex = glGetUniformLocation(shader, "uUsarTextura");
    int locColor  = glGetUniformLocation(shader, "uColor");

    // ── Variantes ─────────────────────────────────────────────────────────────
    std::array<RoomVariant, NUM_VARIANTS> variants;
    variants[0] = buildVariant(0, AnomalyType::NONE);
    variants[1] = buildVariant(1, AnomalyType::RED_LIGHTS);

    // ── Quad de fade ──────────────────────────────────────────────────────────
    unsigned int fadeVAO = buildFadeQuad();

    // ── Estado inicial ────────────────────────────────────────────────────────
    G.currentVariant = rand() % NUM_VARIANTS;
    applyAnomalyColors(variants[G.currentVariant].anomaly);
    activateLights(variants[G.currentVariant]);
    G.entryDir = EntryDir::FROM_A;
    spawnAtCorridor(variants[G.currentVariant], true);

    std::cout << "=========================================\n"
              << "  THE EXIT 8\n"
              << "  WASD + raton para moverte\n"
              << "  Anomalia: VUELVE por donde viniste.\n"
              << "  Normal:   SIGUE adelante.\n"
              << "=========================================\n"
              << "[NIVEL 1] Anomalia: "
              << (G.currentAnomaly == AnomalyType::NONE ? "ninguna" : "LUCES ROJAS") << "\n";

    // ── Bucle principal ───────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float now = static_cast<float>(glfwGetTime());
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);
        applyCollisions(camera.Position, variants[G.currentVariant]);

        // ── Triggers de teleport ──────────────────────────────────────────────
        if (G.state == GameState::PLAYING)
        {
            const RoomVariant& V  = variants[G.currentVariant];
            float camZ = camera.Position.z;
            float camX = camera.Position.x;

            bool inTurnA = (camZ < V.origin.z - 13.0f) && (camZ > V.origin.z - 16.0f);
            bool inTurnB = (camZ > V.origin.z + 13.0f) && (camZ < V.origin.z + 16.0f);

            float trigA_X = V.origin.x + 5.0f;
            float trigB_X = V.origin.x - 5.0f;

            if (inTurnA && camX > trigA_X) {
                bool isRetreating = (G.entryDir == EntryDir::FROM_A);
                bool anomaly      = (G.currentAnomaly != AnomalyType::NONE);
                bool correct      = isRetreating ? anomaly : !anomaly;

                if (correct) G.score++; else G.score = 0;
                G.highScore = std::max(G.score, G.highScore);

                triggerTeleport(nextVariantIdx(G.currentVariant), false, correct);

            } else if (inTurnB && camX < trigB_X) {
                bool isRetreating = (G.entryDir == EntryDir::FROM_B);
                bool anomaly      = (G.currentAnomaly != AnomalyType::NONE);
                bool correct      = isRetreating ? anomaly : !anomaly;

                if (correct) G.score++; else G.score = 0;
                G.highScore = std::max(G.score, G.highScore);

                triggerTeleport(nextVariantIdx(G.currentVariant), true, correct);
            }
        }

        // ── Actualizar fade ───────────────────────────────────────────────────
        if (G.state == GameState::FADING_OUT)
        {
            G.fadeAlpha += G.FADE_SPEED * deltaTime;
            if (G.fadeAlpha >= 1.0f) {
                G.fadeAlpha = 1.0f;
                if (G.pending.active) {
                    G.currentVariant = G.pending.toVariant;
                    applyAnomalyColors(variants[G.currentVariant].anomaly);
                    activateLights(variants[G.currentVariant]);
                    spawnAtCorridor(variants[G.currentVariant], G.pending.toCorA);
                    G.entryDir       = G.pending.toCorA ? EntryDir::FROM_A : EntryDir::FROM_B;
                    G.pending.active = false;
                    std::cout << "[NIVEL " << G.score + 1 << "] Anomalia: "
                              << (G.currentAnomaly == AnomalyType::NONE ? "ninguna" : "LUCES ROJAS")
                              << "\n";
                }
                G.state = GameState::FADING_IN;
            }
        }
        else if (G.state == GameState::FADING_IN)
        {
            G.fadeAlpha -= G.FADE_SPEED * deltaTime;
            if (G.fadeAlpha <= 0.0f) {
                G.fadeAlpha = 0.0f;
                G.state     = GameState::PLAYING;
            }
        }

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 proj = camera.GetProjectionMatrix((float)SCR_WIDTH, (float)SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(locVP,  1, glm::value_ptr(camera.Position));
        glUniform3f (locAmb, G.ambient, G.ambient, G.ambient * 1.05f);
        glUniform1i (locUseTex, 0);
        uploadLights(shader);

        for (int i = 0; i < NUM_VARIANTS; i++)
            drawVariant(shader, locModel, locColor, variants[i], i == G.currentVariant);

        glBindVertexArray(0);

        // ── Fade negro ────────────────────────────────────────────────────────
        if (G.fadeAlpha > 0.001f)
        {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendColor(0, 0, 0, G.fadeAlpha);
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

            glm::mat4 I     = glm::mat4(1.f);
            glm::mat4 ortho = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
            glUniformMatrix4fv(locView,  1, GL_FALSE, glm::value_ptr(I));
            glUniformMatrix4fv(locProj,  1, GL_FALSE, glm::value_ptr(ortho));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(I));
            glUniform3f(locAmb, 0, 0, 0);
            glUniform1i(glGetUniformLocation(shader, "uNumLights"), 0);
            glUniform3f(locColor, 0, 0, 0);

            glBindVertexArray(fadeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_DEPTH_TEST);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// =============================================================================
//  Callbacks
// =============================================================================
void framebufferSizeCallback(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);
}

void mouseCallback(GLFWwindow*, double xI, double yI)
{
    float x = static_cast<float>(xI);
    float y = static_cast<float>(yI);
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    camera.ProcessMouseMovement(x - lastX, lastY - y);
    lastX = x; lastY = y;
}

void scrollCallback(GLFWwindow*, double, double yO)
{
    camera.Fov = glm::clamp(camera.Fov - (float)yO, 20.f, 90.f);
}

void processInput(GLFWwindow* w)
{
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
    if (glfwGetKey(w, GLFW_KEY_W)      == GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::FORWARD,  deltaTime);
    if (glfwGetKey(w, GLFW_KEY_S)      == GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_A)      == GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::LEFT,     deltaTime);
    if (glfwGetKey(w, GLFW_KEY_D)      == GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::RIGHT,    deltaTime);
}

void openGlInit()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 1);
}
