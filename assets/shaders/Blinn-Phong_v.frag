#version 450

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model, view, projection;
} ubo;

void main() {
    vec3 lightDir = normalize(vec3(ubo.view * vec4(-0.2, -1.0, -0.3, 0.0)));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient= vec3(0.3, 0.3, 0.3);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 result = vec3(0.6, 0.6, 0.6) * (ambient + diffuse);
    FragColor = vec4(result, 1.0);
}