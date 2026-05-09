#version 330 core
out vec4 FragColor;
uniform float uTime;
uniform float uAlpha;

float rand(vec2 co){ return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453); }

void main() {
    vec2 uv = gl_FragCoord.xy;
    // Pixelación más fina para la victoria
    vec2 blockUv = floor(uv / 8.0);
    float noise = rand(blockUv + uTime);
    
    // Color verde esmeralda con destellos blancos
    vec3 color = vec3(0.1, 0.8, 0.3) * noise;
    if (noise > 0.95) color = vec3(1.0, 1.0, 1.0); // Chispas blancas
    
    // Pulso de luz central
    float dist = distance(gl_FragCoord.xy / vec2(1280, 720), vec2(0.5));
    color += (1.0 - dist) * vec3(0.0, 0.4, 0.1);

    FragColor = vec4(color, uAlpha * 0.7);
}