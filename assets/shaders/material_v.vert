#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location = 0) out vec2 TexCoords;
layout(location = 1) out vec3 FragPos;
layout(location = 2) out vec3 Normal;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model, view, projection;
} ubo;

void main() {
    TexCoords = aTexCoords;
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(ubo.model))) * aNormal;
    gl_Position = ubo.projection * ubo.view * vec4(FragPos, 1.0);
}
