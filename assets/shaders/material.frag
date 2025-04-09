#version 450 core

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform sampler2D texture_diffuse;
uniform bool hasTexture;
uniform mat4 view;

void main() {

    if (hasTexture) {
        FragColor = texture(texture_diffuse, TexCoords);
    } else {    // No Texture, the same as solid
        vec3 lightDir = normalize(vec3(view * vec4(-0.2, -1.0, -0.3, 0.0)));
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 ambient= vec3(0.3, 0.3, 0.3);
        vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
        vec3 result = vec3(0.6, 0.6, 0.6) * (ambient + diffuse);
        FragColor = vec4(result, 1.0);
    }
    
}
