#version 330 core
out vec4 FragColor;

uniform float uTime;
uniform float uAlpha;

// Función generadora de ruido pseudoaleatorio
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    // 1. Pixelación: Agrupamos los píxeles en bloques de 25x25
    vec2 blockUv = floor(gl_FragCoord.xy / 25.0); 

    // 2. Desgarro horizontal (Tearing): Dividimos la pantalla en franjas gruesas
    float band = floor(gl_FragCoord.y / 60.0); 
    // Calculamos un desplazamiento aleatorio para esta franja en este instante
    float tearOffset = rand(vec2(band, floor(uTime * 15.0))) * 2.0 - 1.0; 
    
    // Si la probabilidad de desgarro es alta, desplazamos el bloque en X violentamente
    if (abs(tearOffset) > 0.6) {
        blockUv.x += floor(tearOffset * 15.0);
    }

    // 3. Ruido digital: Generamos el ruido basado en los bloques y un tiempo acelerado
    float noise = rand(blockUv * (sin(uTime * 10.0) + 2.0));
    
    // Convertimos el ruido suave en cortes bruscos digitales (0.0 o 1.0)
    float intensity = step(0.6, noise); 

    // 4. Paleta de colores del Glitch
    vec3 glitchColor;
    if (intensity > 0.5) {
        glitchColor = vec3(0.85, 0.85, 0.90);  // Bloques de estática casi blanca
    } else if (rand(blockUv - uTime) > 0.8) {
        glitchColor = vec3(0.05, 0.05, 0.05);  // Zonas de vacío negro
    } else {
        glitchColor = vec3(0.7, 0.0, 0.05);    // El fondo base: rojo oscuro de alarma
    }

    // 5. Destellos de aberración cromática (falsos píxeles azules/verdes corruptos)
    if (rand(vec2(gl_FragCoord.y, uTime)) > 0.97) {
        glitchColor.b += 0.6; 
        glitchColor.r -= 0.4;
    }

    // 6. Líneas de escaneo tipo monitor viejo (Scanlines)
    float scanline = sin(gl_FragCoord.y * 1.5) * 0.15;
    glitchColor -= scanline;

    // 7. Transparencia fragmentada: 
    // Hacemos que el alpha dependa del ruido, así el glitch tiene "agujeros" 
    // por los que se ve el pasillo real nítidamente, aumentando la sensación de desgarro.
    float finalAlpha = uAlpha * (0.3 + intensity * 0.5);

    FragColor = vec4(glitchColor, finalAlpha);
}