// =============================================================================
//  main.cpp  ─  The Exit 8
//  · Teleport INSTANTÁNEO (sin fade)
//  · Tres texturas tileable: paredes, suelo, techo
//  · Carga con stb_image (coloca stb_image.h en la carpeta del proyecto)
// =============================================================================
//
//  ARCHIVOS DE TEXTURA ESPERADOS (junto al ejecutable):
//    tex_wall.jpg   – textura tileable para paredes
//    tex_floor.jpg  – textura tileable para suelo
//    tex_ceil.jpg   – textura tileable para techo
//
//  Si alguna falta, ese elemento se renderiza con el color sólido de siempre.
//
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

// stb_image – cabecera de una sola unidad de compilación
// Solo se define STB_IMAGE_IMPLEMENTATION una vez (aquí, en main.cpp).
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
//  Globales compartidos con otros módulos mediante extern
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
//  Carga de texturas
// =============================================================================
//
//  Parámetros GL para tileado sin costura:
//    · GL_REPEAT          → la textura se repite infinitamente
//    · GL_LINEAR_MIPMAP_LINEAR → trilineal, elimina el efecto de "sierra"
//      en las transiciones entre niveles de mipmap (aliasing a distancia)
//
//  Para que no se note el corte, hay que:
//    1. Usar una textura que sea tileable (bordes que encajan).
//    2. Escalar las UVs en buildQuad de forma proporcional al tamaño real
//       del quad (p. ej. 1 UV-unit = 1 metro de mundo), de modo que la
//       escala es coherente en toda la escena.
//
unsigned int loadTexture(const char* path)
{
    unsigned int texID;
    glGenTextures(1, &texID);

    int w, h, nChannels;
    // stb_image carga con origen en la esquina superior-izquierda;
    // OpenGL espera origen en la inferior-izquierda → volteamos.
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &nChannels, 0);

    if (data) {
        GLenum fmt = (nChannels == 4) ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Modo de repetición en ambos ejes
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Filtrado: trilineal (suaviza la transición entre mipmaps)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Anisotropía (extensión GL_EXT_texture_filter_anisotropic).
        // Usamos los valores numéricos directamente para no depender de que
        // el header los defina — son parte del estándar EXT, siempre 0x84FE/0x84FF.
        #ifndef GL_TEXTURE_MAX_ANISOTROPY
        #define GL_TEXTURE_MAX_ANISOTROPY     0x84FE
        #define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
        #endif
        if (glfwExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
            float maxAniso = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        }

        stbi_image_free(data);
        std::cout << "[TEX] Cargada: " << path
                  << "  (" << w << "x" << h << ", ch=" << nChannels << ")\n";
    } else {
        // Si la textura no existe, devolvemos 0 y el shader usará el color sólido.
        std::cerr << "[TEX] AVISO: no se pudo cargar '" << path
                  << "'. Se usará color sólido.\n";
        glDeleteTextures(1, &texID);
        texID = 0;
        stbi_image_free(data);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

// Helper: bindea una textura en la unidad 0 y activa o desactiva el sampler.
// Si texID == 0 (no cargada), desactiva el sampler → color sólido de fallback.
inline void bindTex(int locUseTex, unsigned int texID)
{
    if (texID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glUniform1i(locUseTex, 1);
    } else {
        glUniform1i(locUseTex, 0);
    }
}

// =============================================================================
//  Colores neutros del corredor
// =============================================================================
static const glm::vec3 CWC = { 0.78f, 0.76f, 0.72f };
static const glm::vec3 CFC = { 0.48f, 0.46f, 0.43f };
static const glm::vec3 CCC = { 0.86f, 0.86f, 0.88f };

// =============================================================================
//  Dibuja una variante completa con texturas
//
//  Estrategia de bind:
//    · drawRoom  llama a drawMesh internamente, que no sabe de texturas.
//      Por eso las dibujamos por partes: primero suelos de todas las rooms,
//      luego techos, luego paredes → cambiamos la textura una sola vez
//      por "tipo de superficie", no una vez por mesh.
//  
//  Esto minimiza los cambios de estado de OpenGL.
// =============================================================================
static void drawVariant(unsigned int prog, int locModel, int locColor, int locUseTex,
                        unsigned int texWall, unsigned int texFloor, unsigned int texCeil,
                        const RoomVariant& V, bool isActive)
{
    // Colores de anomalía para la habitación central; neutros para corredores.
    glm::vec3 roomWC = isActive ? G.roomWall  : glm::vec3{0.82f,0.80f,0.75f};
    glm::vec3 roomFC = isActive ? G.roomFloor : glm::vec3{0.55f,0.52f,0.48f};
    glm::vec3 roomCC = isActive ? G.roomCeil  : glm::vec3{0.90f,0.90f,0.92f};

    // ── SUELOS ────────────────────────────────────────────────────────────────
    // Color de fallback en caso de no tener textura
    glUniform3fv(locColor, 1, glm::value_ptr(roomFC));
    bindTex(locUseTex, texFloor);

    drawMesh(prog, locModel, locColor, V.room.floor,      roomFC);
    drawMesh(prog, locModel, locColor, V.corA.floor,      CFC);
    drawMesh(prog, locModel, locColor, V.corB.floor,      CFC);
    drawMesh(prog, locModel, locColor, V.corATurn.floor,  CFC);
    drawMesh(prog, locModel, locColor, V.corBTurn.floor,  CFC);

    // ── TECHOS ────────────────────────────────────────────────────────────────
    glUniform3fv(locColor, 1, glm::value_ptr(roomCC));
    bindTex(locUseTex, texCeil);

    drawMesh(prog, locModel, locColor, V.room.ceiling,      roomCC);
    drawMesh(prog, locModel, locColor, V.corA.ceiling,      CCC);
    drawMesh(prog, locModel, locColor, V.corB.ceiling,      CCC);
    drawMesh(prog, locModel, locColor, V.corATurn.ceiling,  CCC);
    drawMesh(prog, locModel, locColor, V.corBTurn.ceiling,  CCC);

    // ── PAREDES ───────────────────────────────────────────────────────────────
    glUniform3fv(locColor, 1, glm::value_ptr(roomWC));
    bindTex(locUseTex, texWall);

    // Habitación
    drawMesh(prog, locModel, locColor, V.room.wN, roomWC);
    drawMesh(prog, locModel, locColor, V.room.wS, roomWC);
    drawMesh(prog, locModel, locColor, V.room.wE, roomWC);
    drawMesh(prog, locModel, locColor, V.room.wW, roomWC);
    // Pasillos rectos
    drawMesh(prog, locModel, locColor, V.corA.wLeft,    CWC);
    drawMesh(prog, locModel, locColor, V.corA.wRight,   CWC);
    drawMesh(prog, locModel, locColor, V.corB.wLeft,    CWC);
    drawMesh(prog, locModel, locColor, V.corB.wRight,   CWC);
    // Codos
    drawMesh(prog, locModel, locColor, V.corATurn.wLeft,   CWC);
    drawMesh(prog, locModel, locColor, V.corATurn.wRight,  CWC);
    drawMesh(prog, locModel, locColor, V.corBTurn.wLeft,   CWC);
    drawMesh(prog, locModel, locColor, V.corBTurn.wRight,  CWC);
    // Paredes de cierre y esquinas
    drawMesh(prog, locModel, locColor, V.endWallA,    CWC);
    drawMesh(prog, locModel, locColor, V.endWallB,    CWC);
    drawMesh(prog, locModel, locColor, V.cornerWallA, CWC);
    drawMesh(prog, locModel, locColor, V.cornerWallB, CWC);
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
    unsigned int shader = setShaders("exit8.vert", "exit8.frag");
    int locModel  = glGetUniformLocation(shader, "uModel");
    int locView   = glGetUniformLocation(shader, "uView");
    int locProj   = glGetUniformLocation(shader, "uProjection");
    int locVP     = glGetUniformLocation(shader, "uViewPos");
    int locAmb    = glGetUniformLocation(shader, "uAmbient");
    int locUseTex = glGetUniformLocation(shader, "uUsarTextura");
    int locColor  = glGetUniformLocation(shader, "uColor");
    int locTex    = glGetUniformLocation(shader, "uTexBase");

    // El sampler siempre leerá de la unidad de textura 0
    glUseProgram(shader);
    glUniform1i(locTex, 0);

    // ── Texturas ──────────────────────────────────────────────────────────────
    //
    //  Convención de escala UV usada en buildQuad (ver Room.cpp):
    //    suelo/techo  → su = w/2,  sv = d/2   (1 tile ≈ 2 metros de mundo)
    //    paredes      → su = d/2,  sv = h/2
    //  Con GL_REPEAT la textura se repite automáticamente; no hay costuras si
    //  la imagen es tileable (sus bordes opuestos encajan).
    //
    unsigned int texWall  = loadTexture("tex_wall.jpg");
    unsigned int texFloor = loadTexture("tex_floor.jpg");
    unsigned int texCeil  = loadTexture("tex_ceil.jpg");

    // ── Variantes ─────────────────────────────────────────────────────────────
    std::array<RoomVariant, NUM_VARIANTS> variants;
    variants[0] = buildVariant(0, AnomalyType::NONE);

    //variants[1] = buildVariant(1, AnomalyType::NONE);
    variants[1] = buildVariant(1, AnomalyType::RED_LIGHTS);

    // ── Estado inicial ────────────────────────────────────────────────────────
    G.currentVariant = rand() % NUM_VARIANTS;
    applyAnomalyColors(variants[G.currentVariant].anomaly);
    activateLights(variants[G.currentVariant]);
    G.entryDir = EntryDir::FROM_A;

    // Calculamos el trigger a la MITAD EXACTA DEL PASILLO
    float init_cor_len = CL - CW;
    float init_triggerDist = (RD / 2.0f) + (init_cor_len / 2.0f);

    // Spawn inicial modificado
    glm::vec3 fakePos = variants[G.currentVariant].origin + glm::vec3(-RW / 2.f + CW / 2.f, EYE, init_triggerDist);
    spawnAtCorridor(variants[G.currentVariant], variants[G.currentVariant], true, fakePos, init_triggerDist);


    std::cout << "=========================================\n"
              << "  THE EXIT 8\n"
              << "  WASD + raton para moverte\n"
              << "  Anomalia: VUELVE por donde viniste.\n"
              << "  Normal:   SIGUE adelante.\n"
              << "=========================================\n"
              << "[NIVEL 1] Anomalia: "
              << (G.currentAnomaly == AnomalyType::NONE ? "ninguna" : "LUCES ROJAS") << "\n";

    // ── Bucle principal
    while (!glfwWindowShouldClose(window))
    {
        float now = static_cast<float>(glfwGetTime());
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);
        applyCollisions(camera.Position, variants[G.currentVariant]);

        // ── Triggers de teleport ──────────────────────────────────────────────
        // El teleport es INSTANTÁNEO: no hay fade.
        // Se hace directamente en el mismo frame en que se cruza el umbral.
        if (G.state == GameState::PLAYING)
        {
            const RoomVariant& V = variants[G.currentVariant];
            float camZ = camera.Position.z;

            // Calcula el umbral de trigger igual que spawnAtCorridor
            float cor_len     = CL - CW;
            float zDistNear   = (RD / 2.0f) + cor_len;
            float triggerDist = zDistNear - 3.5f;   // 3.5 u antes del codo

            // ─ Trigger Norte (Z+) ─────────────────────────────────────────────
            if (camZ > V.origin.z + triggerDist)
            {
                bool isRetreating = (G.entryDir == EntryDir::FROM_B);
                bool anomaly      = (G.currentAnomaly != AnomalyType::NONE);
                bool correct      = isRetreating ? anomaly : !anomaly;

                if (correct) G.score++; else G.score = 0;
                if (G.score > G.highScore) G.highScore = G.score;

                std::cout << (correct ? "[OK]   " : "[FAIL] ")
                          << (isRetreating ? "Retrocede" : "Avanza")
                          << " por Norte. Anomalia:" << (anomaly ? "SI" : "NO")
                          << "  Racha:" << G.score
                          << "  Record:" << G.highScore << "\n";

                // Cambiar a la siguiente variante y spawnear en el lado opuesto
                int nextVar = nextVariantIdx(G.currentVariant);
                G.currentVariant = nextVar;
                applyAnomalyColors(variants[G.currentVariant].anomaly);
                activateLights(variants[G.currentVariant]);
                // Salir por Norte → entrar por Sur (corA) en la nueva variante
                spawnAtCorridor(variants[G.currentVariant], true);
                G.entryDir = EntryDir::FROM_A;

                std::cout << "[NIVEL " << G.score + 1 << "] Anomalia: "
                          << (G.currentAnomaly == AnomalyType::NONE ? "ninguna" : "LUCES ROJAS")
                          << "\n";
            }
            // ─ Trigger Sur (Z-) ───────────────────────────────────────────────
            else if (camZ < V.origin.z - triggerDist)
            {
                bool isRetreating = (G.entryDir == EntryDir::FROM_A);
                bool anomaly      = (G.currentAnomaly != AnomalyType::NONE);
                bool correct      = isRetreating ? anomaly : !anomaly;

                if (correct) G.score++; else G.score = 0;
                if (G.score > G.highScore) G.highScore = G.score;

                std::cout << (correct ? "[OK]   " : "[FAIL] ")
                          << (isRetreating ? "Retrocede" : "Avanza")
                          << " por Sur. Anomalia:" << (anomaly ? "SI" : "NO")
                          << "  Racha:" << G.score
                          << "  Record:" << G.highScore << "\n";

                int nextVar = nextVariantIdx(G.currentVariant);
                G.currentVariant = nextVar;
                applyAnomalyColors(variants[G.currentVariant].anomaly);
                activateLights(variants[G.currentVariant]);
                // Salir por Sur → entrar por Norte (corB) en la nueva variante
                spawnAtCorridor(variants[G.currentVariant], false);
                G.entryDir = EntryDir::FROM_B;

                std::cout << "[NIVEL " << G.score + 1 << "] Anomalia: "
                          << (G.currentAnomaly == AnomalyType::NONE ? "ninguna" : "LUCES ROJAS")
                          << "\n";
            }
        }

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);   // limpieza, buena práctica

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Limpieza ──────────────────────────────────────────────────────────────
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}
