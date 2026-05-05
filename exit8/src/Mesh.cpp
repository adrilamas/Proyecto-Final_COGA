#include "Mesh.h"

Mesh buildQuad(glm::vec3 o,
               glm::vec3 r, float rl,
               glm::vec3 u, float ul,
               glm::vec3 n,
               float su, float sv)
{
    // 6 vértices × 8 floats (pos + normal + uv)
    float v[48];

    glm::vec3 p0 = o;
    glm::vec3 p1 = o + r * rl;
    glm::vec3 p2 = o + r * rl + u * ul;
    glm::vec3 p3 = o + u * ul;

    auto fill = [&](int i, glm::vec3 p, float tu, float tv) {
        int b = i * 8;
        v[b]   = p.x;  v[b+1] = p.y;  v[b+2] = p.z;
        v[b+3] = n.x;  v[b+4] = n.y;  v[b+5] = n.z;
        v[b+6] = tu;   v[b+7] = tv;
    };

    fill(0, p0, 0,  0 );
    fill(1, p1, su, 0 );
    fill(2, p2, su, sv);
    fill(3, p2, su, sv);
    fill(4, p3, 0,  sv);
    fill(5, p0, 0,  0 );

    Mesh m;
    m.count = 6;
    glGenVertexArrays(1, &m.VAO);
    glGenBuffers(1, &m.VBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return m;
}
