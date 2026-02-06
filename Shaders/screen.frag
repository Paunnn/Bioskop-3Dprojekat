#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uTexture;
uniform bool uUseTexture;
uniform vec3 uEmissionColor;
uniform float uEmissionStrength;

void main() {
    vec3 color;

    if (uUseTexture) {
        color = texture(uTexture, TexCoord).rgb;
    } else {
        color = uEmissionColor;
    }

    // Add emission effect
    vec3 emission = color * uEmissionStrength;
    FragColor = vec4(color + emission, 1.0);
}
