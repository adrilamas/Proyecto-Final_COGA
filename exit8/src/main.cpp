// =============================================================================
//  main.cpp  ─  Exit 8 | Variantes + Teleport entre pasillos
// =============================================================================
//
//  DISEÑO DEL MUNDO
//  ─────────────────────────────────────────────────────────────────────────────
//  Hay N variantes de habitación. Cada variante ocupa el mismo layout pero
//  desplazada en X por VARIANT_STRIDE unidades, de modo que el frustum nunca
//  puede ver dos variantes a la vez.
//
//  Layout de UNA variante (vista desde arriba, origen = centro de la habitación):
//
//       ←  pasillo_A (eje -Z, esquina oeste)
//  ┌──────────────────────┐
//  │     HABITACIÓN       │
//  │  ╔══╗                │
//  │  ║ A║ ← apertura SW  │
//  │  ╚══╝                │
//  │              ╔══╗    │
//  │ apertura NE → ║B║    │
//  │              ╚══╝    │
//  └──────────────────────┘
//       pasillo_B (eje +Z, esquina este) →
//
//  Pasillo A: sale de la esquina SW  → Z decrece (−Z)
//  Pasillo B: sale de la esquina NE  → Z crece   (+Z)
//
//  Los dos pasillos están desplazados en X opuestamente para que al mirar
//  desde uno no se vea el otro a través de la habitación (efecto Z).
//
//  TELEPORT
//  ─────────────────────────────────────────────────────────────────────────────
//  Trigger A  Z < variantOrigin.z - RD/2 - CL*0.5   (a mitad del pasillo A)
//  Trigger B  Z > variantOrigin.z + RD/2 + CL*0.5   (a mitad del pasillo B)
//
//  Al cruzar el trigger:
//    · Se sortea una nueva variante (con o sin anomalía)
//    · La cámara se coloca en el inicio del pasillo OPUESTO de esa variante
//      mirando hacia la habitación, para que el jugador siga caminando
//      sin notar el salto.
//    · Si venías del pasillo A → apareces al inicio del pasillo B (y viceversa)
//
//  LÓGICA DE JUEGO
//  ─────────────────────────────────────────────────────────────────────────────
//  · El jugador SIEMPRE puede andar en cualquier dirección.
//  · "Avanzar" = cruzar el trigger en la misma dirección que llevabas.
//  · "Retroceder" = dar la vuelta y cruzar el trigger del pasillo por el que
//    entraste (no el de salida).
//  · Si hay anomalía: retroceder es CORRECTO (+1), avanzar es FALLO (reset).
//  · Si no hay anomalía: avanzar es CORRECTO (+1), retroceder es FALLO (reset).
//
//  ANOMALÍAS
//  ─────────────────────────────────────────────────────────────────────────────
//  · NONE        – habitación normal, luces blancas
//  · RED_LIGHTS  – todas las luces de la habitación se vuelven rojas
//
//  CONTROLES: WASD + ratón | ESC para salir
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
#include <ctime>
#include <string>
#include <vector>
#include <array>
#include "Camera.h"

// ── Ventana ───────────────────────────────────────────────────────────────────
static constexpr int SCR_WIDTH  = 1280;
static constexpr int SCR_HEIGHT = 720;

// ── Cámara ────────────────────────────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 1.7f, 0.0f));
float lastX      = SCR_WIDTH  / 2.0f;
float lastY      = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime  = 0.0f;
float lastFrame  = 0.0f;

void framebufferSizeCallback(GLFWwindow*, int w, int h){ glViewport(0,0,w,h); }
void mouseCallback(GLFWwindow*, double xI, double yI);
void scrollCallback(GLFWwindow*, double, double yO);
void processInput(GLFWwindow*);
static void openGlInit();

// =============================================================================
//  CONSTANTES GEOMÉTRICAS
// =============================================================================
static constexpr float RW  = 8.0f;   // ancho de la habitación (eje X)
static constexpr float RD  = 8.0f;   // profundidad de la habitación (eje Z)
static constexpr float RH  = 3.5f;   // altura
static constexpr float CW  = 3.0f;   // ancho del corredor
static constexpr float CL  = 12.0f;  // longitud total de cada corredor
static constexpr float EYE = 1.7f;   // altura de los ojos del jugador

// Desplazamiento en X entre variantes para que nunca se vean dos a la vez.
// Debe ser >> RW + 2*CL para que queden totalmente fuera del frustum.
static constexpr float VARIANT_STRIDE = 200.0f;

// Pasillo A: esquina oeste, sale hacia -Z
//   apertura en la pared sur de la habitación, desplazado a X = -RW/2
// Pasillo B: esquina este, sale hacia +Z
//   apertura en la pared norte de la habitación, desplazado a X = +RW/2 - CW
//
// Posición X del centro de cada corredor (relativa al centro de la habitación):
static constexpr float COR_A_DX = -(RW/2.0f - CW/2.0f);   // -2.5  (esquina oeste)
static constexpr float COR_B_DX =  (RW/2.0f - CW/2.0f);   //  2.5  (esquina este)

// Triggers: mitad del corredor
static constexpr float TRIG_A_DZ = -(RD/2.0f + CL * 0.5f);  // -10.0 desde el centro de la hab.
static constexpr float TRIG_B_DZ =  (RD/2.0f + CL * 0.5f);  // +10.0

// Spawn al inicio de cada corredor (0.8 de longitud dentro del corredor desde la hab.)
static constexpr float SPAWN_A_DZ = -(RD/2.0f + CL * 0.15f);  //  -5.2  (cerca de la hab.)
static constexpr float SPAWN_B_DZ =  (RD/2.0f + CL * 0.15f);  //  +5.2

// =============================================================================
//  ANOMALÍAS
// =============================================================================
enum class AnomalyType { NONE, RED_LIGHTS };

AnomalyType randomAnomaly() {
    return (rand() % 2 == 0) ? AnomalyType::NONE : AnomalyType::RED_LIGHTS;
}

// Cuántas variantes distintas hay que renderizar
// (una por cada AnomalyType, en orden: NONE=0, RED_LIGHTS=1)
static constexpr int NUM_VARIANTS = 2;

// =============================================================================
//  ESTADO DEL JUEGO
// =============================================================================
enum class GameState { PLAYING, FADING_OUT, FADING_IN };

// "Dirección" por la que el jugador entró a la habitación actual.
// Usada para determinar si "vuelve atrás" o "avanza".
enum class EntryDir { FROM_A, FROM_B };

struct GameData {
    GameState   state     = GameState::PLAYING;
    int         score     = 0;
    int         highScore = 0;

    int         currentVariant = 0;         // índice de la variante activa
    AnomalyType currentAnomaly = AnomalyType::NONE;
    EntryDir    entryDir       = EntryDir::FROM_A;

    // Fade
    float fadeAlpha = 0.0f;
    static constexpr float FADE_SPEED = 3.0f;

    // Qué hacer al final del fade
    struct PendingTeleport {
        bool   active    = false;
        int    toVariant = 0;
        bool   toCorA    = true;   // true=spawn en corredor A, false=corredor B
        bool   isCorrect = true;
    } pending;

    // Colores actuales de la habitación (cambian por anomalía)
    glm::vec3 roomWall  = {0.82f, 0.80f, 0.75f};
    glm::vec3 roomFloor = {0.55f, 0.52f, 0.48f};
    glm::vec3 roomCeil  = {0.90f, 0.90f, 0.92f};
    glm::vec3 lightCol  = {0.95f, 0.95f, 1.00f};
    float     ambient   = 0.04f;
};
static GameData G;

// =============================================================================
//  LUCES
// =============================================================================
static constexpr int MAX_LIGHTS = 16;

struct PointLight {
    glm::vec3 pos;
    glm::vec3 color;
    float     intensity = 1.0f;
    float     Kc = 1.0f, Kl = 0.07f, Kq = 0.017f;
    float     flickerTimer = 0.0f, flickerDuration = 0.0f;
    bool      isFlickering = false;
};

static PointLight gLights[MAX_LIGHTS];
static int        gNumLights = 0;

void resetLights(std::vector<glm::vec3> positions, glm::vec3 color)
{
    gNumLights = (int)positions.size();
    if (gNumLights > MAX_LIGHTS) gNumLights = MAX_LIGHTS;
    for (int i = 0; i < gNumLights; i++) {
        PointLight& L = gLights[i];
        L.pos          = positions[i];
        L.color        = color;
        L.intensity    = 1.0f;
        L.isFlickering = false;
        L.flickerTimer = 1.5f + (static_cast<float>(rand())/RAND_MAX)*4.5f;
    }
}

void uploadLights(unsigned int prog)
{
    for (int i = 0; i < gNumLights; i++) {
        const PointLight& L = gLights[i];
        std::string b = "uLights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(prog,(b+"position").c_str()),1,glm::value_ptr(L.pos));
        glUniform3fv(glGetUniformLocation(prog,(b+"color").c_str()),   1,glm::value_ptr(L.color));
        glUniform1f (glGetUniformLocation(prog,(b+"intensity").c_str()),L.intensity);
        glUniform1f (glGetUniformLocation(prog,(b+"Kc").c_str()),L.Kc);
        glUniform1f (glGetUniformLocation(prog,(b+"Kl").c_str()),L.Kl);
        glUniform1f (glGetUniformLocation(prog,(b+"Kq").c_str()),L.Kq);
    }
    glUniform1i(glGetUniformLocation(prog,"uNumLights"), gNumLights);
}

// =============================================================================
//  GEOMETRÍA
// =============================================================================
struct Mesh { unsigned int VAO, VBO; int count = 0; };

// Construye un quad de 6 vértices (2 triángulos).
// o = esquina de origen, r = eje "ancho" con longitud rl, u = eje "alto" con ul,
// n = normal, su/sv = escala UV.
Mesh buildQuad(glm::vec3 o, glm::vec3 r, float rl,
               glm::vec3 u, float ul, glm::vec3 n,
               float su=1.f, float sv=1.f)
{
    float v[48];
    glm::vec3 p0=o, p1=o+r*rl, p2=o+r*rl+u*ul, p3=o+u*ul;
    auto f=[&](int i,glm::vec3 p,float tu,float tv){
        int b=i*8;
        v[b]=p.x; v[b+1]=p.y; v[b+2]=p.z;
        v[b+3]=n.x; v[b+4]=n.y; v[b+5]=n.z;
        v[b+6]=tu; v[b+7]=tv;
    };
    f(0,p0,0,0); f(1,p1,su,0); f(2,p2,su,sv);
    f(3,p2,su,sv); f(4,p3,0,sv); f(5,p0,0,0);

    Mesh m; m.count=6;
    glGenVertexArrays(1,&m.VAO); glGenBuffers(1,&m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER,m.VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    int s=8*sizeof(float);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,s,(void*)0);                   glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,s,(void*)(3*sizeof(float)));   glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,s,(void*)(6*sizeof(float)));   glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return m;
}

// ── Habitación ────────────────────────────────────────────────────────────────
// Una habitación está compuesta por 6 quads (paredes, suelo, techo).
// openN/S/E/W indica si esa cara tiene una apertura (no se renderiza).
// Para la apertura de los pasillos usamos openS (pasillo A, -Z) y openN (pasillo B, +Z).
// Como los pasillos solo ocupan parte del ancho de la pared, las paredes completas
// sí se dibujan; la "apertura" es la ausencia de colisión, no un agujero real todavía.
// (Para este prototipo las paredes son sólidas pero los corredores están alineados
// de modo que al caminar se sale naturalmente hacia ellos.)
struct Room {
    Mesh floor, ceiling;
    Mesh wN, wS, wE, wW;  // Norte (+Z), Sur (-Z), Este (+X), Oeste (-X)
};

// ── Habitación ────────────────────────────────────────────────────────────────
Room buildRoom(glm::vec3 center, float w, float d, float h) {
    Room R; float hw=w/2, hd=d/2; glm::vec3 c=center;
    R.floor   = buildQuad(c+glm::vec3(-hw,0, hd), {1,0,0},w, {0,0,-1},d, {0,1,0}, w/2,d/2);
    R.ceiling = buildQuad(c+glm::vec3(-hw,h,-hd), {1,0,0},w, {0,0, 1},d, {0,-1,0}, w/2,d/2);

    // ARREGLADO: Se dibuja desde la esquina Izquierda (-4) hacia el hueco (1) usando +X
    R.wN = buildQuad(c + glm::vec3(-hw, 0, hd), {1,0,0}, 5.f, {0,1,0}, h, {0,0,-1}, 2.5f, h/2);
    
    // Pared Sur (-Z). Hueco en X=[-4, -1]. Sólida en X=[-1, 4].
    R.wS = buildQuad(c + glm::vec3(-1.f, 0,-hd), {1,0,0}, 5.f,  {0,1,0}, h, {0,0, 1}, 2.5f, h/2);
    R.wE = buildQuad(c+glm::vec3( hw,0,-hd), {0,0, 1},d, {0,1,0},h, {-1,0,0}, d/2,h/2);
    R.wW = buildQuad(c+glm::vec3(-hw,0, hd), {0,0,-1},d, {0,1,0},h, { 1,0,0}, d/2,h/2);
    return R;
}

// ── Corredor ──────────────────────────────────────────────────────────────────
// Un corredor es un tubo rectangular que sale de la habitación en el eje Z.
// Tiene suelo, techo, pared izquierda y pared derecha. Los extremos quedan abiertos.
struct Corridor {
    Mesh floor, ceiling, wLeft, wRight;
};

// Corredor que corre en el eje Z (±).
// center = centro del corredor en world-space.
// len = longitud (en Z), w = ancho (en X), h = alto.
// Si dirZ = -1 el corredor sale hacia -Z, si +1 hacia +Z.
Corridor buildCorridorZ(glm::vec3 center, float len, float w, float h) {
    Corridor C; float hl=len/2, hw=w/2; glm::vec3 c=center;
    C.floor   = buildQuad(c+glm::vec3(-hw,0, hl), {1,0,0},w, {0,0,-1},len, {0,1,0}, w/2,len/2);
    C.ceiling = buildQuad(c+glm::vec3(-hw,h,-hl), {1,0,0},w, {0,0, 1},len, {0,-1,0}, w/2,len/2);
    C.wLeft   = buildQuad(c+glm::vec3(-hw,0, hl), {0,0,-1},len, {0,1,0},h, { 1,0,0}, len/2,h/2); // Corregido a +X
    C.wRight  = buildQuad(c+glm::vec3( hw,0,-hl), {0,0, 1},len, {0,1,0},h, {-1,0,0}, len/2,h/2);
    return C;
}

// ── Draw helpers ─────────────────────────────────────────────────────────────
void drawMesh(unsigned int prog, int locModel, int locColor,
              const Mesh& m, glm::vec3 col)
{
    if (m.count == 0) return;
    glm::mat4 I = glm::mat4(1);
    glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(I));
    glUniform3fv(locColor,1,glm::value_ptr(col));
    glBindVertexArray(m.VAO);
    glDrawArrays(GL_TRIANGLES,0,m.count);
}

void drawRoom(unsigned int prog, int locModel, int locColor,
              const Room& R, glm::vec3 wc, glm::vec3 fc, glm::vec3 cc)
{
    drawMesh(prog,locModel,locColor, R.floor,   fc);
    drawMesh(prog,locModel,locColor, R.ceiling, cc);
    drawMesh(prog,locModel,locColor, R.wN,      wc);
    drawMesh(prog,locModel,locColor, R.wS,      wc);
    drawMesh(prog,locModel,locColor, R.wE,      wc);
    drawMesh(prog,locModel,locColor, R.wW,      wc);
}

void drawCorridor(unsigned int prog, int locModel, int locColor,
                  const Corridor& C, glm::vec3 wc, glm::vec3 fc, glm::vec3 cc)
{
    drawMesh(prog,locModel,locColor, C.floor,   fc);
    drawMesh(prog,locModel,locColor, C.ceiling, cc);
    drawMesh(prog,locModel,locColor, C.wLeft,   wc);
    drawMesh(prog,locModel,locColor, C.wRight,  wc);
}

// =============================================================================
//  VARIANTE DE HABITACIÓN
//  Cada variante es una habitación + sus dos corredores, desplazada en X.
// =============================================================================
struct RoomVariant {
    AnomalyType anomaly;
    glm::vec3   origin;     // centro de la habitación en world-space

    Room        room;
    Corridor    corA;       // pasillo que sale por -Z (esquina oeste)
    Corridor    corB;       // pasillo que sale por +Z (esquina este)

    Corridor    corATurn;   // Codo horizontal para pasillo A
    Corridor    corBTurn;   // Codo horizontal para pasillo B
    Mesh        endWallA;   // Pared para tapar la vista infinita en A
    Mesh        endWallB;   // Pared para tapar la vista infinita en B
    Mesh        cornerWallA;
    Mesh        cornerWallB;

    // Centro de los corredores en world-space (para las luces)
    glm::vec3   corACenter;
    glm::vec3   corBCenter;
};

// Construye una variante.
// variantIdx: 0, 1, 2, … define el offset X para que no se superpongan.
// anomaly: qué tiene esta habitación.
// ── Variante ──────────────────────────────────────────────────────────────────
RoomVariant buildVariant(int variantIdx, AnomalyType anomaly) {
    RoomVariant V; V.anomaly = anomaly;
    V.origin = glm::vec3(variantIdx * VARIANT_STRIDE, 0.0f, 0.0f); glm::vec3 o = V.origin;

    V.room = buildRoom(o, RW, RD, RH);
    float cor_len = CL - CW; // Longitud exacta sin pisar el codo (9.0)

    // ================= PASILLO A (Esquina SO) =================
    V.corACenter = o + glm::vec3(COR_A_DX, 0, -(RD/2.0f + cor_len/2.0f)); 
    V.corA = buildCorridorZ(V.corACenter, cor_len, CW, RH);

    // ARREGLADO: Todas las paredes de los codos ahora se dibujan CCW
    V.corATurn.floor   = buildQuad(o + glm::vec3(-4.f, 0, -13.f), {1,0,0}, 10.f, {0,0,-1}, 3.f, {0,1,0}, 5.f, 1.5f);
    V.corATurn.ceiling = buildQuad(o + glm::vec3(-4.f, RH,-16.f), {1,0,0}, 10.f, {0,0, 1}, 3.f, {0,-1,0}, 5.f, 1.5f);
    V.corATurn.wLeft   = buildQuad(o + glm::vec3(-4.f, 0, -16.f), {1,0,0}, 10.f, {0,1,0}, RH, {0,0, 1}, 5.f, RH/2.f); 
    // wRight de A arreglada para ir hacia +X
    V.corATurn.wRight  = buildQuad(o + glm::vec3(-1.f, 0, -13.f), { 1,0,0}, 7.f, {0,1,0}, RH, {0,0, -1}, 3.5f, RH/2.f); 

    V.cornerWallA = buildQuad(o + glm::vec3(-4.f, 0, -13.f), {0,0,-1}, 3.f, {0,1,0}, RH, { 1,0,0}, 1.5f, RH/2.f); 
    V.endWallA    = buildQuad(o + glm::vec3( 6.f, 0, -16.f), {0,0, 1}, 3.f, {0,1,0}, RH, {-1,0,0}, 1.5f, RH/2.f); 

    // ================= PASILLO B (Esquina NE) =================
    V.corBCenter = o + glm::vec3(COR_B_DX, 0, (RD/2.0f + cor_len/2.0f)); 
    V.corB = buildCorridorZ(V.corBCenter, cor_len, CW, RH);

    V.corBTurn.floor   = buildQuad(o + glm::vec3( 4.f, 0,  13.f), {-1,0,0}, 10.f, {0,0, 1}, 3.f, {0,1,0}, 5.f, 1.5f);
    V.corBTurn.ceiling = buildQuad(o + glm::vec3( 4.f, RH, 16.f), {-1,0,0}, 10.f, {0,0,-1}, 3.f, {0,-1,0}, 5.f, 1.5f);
    // wLeft de B arreglada para ir hacia +X
    V.corBTurn.wLeft   = buildQuad(o + glm::vec3(-6.f, 0,  16.f), { 1,0,0}, 10.f, {0,1,0}, RH, {0,0,-1}, 5.f, RH/2.f); 
    V.corBTurn.wRight  = buildQuad(o + glm::vec3(-6.f, 0,  13.f), { 1,0,0}, 7.f, {0,1,0}, RH, {0,0, 1}, 3.5f, RH/2.f); 

    V.cornerWallB = buildQuad(o + glm::vec3( 4.f, 0,  13.f), {0,0, 1}, 3.f, {0,1,0}, RH, {-1,0,0}, 1.5f, RH/2.f); 
    V.endWallB    = buildQuad(o + glm::vec3(-6.f, 0,  16.f), {0,0,-1}, 3.f, {0,1,0}, RH, { 1,0,0}, 1.5f, RH/2.f); 
    
    return V;
}

// Luces de una variante
std::vector<glm::vec3> roomLightPositions(glm::vec3 origin)
{
    float y = RH - 0.3f;
    return {
        origin + glm::vec3(-RW/4, y, -RD/4),
        origin + glm::vec3( RW/4, y, -RD/4),
        origin + glm::vec3(-RW/4, y,  RD/4),
        origin + glm::vec3( RW/4, y,  RD/4),
    };
}

std::vector<glm::vec3> corridorLightPositions(glm::vec3 corridorCenter, float len)
{
    float y = RH - 0.3f;
    // Tres luces distribuidas a lo largo del corredor (eje Z)
    return {
        corridorCenter + glm::vec3(0, y - corridorCenter.y, -len/3.0f),
        corridorCenter + glm::vec3(0, y - corridorCenter.y,  0.0f),
        corridorCenter + glm::vec3(0, y - corridorCenter.y,  len/3.0f),
    };
}

// =============================================================================
//  LÓGICA DE TELEPORT Y JUEGO
// =============================================================================

// Colores según anomalía
void applyAnomalyColors(AnomalyType a)
{
    G.currentAnomaly = a;
    switch (a) {
    case AnomalyType::NONE:
        G.roomWall  = {0.82f, 0.80f, 0.75f};
        G.roomFloor = {0.55f, 0.52f, 0.48f};
        G.roomCeil  = {0.90f, 0.90f, 0.92f};
        G.lightCol  = {0.95f, 0.95f, 1.00f};
        G.ambient   =  0.04f;
        break;
    case AnomalyType::RED_LIGHTS:
        G.roomWall  = {0.40f, 0.10f, 0.10f};
        G.roomFloor = {0.30f, 0.07f, 0.07f};
        G.roomCeil  = {0.35f, 0.08f, 0.08f};
        G.lightCol  = {1.00f, 0.06f, 0.04f};
        G.ambient   =  0.02f;
        break;
    }
}

// Pone la cámara al inicio de un corredor mirando hacia la habitación.
// corA=true → spawn al inicio del pasillo A (Z negativo, mirando hacia +Z)
// corA=false → spawn al inicio del pasillo B (Z positivo, mirando hacia -Z)
void spawnAtCorridor(const RoomVariant& V, bool corA) {
    if (corA) {
        // Spawn A: Zona segura en X=3.5. Mira a -X (hacia el pasillo recto)
        camera.Position = V.origin + glm::vec3(3.5f, EYE, -14.5f);
        camera.Yaw = 180.0f;
    } else {
        // Spawn B: Zona segura en X=-3.5. Mira a +X (hacia el pasillo recto)
        camera.Position = V.origin + glm::vec3(-3.5f, EYE, 14.5f);
        camera.Yaw = 0.0f;
    }
    camera.Pitch = 0.0f; camera.UpdateCameraVectors(); firstMouse = true;
}

void activateLights(const RoomVariant& V) {
    glm::vec3 corColor = {0.95f, 0.95f, 1.0f};
    gNumLights = 0;

    // Helper macro para añadir focos limpios
    auto addL = [&](glm::vec3 p, glm::vec3 c) {
        if(gNumLights >= MAX_LIGHTS) return;
        PointLight& L = gLights[gNumLights++];
        L.pos = p; L.color = c; L.intensity = 1.0f;
        L.Kc=1.0f; L.Kl=0.09f; L.Kq=0.032f;
    };

    // Luces de Habitación
    for (auto& p : roomLightPositions(V.origin)) addL(p, G.lightCol);
    
    // Luces de Pasillos Rectos (Corregimos la atenuación Kl/Kq)
    for (auto& p : corridorLightPositions(V.corACenter, CL)) { p.y = RH-0.3f; addL(p, corColor); }
    for (auto& p : corridorLightPositions(V.corBCenter, CL)) { p.y = RH-0.3f; addL(p, corColor); }

    // ¡NUEVO! Focos directamente sobre las esquinas de los codos
    addL(V.origin + glm::vec3( 1.0f, RH-0.3f, -14.5f), corColor); // Ilumina hueco A
    addL(V.origin + glm::vec3(-1.0f, RH-0.3f,  14.5f), corColor); // Ilumina hueco B
}

// Fade + teleport pendiente
void triggerTeleport(int toVariant, bool toCorA, bool isCorrect)
{
    if (G.state != GameState::PLAYING) return;
    G.state = GameState::FADING_OUT;
    G.fadeAlpha = 0.0f;
    G.pending.active    = true;
    G.pending.toVariant = toVariant;
    G.pending.toCorA    = toCorA;
    G.pending.isCorrect = isCorrect;
}

// Sortea el siguiente índice de variante (evita repetir la misma si hay más de una)
int nextVariantIdx(int current)
{
    if (NUM_VARIANTS <= 1) return 0;
    int next = rand() % NUM_VARIANTS;
    // Si por azar sale la misma, intentamos el siguiente en orden
    if (next == current) next = (current + 1) % NUM_VARIANTS;
    return next;
}

void applyCollisions(glm::vec3& pos, const RoomVariant& V) {
    float R = 0.2f; // Radio del jugador
    
    // Convertimos la posición a coordenadas locales relativas al centro de la variante
    float lx = pos.x - V.origin.x;
    float lz = pos.z - V.origin.z;

    // Límite absoluto de los fondos de los codos
    lz = glm::clamp(lz, -16.0f + R, 16.0f - R);

    // ==========================================
    // LADO SUR (-Z) : Habitación Sur, Cor A, Turn A
    // ==========================================
    if (lz < 0.0f) {
        lx = glm::max(lx, -4.0f + R); // La pared Oeste completa es plana y continua

        // Resolución de esquinas cóncavas ("El bloque sólido" a la derecha del pasillo A)
        // Solo ocurre si invades la zona de la pared: X > -1 y Z está entre -13 y -4
        if (lx > -1.0f - R && lz < -4.0f + R && lz > -13.0f - R) {
            float distX = lx - (-1.0f - R); // Cuánto penetraste en X
            if (lz > -8.5f) { 
                // Estás más cerca de la habitación (Z = -4)
                float distZ = (-4.0f + R) - lz; // Cuánto penetraste en Z
                if (distX < distZ) lx = -1.0f - R; // Te empujamos en X (estás rozando el pasillo)
                else               lz = -4.0f + R; // Te empujamos en Z (chocaste de frente con la sala)
            } else { 
                // Estás más cerca del codo A (Z = -13)
                float distZ = lz - (-13.0f - R);
                if (distX < distZ) lx = -1.0f - R; 
                else               lz = -13.0f - R; 
            }
        }
        
        // Asignación de los anchos dependiendo de tu profundidad Z actual
        if (lz < -13.0f)      lx = glm::min(lx, 6.0f - R);  // Codo A (abierto hasta +6)
        else if (lz < -4.0f)  lx = glm::min(lx, -1.0f - R); // Pasillo recto A (cerrado en -1)
        else                  lx = glm::min(lx, 4.0f - R);  // Habitación Sur (abierto hasta +4)
    } 
    // ==========================================
    // LADO NORTE (+Z) : Habitación Norte, Cor B, Turn B
    // ==========================================
    else {
        lx = glm::min(lx, 4.0f - R); // La pared Este completa es plana y continua

        // Resolución de esquinas cóncavas ("El bloque sólido" a la izquierda del pasillo B)
        // Solo ocurre si invades la zona de la pared: X < 1 y Z está entre 4 y 13
        if (lx < 1.0f + R && lz > 4.0f - R && lz < 13.0f + R) {
            float distX = (1.0f + R) - lx;
            if (lz < 8.5f) { 
                // Estás más cerca de la habitación (Z = 4)
                float distZ = lz - (4.0f - R);
                if (distX < distZ) lx = 1.0f + R;
                else               lz = 4.0f - R;
            } else { 
                // Estás más cerca del codo B (Z = 13)
                float distZ = (13.0f + R) - lz;
                if (distX < distZ) lx = 1.0f + R;
                else               lz = 13.0f + R;
            }
        }

        // Asignación de los anchos dependiendo de tu profundidad Z actual
        if (lz > 13.0f)      lx = glm::max(lx, -6.0f + R); // Codo B (abierto hasta -6)
        else if (lz > 4.0f)  lx = glm::max(lx, 1.0f + R);  // Pasillo recto B (cerrado en +1)
        else                 lx = glm::max(lx, -4.0f + R); // Habitación Norte (abierto hasta -4)
    }

    // Aplicamos de vuelta las posiciones al mundo global
    pos.x = V.origin.x + lx;
    pos.z = V.origin.z + lz;
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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"The Exit 8",nullptr,nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback      (window, mouseCallback);
    glfwSetScrollCallback         (window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    openGlInit();

    unsigned int shader = setShaders("exit8.vert","exit8.frag");
    int locModel  = glGetUniformLocation(shader,"uModel");
    int locView   = glGetUniformLocation(shader,"uView");
    int locProj   = glGetUniformLocation(shader,"uProjection");
    int locVP     = glGetUniformLocation(shader,"uViewPos");
    int locAmb    = glGetUniformLocation(shader,"uAmbient");
    int locUseTex = glGetUniformLocation(shader,"uUsarTextura");
    int locColor  = glGetUniformLocation(shader,"uColor");

    // ── Construir las N variantes ─────────────────────────────────────────────
    // Variante 0 = sin anomalía, Variante 1 = luces rojas
    std::array<RoomVariant, NUM_VARIANTS> variants;
    variants[0] = buildVariant(0, AnomalyType::NONE);
    variants[1] = buildVariant(1, AnomalyType::RED_LIGHTS);

    // ── Fade quad (NDC) ───────────────────────────────────────────────────────
    float fv[] = {
        -1,-1,0, 0,0,1, 0,0,
         1,-1,0, 0,0,1, 1,0,
         1, 1,0, 0,0,1, 1,1,
         1, 1,0, 0,0,1, 1,1,
        -1, 1,0, 0,0,1, 0,1,
        -1,-1,0, 0,0,1, 0,0,
    };
    unsigned int fadeVAO,fadeVBO;
    glGenVertexArrays(1,&fadeVAO); glGenBuffers(1,&fadeVBO);
    glBindVertexArray(fadeVAO);
    glBindBuffer(GL_ARRAY_BUFFER,fadeVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(fv),fv,GL_STATIC_DRAW);
    int fs=8*sizeof(float);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,fs,(void*)0);                   glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,fs,(void*)(3*sizeof(float)));   glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,fs,(void*)(6*sizeof(float)));   glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // ── Estado inicial ────────────────────────────────────────────────────────
    // Empezamos en una variante aleatoria, entrando por el pasillo A
    G.currentVariant = rand() % NUM_VARIANTS;
    applyAnomalyColors(variants[G.currentVariant].anomaly);
    activateLights(variants[G.currentVariant]);
    G.entryDir = EntryDir::FROM_A;
    spawnAtCorridor(variants[G.currentVariant], true /* corA */);

    std::cout << "=========================================\n";
    std::cout << "  THE EXIT 8\n";
    std::cout << "  WASD + raton para moverte\n";
    std::cout << "  Anomalia: VUELVE por donde viniste.\n";
    std::cout << "  Normal:   SIGUE adelante.\n";
    std::cout << "=========================================\n";
    std::cout << "[NIVEL 1] Anomalia: "
              << (G.currentAnomaly==AnomalyType::NONE ? "ninguna" : "LUCES ROJAS") << "\n";

    // ── Bucle principal ───────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float now = static_cast<float>(glfwGetTime());
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);
        // Aplicar colisión para restringir la nueva posición de la cámara
        applyCollisions(camera.Position, variants[G.currentVariant]);

        // ── Lógica de triggers ────────────────────────────────────────────────
        if (G.state == GameState::PLAYING) {
            const RoomVariant& V = variants[G.currentVariant];
            float camZ = camera.Position.z;
            float camX = camera.Position.x;

            // ─ Comprobar si el jugador cruza la esquina del pasillo ──────────────
            bool inTurnA = (camZ < V.origin.z - 13.0f) && (camZ > V.origin.z - 16.0f);
            bool inTurnB = (camZ > V.origin.z + 13.0f) && (camZ < V.origin.z + 16.0f);

            // Zonas alejadas del spawn (+3.5 y -3.5)
            float trigA_X = V.origin.x + 5.0f; 
            float trigB_X = V.origin.x - 5.0f;

            // Atención al signo de comparación.
            // Para A: el fondo está hacia +X, así que cruzamos si camX > trigA_X
            if (inTurnA && camX > trigA_X) {
                bool isRetreating = (G.entryDir == EntryDir::FROM_A);
                bool anomaly = (G.currentAnomaly != AnomalyType::NONE);
                bool correct = isRetreating ? anomaly : !anomaly;

                if (correct) G.score++; else G.score = 0;
                if (G.score > G.highScore) G.highScore = G.score;

                int nextVar = nextVariantIdx(G.currentVariant);
                triggerTeleport(nextVar, false /* toCorB */, correct);

            // Para B: el fondo está hacia -X, así que cruzamos si camX < trigB_X
            } else if (inTurnB && camX < trigB_X) {
                bool isRetreating = (G.entryDir == EntryDir::FROM_B);
                bool anomaly = (G.currentAnomaly != AnomalyType::NONE);
                bool correct = isRetreating ? anomaly : !anomaly;

                if (correct) G.score++; else G.score = 0;
                if (G.score > G.highScore) G.highScore = G.score;

                int nextVar = nextVariantIdx(G.currentVariant);
                triggerTeleport(nextVar, true /* toCorA */, correct);
            }
        }

        // ── Actualizar fade ───────────────────────────────────────────────────
        if (G.state == GameState::FADING_OUT) {
            G.fadeAlpha += G.FADE_SPEED * deltaTime;
            if (G.fadeAlpha >= 1.0f) {
                G.fadeAlpha = 1.0f;
                // Ejecutar el teleport
                if (G.pending.active) {
                    G.currentVariant = G.pending.toVariant;
                    applyAnomalyColors(variants[G.currentVariant].anomaly);
                    activateLights(variants[G.currentVariant]);
                    spawnAtCorridor(variants[G.currentVariant], G.pending.toCorA);
                    G.entryDir = G.pending.toCorA ? EntryDir::FROM_A : EntryDir::FROM_B;
                    G.pending.active = false;
                    std::cout << "[NIVEL " << G.score+1 << "] Anomalia: "
                              << (G.currentAnomaly==AnomalyType::NONE?"ninguna":"LUCES ROJAS")
                              << "\n";
                }
                G.state = GameState::FADING_IN;
            }
        } else if (G.state == GameState::FADING_IN) {
            G.fadeAlpha -= G.FADE_SPEED * deltaTime;
            if (G.fadeAlpha <= 0.0f) {
                G.fadeAlpha = 0.0f;
                G.state = GameState::PLAYING;
            }
        }

        // ── Render ────────────────────────────────────────────────────────────
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 proj = camera.GetProjectionMatrix((float)SCR_WIDTH,(float)SCR_HEIGHT);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(locView,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(locProj,1,GL_FALSE,glm::value_ptr(proj));
        glUniform3fv(locVP,  1, glm::value_ptr(camera.Position));
        glUniform3f (locAmb, G.ambient, G.ambient, G.ambient*1.05f);
        glUniform1i (locUseTex, 0);
        uploadLights(shader);

        // Dibujamos TODAS las variantes (solo se verá la activa dado el VARIANT_STRIDE)
        for (int i = 0; i < NUM_VARIANTS; i++) {
            const RoomVariant& V = variants[i];

            // La habitación activa usa los colores de anomalía; las demás, neutras.
            // (Como están a 200u de distancia nunca se ven, pero por si acaso.)
            glm::vec3 wc, fc, cc;
            if (i == G.currentVariant) {
                wc = G.roomWall; fc = G.roomFloor; cc = G.roomCeil;
            } else {
                wc = {0.82f,0.80f,0.75f}; fc = {0.55f,0.52f,0.48f}; cc = {0.90f,0.90f,0.92f};
            }
            glm::vec3 cwc={0.78f,0.76f,0.72f}, cfc={0.48f,0.46f,0.43f}, ccc={0.86f,0.86f,0.88f};

            drawRoom    (shader,locModel,locColor, V.room, wc,fc,cc);
            drawCorridor(shader,locModel,locColor, V.corA, cwc,cfc,ccc);
            drawCorridor(shader,locModel,locColor, V.corB, cwc,cfc,ccc);

            drawCorridor(shader,locModel,locColor, V.corATurn, cwc,cfc,ccc);
            drawCorridor(shader,locModel,locColor, V.corBTurn, cwc,cfc,ccc);
            drawMesh(shader,locModel,locColor, V.endWallA, cwc);
            drawMesh(shader,locModel,locColor, V.endWallB, cwc);

            drawMesh(shader,locModel,locColor, V.cornerWallA, cwc);
            drawMesh(shader,locModel,locColor, V.cornerWallB, cwc);
        }

        glBindVertexArray(0);

        // ── Fade negro ────────────────────────────────────────────────────────
        if (G.fadeAlpha > 0.001f) {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendColor(0,0,0, G.fadeAlpha);
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
            glm::mat4 I = glm::mat4(1);
            glm::mat4 ortho = glm::ortho(-1.f,1.f,-1.f,1.f,-1.f,1.f);
            glUniformMatrix4fv(locView, 1,GL_FALSE,glm::value_ptr(I));
            glUniformMatrix4fv(locProj, 1,GL_FALSE,glm::value_ptr(ortho));
            glUniformMatrix4fv(locModel,1,GL_FALSE,glm::value_ptr(I));
            glUniform3f(locAmb,0,0,0);
            glUniform1i(glGetUniformLocation(shader,"uNumLights"),0);
            glUniform3f(locColor,0,0,0);
            glBindVertexArray(fadeVAO);
            glDrawArrays(GL_TRIANGLES,0,6);
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
//  CALLBACKS
// =============================================================================
void mouseCallback(GLFWwindow*, double xI, double yI)
{
    float x=static_cast<float>(xI), y=static_cast<float>(yI);
    if (firstMouse){ lastX=x; lastY=y; firstMouse=false; }
    camera.ProcessMouseMovement(x-lastX, lastY-y);
    lastX=x; lastY=y;
}

void scrollCallback(GLFWwindow*, double, double yO) {
    camera.Fov = glm::clamp(camera.Fov-(float)yO, 20.f, 90.f);
}

void processInput(GLFWwindow* w)
{
    if (glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(w,true);
    if (glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::BACKWARD,deltaTime);
    if (glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::LEFT,    deltaTime);
    if (glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS) camera.ProcessKeyboard(CameraMovement::RIGHT,   deltaTime);
}

static void openGlInit() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0,1);
}
