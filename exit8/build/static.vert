#version 330 core
// Truco matemático para dibujar un cuadrado que cubra toda la pantalla usando solo 3 vértices
void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    gl_Position = vec4(x, y, 0.0, 1.0);
}