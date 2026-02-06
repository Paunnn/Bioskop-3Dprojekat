#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform bool uUseLighting;
uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform float uAlpha;

void main() {
    vec3 baseColor = uUseTexture ? texture(uTexture, TexCoord).rgb : uColor;
    float alpha = uAlpha;

    if (uUseTexture) {
        alpha = texture(uTexture, TexCoord).a * uAlpha;
    }

    if (uUseLighting) {
        float ambientStrength = 0.35;
        vec3 ambient = ambientStrength * uLightColor * baseColor;

        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(uLightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        float wrapDiff = max(dot(norm, lightDir) * 0.5 + 0.5, 0.0);
        vec3 diffuse = mix(diff, wrapDiff, 0.3) * uLightColor * baseColor;

        float specularStrength = 0.25;
        vec3 viewDir = normalize(uViewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
        vec3 specular = specularStrength * spec * uLightColor;

        FragColor = vec4(ambient + diffuse + specular, alpha);
    } else {
        FragColor = vec4(baseColor, alpha);
    }
}
