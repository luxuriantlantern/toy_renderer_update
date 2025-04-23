#version 450 core

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

#ifdef VULKAN
// Vulkan 需要显式 binding
layout(binding = 0) uniform ViewMatrix {
    mat4 view;
} ubo_view;
#else
// OpenGL 使用传统 uniform 变量
uniform mat4 view;
#endif

void main() {
    // 统一访问 view 矩阵（Vulkan 用 ubo_view.view，OpenGL 直接用 view）
#ifdef VULKAN
    mat4 viewMatrix = ubo_view.view;
#else
    mat4 viewMatrix = view;
#endif

    // 计算光照
    vec3 lightDir = normalize(vec3(viewMatrix * vec4(-0.2, -1.0, -0.3, 0.0)));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 result = vec3(0.6, 0.6, 0.6) * (ambient + diffuse);
    FragColor = vec4(result, 1.0);
}