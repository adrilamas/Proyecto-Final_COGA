#include "RoomVariant.h"
#include "Constants.h"
#include <iostream>

// =============================================================================
//  Geometría del layout (vista desde arriba, lado A hacia –Z):
//
//   X →
//   ↑ Z=0 (centro sala)
//
//   La sala ocupa X ∈ [–RW/2, +RW/2], Z ∈ [–RD/2, +RD/2]
//
//   Corredor A (lado oeste, sale hacia –Z):
//     Tramo A1: X ∈ [–RW/2, –RW/2+CW],  Z ∈ [–RD/2–CL1, –RD/2]
//     Giro  A1: cuadrado CW×CW en la esquina X∈[–RW/2,–RW/2+CW], Z∈[–RD/2–CL1–CW, –RD/2–CL1]
//     Tramo A2: X ∈ [–RW/2–CL2, –RW/2], Z ∈ [–RD/2–CL1–CW, –RD/2–CL1]   (hacia –X)
//     Giro  A2: cuadrado CW×CW en X∈[–RW/2–CL2–CW, –RW/2–CL2], Z∈[–RD/2–CL1–CW, –RD/2–CL1]
//     Tramo A3: X ∈ [–RW/2–CL2–CW, –RW/2–CL2], Z ∈ [–RD/2–CL1–CW–CL3, –RD/2–CL1–CW]
//     TRIGGER a Z = –(RD/2 + CL1 + CW + CL3/2)
//
//   Corredor B es el espejo exacto hacia +Z (esquina este).
// =============================================================================

// Alias útiles
static inline float hw() { return RW / 2.f; }
static inline float hd() { return RD / 2.f; }

// =============================================================================
RoomVariant buildVariant(int variantIdx, AnomalyType anomaly)
// =============================================================================
{
    RoomVariant V;
    V.anomaly = anomaly;
    V.origin  = glm::vec3(variantIdx * VARIANT_STRIDE, 0.f, 0.f);
    glm::vec3 o = V.origin;

    const float HW = hw();
    const float HD = hd();

    // ── Sala principal ────────────────────────────────────────────────────────
    V.room = buildRoom(o, RW, RD, RH);

    // =========================================================================
    //  LADO A  (corredor en la esquina OESTE–SUR, sale hacia –Z)
    // =========================================================================
    //
    //  Columna X del corredor A: [–HW, –HW+CW]
    //  Z decrece conforme avanzamos.
    //
    const float axL = -HW;          // pared izquierda del corredor A (oeste)
    const float axR = -HW + CW;     // pared derecha  del corredor A (interior)

    // --- Tramo A1 (hacia –Z) -------------------------------------------------
    // Z ∈ [–HD–CL1, –HD]
    float zA1near = -HD;
    float zA1far  = -HD - CL1;
    glm::vec3 cA1center = o + glm::vec3((axL+axR)/2.f, 0.f, (zA1near+zA1far)/2.f);
    V.corA1 = buildCorridorZ(cA1center, CL1, CW, RH);
    V.corA1Center = cA1center;

    // --- Giro A1 (codo 90° de –Z hacia –X) ----------------------------------
    // Ocupa X ∈ [axL, axR], Z ∈ [zA1far–CW, zA1far]
    float zA1c_near = zA1far;
    float zA1c_far  = zA1far - CW;  // el codo se extiende CW en Z
    // Suelo del codo
    V.elbowA1floor = buildQuad(
        o + glm::vec3(axL,  0.f, zA1c_near),
        {1,0,0}, CW, {0,0,-1}, CW, {0,1,0}, CW/2.f, CW/2.f);
    // Techo del codo
    V.elbowA1ceil = buildQuad(
        o + glm::vec3(axL, RH, zA1c_far),
        {1,0,0}, CW, {0,0, 1}, CW, {0,-1,0}, CW/2.f, CW/2.f);
    // Pared exterior del codo (la que queda al sur, Z = zA1c_far)
    V.elbowA1wallOuter = buildQuad(
        o + glm::vec3(axL,  0.f, zA1c_far),
        {1,0,0}, CW, {0,1,0}, RH, {0,0, 1}, CW/2.f, RH/2.f);
    // Pared interior del codo (la que queda al este, X = axR) – sólo el trozo del giro
    V.elbowA1wallInner = buildQuad(
        o + glm::vec3(axR,  0.f, zA1c_far),
        {0,0, 1}, CW, {0,1,0}, RH, {-1,0,0}, CW/2.f, RH/2.f);

    // --- Tramo A2 (hacia –X) -------------------------------------------------
    // X ∈ [axL–CL2, axL],  Z ∈ [zA1c_far, zA1c_near]  (altura fija)
    float xA2far  = axL - CL2;
    float xA2near = axL;
    glm::vec3 cA2center = o + glm::vec3((xA2far+xA2near)/2.f, 0.f, (zA1c_far+zA1c_near)/2.f);
    // buildCorridorZ trabaja en Z; aquí el corredor va en X, así que lo
    // construimos como un corredor en X usando buildQuad directamente.
    {
        float len  = CL2;
        float w    = CW;
        float zLo  = zA1c_far;
        float zHi  = zA1c_near;
        float xNear= xA2near;
        float xFar = xA2far;
        // Suelo: corre en X desde xFar hasta xNear, ancho CW en Z
        V.corA2.floor = buildQuad(
            o + glm::vec3(xFar, 0.f, zLo),
            {1,0,0}, len, {0,0,1}, w, {0,1,0}, len/2.f, w/2.f);
        // Techo
        V.corA2.ceiling = buildQuad(
            o + glm::vec3(xFar, RH, zHi),
            {1,0,0}, len, {0,0,-1}, w, {0,-1,0}, len/2.f, w/2.f);
        // Pared norte (Z = zHi = zA1c_near): cara interior –Z
        V.corA2.wLeft = buildQuad(
            o + glm::vec3(xFar, 0.f, zHi),
            {1,0,0}, len, {0,1,0}, RH, {0,0,-1}, len/2.f, RH/2.f);
        // Pared sur (Z = zLo = zA1c_far): cara interior +Z
        V.corA2.wRight = buildQuad(
            o + glm::vec3(xFar, 0.f, zLo),
            {1,0,0}, len, {0,1,0}, RH, {0,0, 1}, len/2.f, RH/2.f);
    }
    V.corA2Center = cA2center;

    // --- Giro A2 (codo 90° de –X vuelve a –Z) --------------------------------
    // Ocupa X ∈ [xA2far–CW, xA2far], Z ∈ [zA1c_far, zA1c_near]
    float xA2c_near = xA2far;
    float xA2c_far  = xA2far - CW;
    float zA2c_near = zA1c_near;  // = zA1far
    float zA2c_far  = zA1c_far;   // = zA1far–CW
    V.elbowA2floor = buildQuad(
        o + glm::vec3(xA2c_far, 0.f, zA2c_far),
        {1,0,0}, CW, {0,0,1}, CW, {0,1,0}, CW/2.f, CW/2.f);
    V.elbowA2ceil = buildQuad(
        o + glm::vec3(xA2c_far, RH, zA2c_near),
        {1,0,0}, CW, {0,0,-1}, CW, {0,-1,0}, CW/2.f, CW/2.f);
    // Pared norte del codo (Z = zA2c_near): cierra el corredor A2 por el norte
    V.elbowA2wallOuter = buildQuad(
        o + glm::vec3(xA2c_far, 0.f, zA2c_near),
        {1,0,0}, CW, {0,1,0}, RH, {0,0,-1}, CW/2.f, RH/2.f);
    // Pared oeste del codo (X = xA2c_far): cierra el codo por la izquierda
    V.elbowA2wallInner = buildQuad(
        o + glm::vec3(xA2c_far, 0.f, zA2c_far),
        {0,0,1}, CW, {0,1,0}, RH, {1,0,0}, CW/2.f, RH/2.f);

    // --- Tramo A3 (hacia –Z de nuevo) ----------------------------------------
    // X ∈ [xA2c_far, xA2c_near], Z ∈ [zA2c_far–CL3, zA2c_far]
    float zA3near = zA2c_far;
    float zA3far  = zA2c_far - CL3;
    glm::vec3 cA3center = o + glm::vec3((xA2c_far+xA2c_near)/2.f, 0.f, (zA3near+zA3far)/2.f);
    V.corA3 = buildCorridorZ(cA3center, CL3, CW, RH);
    V.corA3Center = cA3center;

    // Pared de cierre al fondo del tramo A3
    V.endWallA = buildQuad(
        o + glm::vec3(xA2c_far, 0.f, zA3far),
        {1,0,0}, CW, {0,1,0}, RH, {0,0,1}, CW/2.f, RH/2.f);

    // Trigger: a mitad del tramo A3 (en Z absoluto)
    // Como está desplazado en X, usamos solo la coordenada Z del trigger.
    float trigZ_A = zA3near + (zA3far - zA3near) * 0.5f; // negativo
    // triggerDist es la distancia desde origin.z al punto de trigger en Z
    V.triggerDist = -trigZ_A + o.z; // distancia positiva

    // =========================================================================
    //  LADO B  (corredor en la esquina ESTE–NORTE, sale hacia +Z)
    //  Espejo exacto de A respecto al centro de la sala.
    // =========================================================================
    //
    //  Columna X del corredor B: [+HW–CW, +HW]
    //
    const float bxL = HW - CW;
    const float bxR = HW;

    // --- Tramo B1 (hacia +Z) -------------------------------------------------
    float zB1near = HD;
    float zB1far  = HD + CL1;
    glm::vec3 cB1center = o + glm::vec3((bxL+bxR)/2.f, 0.f, (zB1near+zB1far)/2.f);
    V.corB1 = buildCorridorZ(cB1center, CL1, CW, RH);
    V.corB1Center = cB1center;

    // --- Giro B1 (codo 90° de +Z hacia +X) ----------------------------------
    float zB1c_near = zB1far;
    float zB1c_far  = zB1far + CW;
    V.elbowB1floor = buildQuad(
        o + glm::vec3(bxL, 0.f, zB1c_near),
        {1,0,0}, CW, {0,0,1}, CW, {0,1,0}, CW/2.f, CW/2.f);
    V.elbowB1ceil = buildQuad(
        o + glm::vec3(bxL, RH, zB1c_far),
        {1,0,0}, CW, {0,0,-1}, CW, {0,-1,0}, CW/2.f, CW/2.f);
    // Pared exterior (Z = zB1c_far, cara –Z)
    V.elbowB1wallOuter = buildQuad(
        o + glm::vec3(bxL, 0.f, zB1c_far),
        {1,0,0}, CW, {0,1,0}, RH, {0,0,-1}, CW/2.f, RH/2.f);
    // Pared interior (X = bxL, cara +X)
    V.elbowB1wallInner = buildQuad(
        o + glm::vec3(bxL, 0.f, zB1c_near),
        {0,0,1}, CW, {0,1,0}, RH, {1,0,0}, CW/2.f, RH/2.f);

    // --- Tramo B2 (hacia +X) -------------------------------------------------
    float xB2near = bxR;
    float xB2far  = bxR + CL2;
    {
        float len  = CL2;
        float w    = CW;
        float zLo  = zB1c_near;
        float zHi  = zB1c_far;
        V.corB2.floor = buildQuad(
            o + glm::vec3(xB2near, 0.f, zLo),
            {1,0,0}, len, {0,0,1}, w, {0,1,0}, len/2.f, w/2.f);
        V.corB2.ceiling = buildQuad(
            o + glm::vec3(xB2near, RH, zHi),
            {1,0,0}, len, {0,0,-1}, w, {0,-1,0}, len/2.f, w/2.f);
        // Pared sur (Z = zLo): cara interior +Z
        V.corB2.wLeft = buildQuad(
            o + glm::vec3(xB2near, 0.f, zLo),
            {1,0,0}, len, {0,1,0}, RH, {0,0, 1}, len/2.f, RH/2.f);
        // Pared norte (Z = zHi): cara interior –Z
        V.corB2.wRight = buildQuad(
            o + glm::vec3(xB2near, 0.f, zHi),
            {1,0,0}, len, {0,1,0}, RH, {0,0,-1}, len/2.f, RH/2.f);
    }
    V.corB2Center = o + glm::vec3((xB2near+xB2far)/2.f, 0.f, (zB1c_near+zB1c_far)/2.f);

    // --- Giro B2 (codo 90° de +X vuelve a +Z) --------------------------------
    float xB2c_near = xB2far;
    float xB2c_far  = xB2far + CW;
    float zB2c_low  = zB1c_near;
    float zB2c_high = zB1c_far;
    V.elbowB2floor = buildQuad(
        o + glm::vec3(xB2c_near, 0.f, zB2c_low),
        {1,0,0}, CW, {0,0,1}, CW, {0,1,0}, CW/2.f, CW/2.f);
    V.elbowB2ceil = buildQuad(
        o + glm::vec3(xB2c_near, RH, zB2c_high),
        {1,0,0}, CW, {0,0,-1}, CW, {0,-1,0}, CW/2.f, CW/2.f);
    // Pared sur del codo (Z = zB2c_low): cara interior +Z
    V.elbowB2wallOuter = buildQuad(
        o + glm::vec3(xB2c_near, 0.f, zB2c_low),
        {1,0,0}, CW, {0,1,0}, RH, {0,0, 1}, CW/2.f, RH/2.f);
    // Pared este del codo (X = xB2c_far): cierra el codo por la derecha
    V.elbowB2wallInner = buildQuad(
        o + glm::vec3(xB2c_far, 0.f, zB2c_high),
        {0,0,-1}, CW, {0,1,0}, RH, {-1,0,0}, CW/2.f, RH/2.f);

    // --- Tramo B3 (hacia +Z de nuevo) ----------------------------------------
    float zB3near = zB2c_high;
    float zB3far  = zB2c_high + CL3;
    glm::vec3 cB3center = o + glm::vec3((xB2c_near+xB2c_far)/2.f, 0.f, (zB3near+zB3far)/2.f);
    V.corB3 = buildCorridorZ(cB3center, CL3, CW, RH);
    V.corB3Center = cB3center;

    // Pared de cierre al fondo del tramo B3
    V.endWallB = buildQuad(
        o + glm::vec3(xB2c_near, 0.f, zB3far),
        {1,0,0}, CW, {0,1,0}, RH, {0,0,-1}, CW/2.f, RH/2.f);

    // Comprobamos consistencia: triggerDist debe ser la misma para A y B
    float trigZ_B = zB3near + (zB3far - zB3near) * 0.5f; // positivo
    // triggerDist ya calculado por A; verificamos coincidencia aproximada
    // (son geométricamente simétricos así que deben ser iguales)

    // ── Lámparas LED (Tubos lineales) ─────────────────────────────────────────
    auto addFixture = [&](glm::vec3 p, bool alongZ, bool isRoom) {
        float sizeX = alongZ ? 2.4f : 0.3f;
        float sizeZ = alongZ ? 0.3f : 2.4f;
        float H     = 0.15f; 

        float xMin = p.x - sizeX / 2.0f;
        float xMax = p.x + sizeX / 2.0f;
        float zMin = p.z - sizeZ / 2.0f;
        float zMax = p.z + sizeZ / 2.0f;
        float yB   = RH - H; 

        V.lightCasings.push_back(buildQuad({xMin, yB, zMax}, { 1, 0, 0}, sizeX, {0, 1, 0}, H, { 0, 0,  1}, 1, 1));
        V.lightCasings.push_back(buildQuad({xMax, yB, zMin}, {-1, 0, 0}, sizeX, {0, 1, 0}, H, { 0, 0, -1}, 1, 1));
        V.lightCasings.push_back(buildQuad({xMax, yB, zMax}, { 0, 0,-1}, sizeZ, {0, 1, 0}, H, { 1, 0,  0}, 1, 1));
        V.lightCasings.push_back(buildQuad({xMin, yB, zMin}, { 0, 0, 1}, sizeZ, {0, 1, 0}, H, {-1, 0,  0}, 1, 1));

        auto ledMesh = buildQuad({xMin, yB, zMin}, {1, 0, 0}, sizeX, {0, 0, 1}, sizeZ, {0, -1, 0}, 1, 1);
        
        // Separamos la malla de luz dependiendo de la zona
        if (isRoom) V.roomLEDs.push_back(ledMesh);
        else        V.corridorLEDs.push_back(ledMesh);
    };

    // Sala Principal
    for (auto& p : roomLightPositions(o)) addFixture(p, false, true);
    
    // Corredores Lado A
    for (auto& p : corridorLightPositions(V.corA1Center, CL1, true))  addFixture(p, true, false);
    for (auto& p : corridorLightPositions(V.corA2Center, CL2, false)) addFixture(p, false, false);
    for (auto& p : corridorLightPositions(V.corA3Center, CL3, true))  addFixture(p, true, false);
    
    // Corredores Lado B
    for (auto& p : corridorLightPositions(V.corB1Center, CL1, true))  addFixture(p, true, false);
    for (auto& p : corridorLightPositions(V.corB2Center, CL2, false)) addFixture(p, false, false);
    for (auto& p : corridorLightPositions(V.corB3Center, CL3, true))  addFixture(p, true, false);

    if (variantIdx == 0) {
        std::cout << "\n[=== GEOMETRIA (Variante 0) ===]\n"
                  << " Sala: X[" << -HW << "," << HW << "]  Z[" << -HD << "," << HD << "]\n"
                  << " Corredor A: columna X[" << axL << "," << axR << "]\n"
                  << "   Tramo A1: Z[" << zA1far  << "," << zA1near  << "]\n"
                  << "   Tramo A2: X[" << xA2far  << "," << xA2near << "]\n"
                  << "   Tramo A3: Z[" << zA3far  << "," << zA3near << "]\n"
                  << " Trigger A en Z=" << trigZ_A + o.z << " (dist=" << V.triggerDist << ")\n"
                  << " Corredor B: columna X[" << bxL << "," << bxR << "]\n"
                  << "   Tramo B3: Z[" << zB3near << "," << zB3far  << "]\n"
                  << " Trigger B en Z=" << trigZ_B + o.z << "\n"
                  << "===============================\n\n";
    }

    return V;
}

// =============================================================================
std::vector<glm::vec3> roomLightPositions(glm::vec3 origin)
// =============================================================================
{
    std::vector<glm::vec3> pts;
    float y = RH - 0.4f;
    int n = 2; // Forzamos a que haya exactamente 2 lámparas
    for (int i = 0; i < n; ++i) {
        float t = (i + 0.5f) / n;           
        float off = (t - 0.5f) * RW;
        glm::vec3 p = origin;
        p.y += y;
        p.x += off;  // Se distribuirán en X = -5 y X = +5
        pts.push_back(p);
    }
    return pts;
}

// =============================================================================
std::vector<glm::vec3> corridorLightPositions(glm::vec3 center, float len, bool alongZ)
// =============================================================================
{
    float y = RH - 0.4f;
    std::vector<glm::vec3> pts;
    int n = glm::max(2, (int)(len / 5.f)); // una luz cada ~5 unidades
    for (int i = 0; i < n; ++i) {
        float t = (i + 0.5f) / n;           // mitad de cada segmento
        float off = (t - 0.5f) * len;
        glm::vec3 p = center;
        p.y = center.y + y;                 // elevamos a la altura del techo
        if (alongZ) p.z += off;
        else        p.x += off;
        pts.push_back(p);
    }
    return pts;
}
