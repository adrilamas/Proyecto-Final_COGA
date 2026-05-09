#include "Room.h"
#include "Constants.h"
#include <glm/gtc/type_ptr.hpp>

// =============================================================================
//  buildRoom
//
//  La sala tiene dos aperturas:
//    · Sur (–Z): corredor A en la esquina oeste  X ∈ [–HW, –HW+CW]
//    · Norte (+Z): corredor B en la esquina este X ∈ [+HW–CW, +HW]
//
//  Las paredes N y S se dividen en un tramo sólido (el hueco ya está abierto).
// =============================================================================
Room buildRoom(glm::vec3 center, float w, float d, float h)
{
    Room R;
    float hw = w / 2.f;
    float hd = d / 2.f;
    glm::vec3 c = center;

    // ── Suelo y techo completos ───────────────────────────────────────────────
    R.floor   = buildQuad(c + glm::vec3(-hw, 0, hd),
                          {1,0,0}, w, {0,0,-1}, d, {0,1,0},
                          w/2.f, d/2.f);
    R.ceiling = buildQuad(c + glm::vec3(-hw, h, -hd),
                          {1,0,0}, w, {0,0, 1}, d, {0,-1,0},
                          w/2.f, d/2.f);

    // ── Paredes Este y Oeste completas ────────────────────────────────────────
    R.wE = buildQuad(c + glm::vec3( hw, 0, -hd),
                     {0,0,1}, d, {0,1,0}, h, {-1,0,0},
                     d/2.f, h/2.f);
    R.wW = buildQuad(c + glm::vec3(-hw, 0, hd),
                     {0,0,-1}, d, {0,1,0}, h, {1,0,0},
                     d/2.f, h/2.f);

    // ── Pared Sur (–Z): hueco en el lado oeste [–hw, –hw+CW], sólido el resto ─
    // Sólido: X ∈ [–hw+CW, hw]  longitud = w – CW
    float solidW = w - CW;
    R.wS = buildQuad(c + glm::vec3(-hw + CW, 0, -hd),
                     {1,0,0}, solidW, {0,1,0}, h, {0,0,1},
                     solidW/2.f, h/2.f);

    // ── Pared Norte (+Z): hueco en el lado este [hw–CW, hw], sólido el resto ─
    R.wN = buildQuad(c + glm::vec3(-hw, 0, hd),
                     {1,0,0}, solidW, {0,1,0}, h, {0,0,-1},
                     solidW/2.f, h/2.f);

    return R;
}

// =============================================================================
//  buildCorridorZ  – tubo rectangular a lo largo del eje Z
// =============================================================================
Corridor buildCorridorZ(glm::vec3 center, float len, float w, float h)
{
    Corridor C;
    float hl = len / 2.f;
    float hw = w   / 2.f;
    glm::vec3 c = center;

    C.floor   = buildQuad(c + glm::vec3(-hw, 0, -hl),
                          {1,0,0}, w, {0,0,1}, len, {0,1,0},
                          w/2.f, len/2.f);
    C.ceiling = buildQuad(c + glm::vec3(-hw, h, hl),
                          {1,0,0}, w, {0,0,-1}, len, {0,-1,0},
                          w/2.f, len/2.f);
    C.wLeft   = buildQuad(c + glm::vec3(-hw, 0, -hl),
                          {0,0,1}, len, {0,1,0}, h, {1,0,0},
                          len/2.f, h/2.f);
    C.wRight  = buildQuad(c + glm::vec3( hw, 0, hl),
                          {0,0,-1}, len, {0,1,0}, h, {-1,0,0},
                          len/2.f, h/2.f);
    return C;
}

// =============================================================================
//  Draw helpers
// =============================================================================
void drawMesh(unsigned int prog, int locModel, int locColor,
              const Mesh& m, glm::vec3 col)
{
    if (m.count == 0) return;
    glm::mat4 I(1.f);
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(I));
    glUniform3fv(locColor, 1, glm::value_ptr(col));
    glBindVertexArray(m.VAO);
    glDrawArrays(GL_TRIANGLES, 0, m.count);
}

void drawRoom(unsigned int prog, int locModel, int locColor,
              const Room& R,
              glm::vec3 wc, glm::vec3 fc, glm::vec3 cc)
{
    drawMesh(prog, locModel, locColor, R.floor,   fc);
    drawMesh(prog, locModel, locColor, R.ceiling, cc);
    drawMesh(prog, locModel, locColor, R.wN,      wc);
    drawMesh(prog, locModel, locColor, R.wS,      wc);
    drawMesh(prog, locModel, locColor, R.wE,      wc);
    drawMesh(prog, locModel, locColor, R.wW,      wc);
}

void drawCorridor(unsigned int prog, int locModel, int locColor,
                  const Corridor& C,
                  glm::vec3 wc, glm::vec3 fc, glm::vec3 cc)
{
    drawMesh(prog, locModel, locColor, C.floor,   fc);
    drawMesh(prog, locModel, locColor, C.ceiling, cc);
    drawMesh(prog, locModel, locColor, C.wLeft,   wc);
    drawMesh(prog, locModel, locColor, C.wRight,  wc);
}
