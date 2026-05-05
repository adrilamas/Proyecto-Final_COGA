#include "RoomVariant.h"
#include "Constants.h"

// =============================================================================
//  buildVariant
// =============================================================================
RoomVariant buildVariant(int variantIdx, AnomalyType anomaly)
{
    RoomVariant V;
    V.anomaly = anomaly;
    V.origin  = glm::vec3(variantIdx * VARIANT_STRIDE, 0.0f, 0.0f);
    glm::vec3 o = V.origin;

    // ── Habitación central ────────────────────────────────────────────────────
    V.room = buildRoom(o, RW, RD, RH);

    // Longitud del tramo recto (sin pisar el codo)
    float cor_len = CL - CW; // 9.0

    // ── Pasillo A – esquina SO, sale hacia –Z ─────────────────────────────────
    V.corACenter = o + glm::vec3(COR_A_DX, 0.f, -(RD / 2.f + cor_len / 2.f));
    V.corA       = buildCorridorZ(V.corACenter, cor_len, CW, RH);

    // Codo A (gira en horizontal hacia +X)
    V.corATurn.floor   = buildQuad(o + glm::vec3(-4.f, 0,    -13.f), {1,0,0}, 10.f, {0,0,-1}, 3.f, {0,1,0},  5.f,    1.5f);
    V.corATurn.ceiling = buildQuad(o + glm::vec3(-4.f, RH,   -16.f), {1,0,0}, 10.f, {0,0, 1}, 3.f, {0,-1,0}, 5.f,    1.5f);
    V.corATurn.wLeft   = buildQuad(o + glm::vec3(-4.f, 0,    -16.f), {1,0,0}, 10.f, {0,1,0}, RH, {0,0, 1},   5.f,   RH/2.f);
    V.corATurn.wRight  = buildQuad(o + glm::vec3(-1.f, 0,    -13.f), {1,0,0},  7.f, {0,1,0}, RH, {0,0,-1},   3.5f,  RH/2.f);

    V.cornerWallA = buildQuad(o + glm::vec3(-4.f, 0, -13.f), {0,0,-1}, 3.f, {0,1,0}, RH, { 1,0,0}, 1.5f, RH/2.f);
    V.endWallA    = buildQuad(o + glm::vec3( 6.f, 0, -16.f), {0,0, 1}, 3.f, {0,1,0}, RH, {-1,0,0}, 1.5f, RH/2.f);

    // ── Pasillo B – esquina NE, sale hacia +Z ─────────────────────────────────
    V.corBCenter = o + glm::vec3(COR_B_DX, 0.f, (RD / 2.f + cor_len / 2.f));
    V.corB       = buildCorridorZ(V.corBCenter, cor_len, CW, RH);

    // Codo B (gira en horizontal hacia –X)
    V.corBTurn.floor   = buildQuad(o + glm::vec3( 4.f, 0,    13.f), {-1,0,0}, 10.f, {0,0, 1}, 3.f, {0,1,0},  5.f,   1.5f);
    V.corBTurn.ceiling = buildQuad(o + glm::vec3( 4.f, RH,   16.f), {-1,0,0}, 10.f, {0,0,-1}, 3.f, {0,-1,0}, 5.f,   1.5f);
    V.corBTurn.wLeft   = buildQuad(o + glm::vec3(-6.f, 0,    16.f), { 1,0,0}, 10.f, {0,1,0}, RH, {0,0,-1},   5.f,   RH/2.f);
    V.corBTurn.wRight  = buildQuad(o + glm::vec3(-6.f, 0,    13.f), { 1,0,0},  7.f, {0,1,0}, RH, {0,0, 1},   3.5f,  RH/2.f);

    V.cornerWallB = buildQuad(o + glm::vec3( 4.f, 0,  13.f), {0,0, 1}, 3.f, {0,1,0}, RH, {-1,0,0}, 1.5f, RH/2.f);
    V.endWallB    = buildQuad(o + glm::vec3(-6.f, 0,  16.f), {0,0,-1}, 3.f, {0,1,0}, RH, { 1,0,0}, 1.5f, RH/2.f);

    return V;
}

// =============================================================================
//  roomLightPositions
// =============================================================================
std::vector<glm::vec3> roomLightPositions(glm::vec3 origin)
{
    float y = RH - 0.3f;
    return {
        origin + glm::vec3(-RW / 4.f, y, -RD / 4.f),
        origin + glm::vec3( RW / 4.f, y, -RD / 4.f),
        origin + glm::vec3(-RW / 4.f, y,  RD / 4.f),
        origin + glm::vec3( RW / 4.f, y,  RD / 4.f),
    };
}

// =============================================================================
//  corridorLightPositions
// =============================================================================
std::vector<glm::vec3> corridorLightPositions(glm::vec3 corridorCenter, float len)
{
    float y = RH - 0.3f;
    return {
        corridorCenter + glm::vec3(0.f, y - corridorCenter.y, -len / 3.f),
        corridorCenter + glm::vec3(0.f, y - corridorCenter.y,  0.0f),
        corridorCenter + glm::vec3(0.f, y - corridorCenter.y,  len / 3.f),
    };
}
