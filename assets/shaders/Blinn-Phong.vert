#version 450 core

// 检测是否是 Vulkan（GL_SPIRV 是 Vulkan 的宏）
#if defined(GL_SPIRV)
#define VULKAN 1
#else
#define VULKAN 0
#endif

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

#if VULKAN
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;
#else
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
#endif

void main() {
#if VULKAN
    mat4 model = ubo.model;
    mat4 view = ubo.view;
    mat4 projection = ubo.projection;
#endif

    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}