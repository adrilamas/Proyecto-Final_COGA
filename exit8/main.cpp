//  main.cpp  ─  Exit 8 | Motor Base
//  Perfil A: "El Arquitecto del Motor"
//  Contenido: ventana GLFW, contexto OpenGL 3.3, z-buffer, camara FPS,
//             bucle principal con DeltaTime, suelo de prueba.
// ===========================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <BibliotecasCurso/lecturaShader.h>
#include <iostream>
#include "Camera.h"

//---------DEBUGGEO--------------
#include <filesystem>
#include <fstream>
//-----------------------------------



//Shader
unsigned int shaderProgram;

// ── Configuracion de ventana ─────────────────────────────────────────────────
static constexpr int   SCR_WIDTH  = 1280;
static constexpr int   SCR_HEIGHT = 720;
static constexpr char  TITLE[]    = "The Exit 8 - Motor Base";

// ── Camara global ────────────────────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 1.7f, 0.0f));

// ── Estado del raton ─────────────────────────────────────────────────────────
float lastX      = SCR_WIDTH  / 2.0f;
float lastY      = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;

// ── DeltaTime ────────────────────────────────────────────────────────────────
float deltaTime = 0.0f; // tiempo entre el frame actual y el anterior
float lastFrame = 0.0f;

// ── Prototipos de callbacks ───────────────────────────────────────────────────
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback          (GLFWwindow* window, double xPos, double yPos);
void scrollCallback         (GLFWwindow* window, double xOffset, double yOffset);
void processInput           (GLFWwindow* window);

static void openGlInit();

// ── Geometria del suelo ───────────────────────────────────────────────────────
// Un plano grande hecho de dos triangulos con coordenadas UV para el tablero
static float floorVertices[] = {
    // posicion (XYZ)        // normal (XYZ)        // UV
    -50.0f, 0.0f, -50.0f,   0.0f, 1.0f, 0.0f,    0.0f,  50.0f,
     50.0f, 0.0f, -50.0f,   0.0f, 1.0f, 0.0f,   50.0f,  50.0f,
     50.0f, 0.0f,  50.0f,   0.0f, 1.0f, 0.0f,   50.0f,   0.0f,

     50.0f, 0.0f,  50.0f,   0.0f, 1.0f, 0.0f,   50.0f,   0.0f,
    -50.0f, 0.0f,  50.0f,   0.0f, 1.0f, 0.0f,    0.0f,   0.0f,
    -50.0f, 0.0f, -50.0f,   0.0f, 1.0f, 0.0f,    0.0f,  50.0f,
};

struct ShaderLocs {
    int model;
    int view;
    int proj;
    int color;
};

ShaderLocs getLocs(unsigned int program) {
    ShaderLocs locs;
    // Buscamos exactamente los nombres que pusiste en el código GLSL
    locs.model = glGetUniformLocation(program, "uModel");
    locs.view = glGetUniformLocation(program, "uView");
    locs.proj = glGetUniformLocation(program, "uProjection");
    locs.color = glGetUniformLocation(program, "uColor");
    return locs;
}

// ── main ─────────────────────────────────────────────────────────────────────
int main()  {

    // =========================================================================
        // ── ZONA DE DEBUGGING DE ARCHIVOS ────────────────────────────────────────
        // =========================================================================
    std::cout << "\n=== INICIANDO DEBUG DE SHADERS ===\n";

    // 1. ¿Dónde cree el programa que está parado?
    std::cout << "1. El programa se esta ejecutando desde la carpeta:\n   "
        << std::filesystem::current_path() << "\n\n";

    // 2. Vamos a intentar abrir el archivo "a mano" sin usar tu librería
    std::ifstream vertexTest("exit8.vert");
    std::cout << "2. Buscando 'exit8.vert' en esa carpeta...\n";

    if (vertexTest.is_open()) {
        std::cout << "   -> EXITO: El archivo exit8.vert EXISTE y se puede leer.\n";
        vertexTest.close();
    }
    else {
        std::cout << "   -> ERROR CRITICO: El programa es incapaz de encontrar 'exit8.vert' aqui.\n";
    }

    std::cout << "==================================\n\n";
    // =========================================================================

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sistema Solar", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error al crear la ventana\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    // Bloquear el cursor en el centro
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error al inicializar GLAD\n";
        return -1;
    }

    openGlInit();

    // ── 6. Preparar VAO/VBO del suelo ─────────────────────────────────────────
    unsigned int floorVAO, floorVBO;

    glGenVertexArrays(1, &floorVAO);
    glGenBuffers     (1, &floorVBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    // Stride = 8 floats por vertice (3 pos + 3 normal + 2 UV)
    int stride = 8 * sizeof(float);

    // Atributo 0: posicion (3 floats, offset 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Atributo 1: normal (3 floats, offset 3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Atributo 2: UV (2 floats, offset 6 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    shaderProgram = setShaders("exit8.vert", "exit8.frag");
    ShaderLocs L1 = getLocs(shaderProgram);

    // ── 7. Bucle principal ────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        // ── DeltaTime: tiempo entre frames ────────────────────────────────────
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        /*
        // Mostrar FPS en la barra de titulo cada segundo
        static float fpsTimer = 0.0f;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f)
        {
            int fps = static_cast<int>(1.0f / deltaTime);
            std::string newTitle = std::string(TITLE) + "  |  FPS: " + std::to_string(fps);
            glfwSetWindowTitle(window, newTitle.c_str());
            fpsTimer = 0.0f;
        }*/

        // ── Input de teclado ───────────────────────────────────────────────────
        processInput(window);

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculamos las matrices con la posición actual de la cámara
        glm::mat4 projection = camera.GetProjectionMatrix((float)SCR_WIDTH, (float)SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        // --- DIBUJAR SUELO ---
        // Decimos: "Usa este programa"
        glUseProgram(shaderProgram);

        // Enviamos los datos a las posiciones que guardamos en L1
        // Nota: Usamos L1.proj porque en tu Vertex Shader se llama uProjection
        glUniformMatrix4fv(L1.model, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(L1.view, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(L1.proj, 1, GL_FALSE, glm::value_ptr(projection));

        // El color gris azulado para el suelo
        glUniform3f(L1.color, 0.45f, 0.45f, 0.50f);

        // Dibujamos
        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // ── Intercambiar buffers ────────────────────────────────────────────
        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    // ── 8. Limpieza ───────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers     (1, &floorVBO);
    glfwTerminate();
    return 0;
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

// Redimension de ventana: actualiza el viewport
void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Movimiento del raton: calcula offsets y los manda a la camara
void mouseCallback(GLFWwindow* /*window*/, double xPosIn, double yPosIn)
{
    float xPos = static_cast<float>(xPosIn);
    float yPos = static_cast<float>(yPosIn);

    if (firstMouse)
    {
        lastX      = xPos;
        lastY      = yPos;
        firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos; // invertido: Y de pantalla va hacia abajo

    lastX = xPos;
    lastY = yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

// Scroll: ajusta el FOV (zoom optico)
void scrollCallback(GLFWwindow* /*window*/, double /*xOffset*/, double yOffset)
{
    camera.Fov -= static_cast<float>(yOffset);
    if (camera.Fov < 20.0f) camera.Fov = 20.0f;
    if (camera.Fov > 90.0f) camera.Fov = 90.0f;
}

// Teclado: WASD para moverse, ESC para salir
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
}

static void openGlInit() {
    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
}