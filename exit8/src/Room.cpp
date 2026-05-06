#include "Room.h"
#include "Constants.h"
#include <glm/gtc/type_ptr.hpp>

// =============================================================================
//  buildRoom
// =============================================================================
/*Room buildRoom(glm::vec3 center, float w, float d, float h)
{
    Room R;
    float hw = w / 2.f, hd = d / 2.f;
    glm::vec3 c = center;

    R.floor   = buildQuad(c + glm::vec3(-hw, 0,  hd), {1,0,0}, w, {0,0,-1}, d, {0,1,0}, w/2.f, d/2.f);
    R.ceiling = buildQuad(c + glm::vec3(-hw, h, -hd), {1,0,0}, w, {0,0, 1}, d, {0,-1,0}, w/2.f, d/2.f);

    // Pared Norte (+Z): Sólido en X=[-4, 1], Hueco en X=[1, 4]
    R.wN = buildQuad(c + glm::vec3(-4.f, 0, hd), { 1,0,0 }, 5.f, { 0,1,0 }, h, { 0,0,-1 }, 2.5f, h / 2.f);

    // Pared Sur (-Z): Sólido en X=[-1, 4], Hueco en X=[-4, -1]
    R.wS = buildQuad(c + glm::vec3(-1.f, 0, -hd), { 1,0,0 }, 5.f, { 0,1,0 }, h, { 0,0,1 }, 2.5f, h / 2.f);

    R.wE = buildQuad(c + glm::vec3( hw, 0, -hd), {0,0,1}, d,  {0,1,0}, h, {-1,0,0}, d/2.f, h/2.f);
    R.wW = buildQuad(c + glm::vec3(-hw, 0,  hd), {0,0,-1}, d, {0,1,0}, h, { 1,0,0}, d/2.f, h/2.f);

    return R;
}*/

Room buildRoom(glm::vec3 center, float w, float d, float h)
{
    Room R;
    float hw = w / 2.f;
    float hd = d / 2.f;
    glm::vec3 c = center;

    R.floor = buildQuad(c + glm::vec3(-hw, 0, hd), { 1,0,0 }, w, { 0,0,-1 }, d, { 0,1,0 }, w / 2.f, d / 2.f);
    R.ceiling = buildQuad(c + glm::vec3(-hw, h, -hd), { 1,0,0 }, w, { 0,0, 1 }, d, { 0,-1,0 }, w / 2.f, d / 2.f);

    // Asumimos que CW (ancho del pasillo) es accesible aquí. Si no lo es, pásalo como parámetro.
    float wallWidth = w - CW; // La parte sólida de la pared

    // Pared Norte (+Z): Hueco en la derecha (X = hw)
    float xNorthCenter = -hw + (wallWidth / 2.f);
    R.wN = buildQuad(c + glm::vec3(-hw, 0, hd), { 1,0,0 }, wallWidth, { 0,1,0 }, h, { 0,0,-1 }, wallWidth / 2.f, h / 2.f);

    // Pared Sur (-Z): Hueco en la izquierda (X = -hw)
    float xSouthCenter = hw - (wallWidth / 2.f);
    R.wS = buildQuad(c + glm::vec3(-hw + CW, 0, -hd), { 1,0,0 }, wallWidth, { 0,1,0 }, h, { 0,0,1 }, wallWidth / 2.f, h / 2.f);

    R.wE = buildQuad(c + glm::vec3(hw, 0, -hd), { 0,0,1 }, d, { 0,1,0 }, h, { -1,0,0 }, d / 2.f, h / 2.f);
    R.wW = buildQuad(c + glm::vec3(-hw, 0, hd), { 0,0,-1 }, d, { 0,1,0 }, h, { 1,0,0 }, d / 2.f, h / 2.f);

    return R;
}

// =============================================================================
//  buildCorridorZ
// =============================================================================
Corridor buildCorridorZ(glm::vec3 center, float len, float w, float h)
{
    Corridor C;
    float hl = len / 2.f, hw = w / 2.f;
    glm::vec3 c = center;

    C.floor   = buildQuad(c + glm::vec3(-hw, 0,  hl), {1,0,0}, w, {0,0,-1}, len, {0,1,0},  w/2.f, len/2.f);
    C.ceiling = buildQuad(c + glm::vec3(-hw, h, -hl), {1,0,0}, w, {0,0, 1}, len, {0,-1,0}, w/2.f, len/2.f);
    C.wLeft   = buildQuad(c + glm::vec3(-hw, 0,  hl), {0,0,-1}, len, {0,1,0}, h, { 1,0,0}, len/2.f, h/2.f);
    C.wRight  = buildQuad(c + glm::vec3( hw, 0, -hl), {0,0, 1}, len, {0,1,0}, h, {-1,0,0}, len/2.f, h/2.f);

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
