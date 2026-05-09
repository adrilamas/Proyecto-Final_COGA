// =============================================================================
//  main.cpp  ─  The Exit 8
//  · Pasillo en forma de Z con dos giros antes del trigger
//  · Anomalías aleatorias y extensibles
//  · Teleport instantáneo
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

#define STB_IMAGE_IMPLEMENTATION
#include <BibliotecasCurso/stb_image.h>

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
//  Globales
// =============================================================================
GameData G;
Camera   camera(glm::vec3(0.f, EYE, 0.f));
float    lastX      = SCR_WIDTH  / 2.f;
float    lastY      = SCR_HEIGHT / 2.f;
bool     firstMouse = true;
float    deltaTime  = 0.f;
float    lastFrame  = 0.f;

// =============================================================================
//  Prototipos
// =============================================================================
void framebufferSizeCallback(GLFWwindow*, int w, int h);
void mouseCallback           (GLFWwindow*, double x, double y);
void scrollCallback          (GLFWwindow*, double, double yO);
void processInput            (GLFWwindow*);
void openGlInit              ();

// =============================================================================
//  Carga de textura
// =============================================================================
unsigned int loadTexture(const char* path)
{
    unsigned int texID;
    glGenTextures(1, &texID);

    int w, h, nCh;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &nCh, 0);
    if (data) {
        GLenum fmt = (nCh == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        #ifndef GL_TEXTURE_MAX_ANISOTROPY
        #define GL_TEXTURE_MAX_ANISOTROPY     0x84FE
        #define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
        #endif
        if (glfwExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
            float maxAniso = 1.f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        }
        stbi_image_free(data);
        std::cout << "[TEX] " << path << " (" << w << "x" << h << ")\n";
    } else {
        std::cerr << "[TEX] No encontrada: " << path << " – usando color sólido\n";
        glDeleteTextures(1, &texID);
        texID = 0;
        stbi_image_free(data);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

inline void bindTex(int locUseTex, unsigned int texID)
{
    if (texID) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(locUseTex, 1);
    } else {
        glUniform1i(locUseTex, 0);
    }
}

// =============================================================================
//  Colores neutros de corredor
// =============================================================================
static const glm::vec3 CWC = { 0.78f, 0.76f, 0.72f };
static const glm::vec3 CFC = { 0.48f, 0.46f, 0.43f };
static const glm::vec3 CCC = { 0.86f, 0.86f, 0.88f };

// =============================================================================
//  drawVariant – dibuja toda la geometría de una variante
// =============================================================================
static void drawVariant(unsigned int prog, int locModel, int locColor, int locUseTex,
                        unsigned int texWall, unsigned int texFloor, unsigned int texCeil,
                        const RoomVariant& V, bool isActive)
{
    glm::vec3 roomWC = isActive ? G.roomWall  : glm::vec3{0.82f,0.80f,0.75f};
    glm::vec3 roomFC = isActive ? G.roomFloor : glm::vec3{0.55f,0.52f,0.48f};
    glm::vec3 roomCC = isActive ? G.roomCeil  : glm::vec3{0.90f,0.90f,0.92f};

    // ── SUELOS ────────────────────────────────────────────────────────────────
    bindTex(locUseTex, texFloor);
    drawMesh(prog, locModel, locColor, V.room.floor,      roomFC);
    // Tramos rectos A
    drawMesh(prog, locModel, locColor, V.corA1.floor,     CFC);
    drawMesh(prog, locModel, locColor, V.corA2.floor,     CFC);
    drawMesh(prog, locModel, locColor, V.corA3.floor,     CFC);
    // Codos A
    drawMesh(prog, locModel, locColor, V.elbowA1floor,    CFC);
    drawMesh(prog, locModel, locColor, V.elbowA2floor,    CFC);
    // Tramos rectos B
    drawMesh(prog, locModel, locColor, V.corB1.floor,     CFC);
    drawMesh(prog, locModel, locColor, V.corB2.floor,     CFC);
    drawMesh(prog, locModel, locColor, V.corB3.floor,     CFC);
    // Codos B
    drawMesh(prog, locModel, locColor, V.elbowB1floor,    CFC);
    drawMesh(prog, locModel, locColor, V.elbowB2floor,    CFC);

    // ── TECHOS ────────────────────────────────────────────────────────────────
    bindTex(locUseTex, texCeil);
    drawMesh(prog, locModel, locColor, V.room.ceiling,    roomCC);
    drawMesh(prog, locModel, locColor, V.corA1.ceiling,   CCC);
    drawMesh(prog, locModel, locColor, V.corA2.ceiling,   CCC);
    drawMesh(prog, locModel, locColor, V.corA3.ceiling,   CCC);
    drawMesh(prog, locModel, locColor, V.elbowA1ceil,     CCC);
    drawMesh(prog, locModel, locColor, V.elbowA2ceil,     CCC);
    drawMesh(prog, locModel, locColor, V.corB1.ceiling,   CCC);
    drawMesh(prog, locModel, locColor, V.corB2.ceiling,   CCC);
    drawMesh(prog, locModel, locColor, V.corB3.ceiling,   CCC);
    drawMesh(prog, locModel, locColor, V.elbowB1ceil,     CCC);
    drawMesh(prog, locModel, locColor, V.elbowB2ceil,     CCC);

    // ── PAREDES ───────────────────────────────────────────────────────────────
    bindTex(locUseTex, texWall);
    // Sala
    drawMesh(prog, locModel, locColor, V.room.wN,         roomWC);
    drawMesh(prog, locModel, locColor, V.room.wS,         roomWC);
    drawMesh(prog, locModel, locColor, V.room.wE,         roomWC);
    drawMesh(prog, locModel, locColor, V.room.wW,         roomWC);
    // Corredor A1
    drawMesh(prog, locModel, locColor, V.corA1.wLeft,     CWC);
    drawMesh(prog, locModel, locColor, V.corA1.wRight,    CWC);
    // Codo A1
    drawMesh(prog, locModel, locColor, V.elbowA1wallOuter, CWC);
    drawMesh(prog, locModel, locColor, V.elbowA1wallInner, CWC);
    // Corredor A2
    drawMesh(prog, locModel, locColor, V.corA2.wLeft,     CWC);
    drawMesh(prog, locModel, locColor, V.corA2.wRight,    CWC);
    // Codo A2
    drawMesh(prog, locModel, locColor, V.elbowA2wallOuter, CWC);
    drawMesh(prog, locModel, locColor, V.elbowA2wallInner, CWC);
    // Corredor A3
    drawMesh(prog, locModel, locColor, V.corA3.wLeft,     CWC);
    drawMesh(prog, locModel, locColor, V.corA3.wRight,    CWC);
    drawMesh(prog, locModel, locColor, V.endWallA,        CWC);
    // Corredor B1
    drawMesh(prog, locModel, locColor, V.corB1.wLeft,     CWC);
    drawMesh(prog, locModel, locColor, V.corB1.wRight,    CWC);
    // Codo B1
    drawMesh(prog, locModel, locColor, V.elbowB1wallOuter, CWC);
    drawMesh(prog, locModel, locColor, V.elbowB1wallInner, CWC);
    // Corredor B2
    drawMesh(prog, locModel, locColor, V.corB2.wLeft,     CWC);
    drawMesh(prog, locModel, locColor, V.corB2.wRight,    CWC);
    // Codo B2
    drawMesh(prog, locModel, locColor, V.elbowB2wallOuter, CWC);
    drawMesh(prog, locModel, locColor, V.elbowB2wallInner, CWC);
    // Corredor B3
    drawMesh(prog, locModel, locColor, V.corB3.wLeft,     CWC);
    drawMesh(prog, locModel, locColor, V.corB3.wRight,    CWC);
    drawMesh(prog, locModel, locColor, V.endWallB,        CWC);
}

// =============================================================================
//  MAIN
// =============================================================================
int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

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
    unsigned int shader = setShaders("exit8.vert", "exit8.frag");
    int locModel  = glGetUniformLocation(shader, "uModel");
    int locView   = glGetUniformLocation(shader, "uView");
    int locProj   = glGetUniformLocation(shader, "uProjection");
    int locVP     = glGetUniformLocation(shader, "uViewPos");
    int locAmb    = glGetUniformLocation(shader, "uAmbient");
    int locUseTex = glGetUniformLocation(shader, "uUsarTextura");
    int locColor  = glGetUniformLocation(shader, "uColor");
    int locTex    = glGetUniformLocation(shader, "uTexBase");

    // ── Shader de Efecto de Fallo (Glitch) ────────────────────────────────────
    unsigned int staticShader = setShaders("static.vert", "static.frag");
    int locStatTime  = glGetUniformLocation(staticShader, "uTime");
    int locStatAlpha = glGetUniformLocation(staticShader, "uAlpha");

    // Necesitamos un VAO vacío para usar el truco del triángulo de pantalla completa
    unsigned int emptyVAO;
    glGenVertexArrays(1, &emptyVAO);

    float failTimer = 0.0f; // Temporizador para el desvanecimiento del efecto

    // Carga del Shader de Victoria
    unsigned int winShader = setShaders("static.vert", "win.frag"); // Reusa el .vert
    int locWinTime  = glGetUniformLocation(winShader, "uTime");
    int locWinAlpha = glGetUniformLocation(winShader, "uAlpha");

    float winTimer = 0.0f; // Nuevo temporizador

    glUseProgram(shader);
    glUniform1i(locTex, 0);

    // ── Texturas ──────────────────────────────────────────────────────────────
    unsigned int texWall  = loadTexture("tex_baldosa.jpg");
    unsigned int texFloor = loadTexture("tex_baldosa.jpg");
    unsigned int texCeil  = loadTexture("tex_baldosa.jpg");

    // ── Variantes ─────────────────────────────────────────────────────────────
    // Variante 0: siempre empieza sin anomalía para el tutorial implícito
    // Variante 1: anomalía aleatoria
    std::array<RoomVariant, NUM_VARIANTS> variants;
    variants[0] = buildVariant(0, AnomalyType::NONE);
    variants[1] = buildVariant(1, randomAnomaly());

    // ── Estado inicial ────────────────────────────────────────────────────────
    G.currentVariant = 0;
    applyAnomalyColors(variants[G.currentVariant].anomaly);
    activateLights(variants[G.currentVariant]);
    G.entryDir = EntryDir::FROM_A;

    // Spawn: al inicio del tramo A3 (el jugador acaba de cruzar el trigger
    // ficticio y está a punto de entrar en la sala por primera vez).
    spawnAtCorridor(variants[G.currentVariant], true);

    std::cout << "=========================================\n"
              << "  THE EXIT 8\n"
              << "  WASD + ratón para moverte\n"
              << "  Anomalía: VUELVE por donde viniste.\n"
              << "  Normal:   SIGUE adelante.\n"
              << "=========================================\n"
              << "[NIVEL 1] Anomalía: "
              << (G.currentAnomaly == AnomalyType::NONE ? "ninguna" : "LUCES ROJAS")
              << "\n";

    // ── Bucle principal ────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float now  = static_cast<float>(glfwGetTime());
        deltaTime  = now - lastFrame;
        lastFrame  = now;

        processInput(window);
        applyCollisions(camera.Position, variants[G.currentVariant]);

        // ── Triggers ──────────────────────────────────────────────────────────
        // El trigger se activa en la mitad del tramo 3 del pasillo activo.
        // Geometría:
        //   Tramo A3 center Z = corA3Center.z
        //   El trigger está a corA3Center.z + CL3/2 (el extremo más cercano a A1)
        //     pero la cámara viene desde ese extremo, así que el trigger es
        //     cuando Z < corA3Center.z – CL3/2 + margen (o sea, pasada la mitad)
        //
        // Simplificado: usamos triggerDist guardado en V para determinar
        // si el jugador ha cruzado el punto medio del tramo 3.

        if (G.state == GameState::PLAYING)
        {
            const RoomVariant& currentV = variants[G.currentVariant];
            float trigX_A = currentV.corA2Center.x - (CL2 / 2.f) + 1.5f;
            float trigX_B = currentV.corB2Center.x + (CL2 / 2.f) - 1.5f;
            float camX = camera.Position.x;

            bool isAdvance = (G.entryDir == EntryDir::FROM_A && camX > trigX_B) || 
                             (G.entryDir == EntryDir::FROM_B && camX < trigX_A);
            
            bool isRetreat = (G.entryDir == EntryDir::FROM_A && camX < trigX_A) || 
                             (G.entryDir == EntryDir::FROM_B && camX > trigX_B);

            if (isAdvance || isRetreat)
            {
                bool hasAnomaly = (currentV.anomaly != AnomalyType::NONE);
                bool correct = (isAdvance && !hasAnomaly) || (isRetreat && hasAnomaly);

                if (correct) {
                    G.score++;
                    if (G.score > G.highScore) G.highScore = G.score;
                    
                    if (G.score > 8) {
                        std::cout << "¡VICTORIA! Has escapado del bucle.\n";
                        winTimer = 2.5f; // Efecto verde largo
                        G.score = 0;     // Reinicio tras ganar
                    } else {
                        std::cout << "[OK] Avanzas al Nivel " << G.score << "\n";
                    }
                } else {
                    G.score = 0;
                    std::cout << "[FAIL] Error en la simulación. Reiniciando...\n";
                    failTimer = 1.5f; // Efecto rojo
                }

                // SIEMPRE generamos una sala nueva. 
                // El nivel 0 (score 0) nunca tiene anomalías.
                AnomalyType nextAnomaly = (G.score == 0) ? AnomalyType::NONE : randomAnomaly();

                int nextVarIdx = nextVariantIdx(G.currentVariant);
                variants[nextVarIdx] = buildVariant(nextVarIdx, nextAnomaly);

                // TP relativo (Mantenemos tu lógica de inercias anterior)
                bool toA = (camX > trigX_B);
                const RoomVariant& nextV = variants[nextVarIdx];
                if (toA) {
                    float overX = camX - trigX_B;
                    camera.Position.x = nextV.corA2Center.x + (CL2 / 2.f) - 1.5f + overX;
                    camera.Position.z = nextV.corA2Center.z + (camera.Position.z - currentV.corB2Center.z);
                } else {
                    float overX = camX - trigX_A;
                    camera.Position.x = nextV.corB2Center.x - (CL2 / 2.f) + 1.5f + overX;
                    camera.Position.z = nextV.corB2Center.z + (camera.Position.z - currentV.corA2Center.z);
                }

                G.currentVariant = nextVarIdx;
                G.entryDir = toA ? EntryDir::FROM_A : EntryDir::FROM_B;
                applyAnomalyColors(variants[G.currentVariant].anomaly);
                activateLights(variants[G.currentVariant]);
            }
        }

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 proj = camera.GetProjectionMatrix((float)SCR_WIDTH, (float)SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(locVP,  1, glm::value_ptr(camera.Position));
        glUniform3f (locAmb, G.ambient, G.ambient, G.ambient * 1.05f);
        uploadLights(shader);

        for (int i = 0; i < NUM_VARIANTS; i++)
            drawVariant(shader, locModel, locColor, locUseTex,
                        texWall, texFloor, texCeil,
                        variants[i], i == G.currentVariant);

        // ── Render Efecto Estática (Post-Procesado) ───────────────────────────
        if (failTimer > 0.0f)
        {
            failTimer -= deltaTime;
            if (failTimer < 0.0f) failTimer = 0.0f;

            glUseProgram(staticShader);
            glUniform1f(locStatTime, (float)glfwGetTime());
            // Calculamos el alpha para que se desvanezca (1.0 -> 0.0)
            glUniform1f(locStatAlpha, failTimer / 1.5f);

            // Desactivamos el test de profundidad para que se dibuje por encima de todo
            glDisable(GL_DEPTH_TEST);
            
            glBindVertexArray(emptyVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3); // Dibuja la pantalla completa
            
            // Lo volvemos a activar para el siguiente frame
            glEnable(GL_DEPTH_TEST);
        }

        // ── Render Efecto Victoria (Verde) ────────────────────────────────────
        if (winTimer > 0.0f)
        {
            winTimer -= deltaTime;
            glUseProgram(winShader);
            glUniform1f(locWinTime, (float)glfwGetTime());
            glUniform1f(locWinAlpha, winTimer / 2.5f);
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(emptyVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glEnable(GL_DEPTH_TEST);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (texWall)  glDeleteTextures(1, &texWall);
    if (texFloor) glDeleteTextures(1, &texFloor);
    if (texCeil)  glDeleteTextures(1, &texCeil);
    glfwTerminate();
    return 0;
}

// =============================================================================
//  Callbacks
// =============================================================================
void framebufferSizeCallback(GLFWwindow*, int w, int h)
{ glViewport(0, 0, w, h); }

void mouseCallback(GLFWwindow*, double xI, double yI)
{
    float x = (float)xI, y = (float)yI;
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    camera.ProcessMouseMovement(x - lastX, lastY - y);
    lastX = x; lastY = y;
}

void scrollCallback(GLFWwindow*, double, double yO)
{ camera.Fov = glm::clamp(camera.Fov - (float)yO, 20.f, 90.f); }

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
    glClearColor(0.f, 0.f, 0.f, 1.f);
}
