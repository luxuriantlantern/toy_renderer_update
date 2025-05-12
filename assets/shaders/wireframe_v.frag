#version 450 core

layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in vec3 Normal;
layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(0.5, 1.0, 1.0, 1.0);
}
