#include "RoomVariant.h"
#include "Constants.h"

// =============================================================================
//  buildVariant
// =============================================================================
/*RoomVariant buildVariant(int variantIdx, AnomalyType anomaly)
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

    // --- Codo A (Ahora gira hacia -X para alejarse) ---
    // Cambiamos el inicio de -4.f a -1.f y la dirección de {1,0,0} a {-1,0,0}
    V.corATurn.floor = buildQuad(o + glm::vec3(-1.f, 0, -13.f), { -1,0,0 }, 10.f, { 0,0,-1 }, 3.f, { 0,1,0 }, 5.f, 1.5f);
    V.corATurn.ceiling = buildQuad(o + glm::vec3(-1.f, RH, -16.f), { -1,0,0 }, 10.f, { 0,0, 1 }, 3.f, { 0,-1,0 }, 5.f, 1.5f);
    V.corATurn.wLeft = buildQuad(o + glm::vec3(-1.f, 0, -16.f), { -1,0,0 }, 10.f, { 0,1,0 }, RH, { 0,0, 1 }, 5.f, RH / 2.f);
    V.corATurn.wRight = buildQuad(o + glm::vec3(-4.f, 0, -13.f), { -1,0,0 }, 7.f, { 0,1,0 }, RH, { 0,0,-1 }, 3.5f, RH / 2.f);

    V.cornerWallA = buildQuad(o + glm::vec3(-1.f, 0, -13.f), { 0,0,-1 }, 3.f, { 0,1,0 }, RH, { -1,0,0 }, 1.5f, RH / 2.f);
    V.endWallA = buildQuad(o + glm::vec3(-11.f, 0, -16.f), { 0,0, 1 }, 3.f, { 0,1,0 }, RH, { 1,0,0 }, 1.5f, RH / 2.f);

    // ── Pasillo B – esquina NE, sale hacia +Z ─────────────────────────────────
    V.corBCenter = o + glm::vec3(COR_B_DX, 0.f, (RD / 2.f + cor_len / 2.f));
    V.corB       = buildCorridorZ(V.corBCenter, cor_len, CW, RH);
   
    // --- Codo B (Ahora gira hacia +X para alejarse) ---
    // Cambiamos el inicio de 4.f a 1.f y la dirección de {-1,0,0} a {1,0,0}
    V.corBTurn.floor = buildQuad(o + glm::vec3(1.f, 0, 13.f), { 1,0,0 }, 10.f, { 0,0, 1 }, 3.f, { 0,1,0 }, 5.f, 1.5f);
    V.corBTurn.ceiling = buildQuad(o + glm::vec3(1.f, RH, 16.f), { 1,0,0 }, 10.f, { 0,0,-1 }, 3.f, { 0,-1,0 }, 5.f, 1.5f);
    V.corBTurn.wLeft = buildQuad(o + glm::vec3(1.f, 0, 16.f), { 1,0,0 }, 10.f, { 0,1,0 }, RH, { 0,0,-1 }, 5.f, RH / 2.f);
    V.corBTurn.wRight = buildQuad(o + glm::vec3(4.f, 0, 13.f), { 1,0,0 }, 7.f, { 0,1,0 }, RH, { 0,0, 1 }, 3.5f, RH / 2.f);

    V.cornerWallB = buildQuad(o + glm::vec3(1.f, 0, 13.f), { 0,0, 1 }, 3.f, { 0,1,0 }, RH, { 1,0,0 }, 1.5f, RH / 2.f);
    V.endWallB = buildQuad(o + glm::vec3(11.f, 0, 16.f), { 0,0,-1 }, 3.f, { 0,1,0 }, RH, { -1,0,0 }, 1.5f, RH / 2.f);

    return V;
}*/

RoomVariant buildVariant(int variantIdx, AnomalyType anomaly)
{
    RoomVariant V;
    V.anomaly = anomaly;
    V.origin = glm::vec3(variantIdx * VARIANT_STRIDE, 0.0f, 0.0f);
    glm::vec3 o = V.origin;

    // ── Habitación central ────────────────────────────────────────────────────
    V.room = buildRoom(o, RW, RD, RH);

    float hw = RW / 2.f;
    float hd = RD / 2.f;
    float cor_len = CL - CW;
    float zDistNear = hd + cor_len;  // Inicio del codo
    float zDistFar = zDistNear + CW; // Fondo del codo

    // Posiciones exactas en X de las paredes de los pasillos
    float xLeftWall_A = -hw;
    float xRightWall_A = -hw + CW;
    float xRightWall_B = hw;
    float xLeftWall_B = hw - CW;

    float elbow_len = 10.0f; // La longitud de los codos laterales hacia X

    // ── Pasillo A – esquina SO, sale hacia –Z ─────────────────────────────────
    // AHORA SÍ ESTÁ EN LA IZQUIERDA (-X)
    V.corACenter = o + glm::vec3(xLeftWall_A + (CW / 2.f), 0.f, -(hd + cor_len / 2.f));
    V.corA = buildCorridorZ(V.corACenter, cor_len, CW, RH);

    // Codo A (gira hacia -X)
    V.corATurn.floor = buildQuad(o + glm::vec3(xRightWall_A, 0, -zDistNear), { -1,0,0 }, elbow_len, { 0,0,-1 }, CW, { 0,1,0 }, elbow_len / 2.f, CW / 2.f);
    V.corATurn.ceiling = buildQuad(o + glm::vec3(xRightWall_A, RH, -zDistFar), { -1,0,0 }, elbow_len, { 0,0, 1 }, CW, { 0,-1,0 }, elbow_len / 2.f, CW / 2.f);
    V.corATurn.wLeft = buildQuad(o + glm::vec3(xRightWall_A, 0, -zDistFar), { -1,0,0 }, elbow_len, { 0,1,0 }, RH, { 0,0, 1 }, elbow_len / 2.f, RH / 2.f);
    V.corATurn.wRight = buildQuad(o + glm::vec3(xLeftWall_A, 0, -zDistNear), { -1,0,0 }, elbow_len - CW, { 0,1,0 }, RH, { 0,0,-1 }, (elbow_len - CW) / 2.f, RH / 2.f);

    V.cornerWallA = buildQuad(o + glm::vec3(xRightWall_A, 0, -zDistNear), { 0,0,-1 }, CW, { 0,1,0 }, RH, { -1,0,0 }, CW / 2.f, RH / 2.f);
    V.endWallA = buildQuad(o + glm::vec3(xRightWall_A - elbow_len, 0, -zDistFar), { 0,0, 1 }, CW, { 0,1,0 }, RH, { 1,0,0 }, CW / 2.f, RH / 2.f);

    // ── Pasillo B – esquina NE, sale hacia +Z ─────────────────────────────────
    // AHORA SÍ ESTÁ EN LA DERECHA (+X)
    V.corBCenter = o + glm::vec3(xLeftWall_B + (CW / 2.f), 0.f, (hd + cor_len / 2.f));
    V.corB = buildCorridorZ(V.corBCenter, cor_len, CW, RH);

    // Codo B (gira hacia +X)
    V.corBTurn.floor = buildQuad(o + glm::vec3(xLeftWall_B, 0, zDistNear), { 1,0,0 }, elbow_len, { 0,0, 1 }, CW, { 0,1,0 }, elbow_len / 2.f, CW / 2.f);
    V.corBTurn.ceiling = buildQuad(o + glm::vec3(xLeftWall_B, RH, zDistFar), { 1,0,0 }, elbow_len, { 0,0,-1 }, CW, { 0,-1,0 }, elbow_len / 2.f, CW / 2.f);
    V.corBTurn.wLeft = buildQuad(o + glm::vec3(xLeftWall_B, 0, zDistFar), { 1,0,0 }, elbow_len, { 0,1,0 }, RH, { 0,0,-1 }, elbow_len / 2.f, RH / 2.f);
    V.corBTurn.wRight = buildQuad(o + glm::vec3(xRightWall_B, 0, zDistNear), { 1,0,0 }, elbow_len - CW, { 0,1,0 }, RH, { 0,0, 1 }, (elbow_len - CW) / 2.f, RH / 2.f);

    V.cornerWallB = buildQuad(o + glm::vec3(xLeftWall_B, 0, zDistNear), { 0,0, 1 }, CW, { 0,1,0 }, RH, { 1,0,0 }, CW / 2.f, RH / 2.f);
    V.endWallB = buildQuad(o + glm::vec3(xLeftWall_B + elbow_len, 0, zDistFar), { 0,0,-1 }, CW, { 0,1,0 }, RH, { -1,0,0 }, CW / 2.f, RH / 2.f);

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
