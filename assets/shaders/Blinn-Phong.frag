#version 450 core

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform mat4 view;

void main() {
    vec3 lightDir = normalize(vec3(view * vec4(-0.2, -1.0, -0.3, 0.0)));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient= vec3(0.3, 0.3, 0.3);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 result = vec3(0.6, 0.6, 0.6) * (ambient + diffuse);
    FragColor = vec4(result, 1.0);
}
