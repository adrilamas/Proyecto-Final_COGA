#version 330 core

// ─── Salida ───────────────────────────────────────────────────────────────────
out vec4 FragColor;

// ─── Entradas interpoladas del vertex shader ─────────────────────────────────
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// ─── Textura y color de fallback ─────────────────────────────────────────────
uniform sampler2D uTexBase;
uniform bool      uUsarTextura;
uniform vec3      uColor;       // Color sólido cuando no hay textura

// ─── Cámara ───────────────────────────────────────────────────────────────────
uniform vec3 uViewPos;

// ─── Luz ambiente global ─────────────────────────────────────────────────────
// Simula la luz residual de un pasillo casi a oscuras.
// Se puede bajar a 0.02 para anomalías de "apagón total".
uniform vec3 uAmbient;

// ─── Luces de techo (Point Lights) ───────────────────────────────────────────
// Soportamos hasta 8 fluorescentes en el techo.
// En la práctica se usarán las que haya definidas en MAX_LIGHTS.
#define MAX_LIGHTS 16

struct PointLight {
    vec3  position;     // Posición en world-space (colgando del techo)
    vec3  color;        // Color de la bombilla (blanco-frío fluorescente)
    float intensity;    // [0.0 – 1.0]  0 = apagada, 1 = plena intensidad
                        // Úsalo para parpadeos: interpola entre 0 y 1 en la CPU
    // Coeficientes de atenuación cuadrática  1 / (Kc + Kl*d + Kq*d²)
    float Kc;           // Constante  (≈ 1.0)
    float Kl;           // Lineal     (≈ 0.14  para rango ~30 u)
    float Kq;           // Cuadrático (≈ 0.07  para rango ~30 u)
};

uniform PointLight uLights[MAX_LIGHTS];
uniform int        uNumLights;   // Cuántas luces están activas en esta escena

// ─── Función auxiliar: contribución Phong de una point light ─────────────────
vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos,
                    vec3 viewDir, vec3 baseColor)
{
    // Si la intensidad es cero (apagada o en parpadeo), no hay contribución
    if (light.intensity <= 0.0) return vec3(0.0);

    vec3  lightDir = normalize(light.position - fragPos);
    float dist     = length(light.position - fragPos);

    // Atenuación cuadrática
    float atten = 1.0 / (light.Kc + light.Kl * dist + light.Kq * dist * dist);

    // Componente difusa
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  diffuse = diff * light.color * baseColor;

    // Componente especular (Phong, shininess = 32 – adecuado para plástico/pintura)
    vec3  reflDir = reflect(-lightDir, norm);
    float spec    = pow(max(dot(viewDir, reflDir), 0.0), 32.0);
    vec3  specular = spec * light.color * 0.25;   // faros de techo no son muy especulares

    return (diffuse + specular) * atten * light.intensity;
}

void main()
{
    // ── Color base: textura o color sólido de fallback ────────────────────────
    vec4 sample   = uUsarTextura ? texture(uTexBase, TexCoord) : vec4(uColor, 1.0);
    vec3 baseColor = sample.rgb;
    float alpha    = sample.a;

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(uViewPos - FragPos);

    // ── Iluminación ambiente (nunca llega a cero para evitar negro puro) ──────
    vec3 result = uAmbient * baseColor;

    // ── Suma de todas las luces de techo ─────────────────────────────────────
    for (int i = 0; i < uNumLights && i < MAX_LIGHTS; i++) {
        result += calcPointLight(uLights[i], norm, FragPos, viewDir, baseColor);
    }

    FragColor = vec4(result, alpha);
}
