#version 450

layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in vec3 Normal;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model, view, projection;
} ubo;


layout(binding = 1) uniform HasTexture {
    int hasTexture;
} hasTexture;

layout(binding = 2) uniform sampler2D texture_diffuse;


void main() {
    if (hasTexture.hasTexture > 0) {
        FragColor = texture(texture_diffuse, TexCoords);
    } else {    // No Texture, the same as solid
        vec3 lightDir = normalize(vec3(ubo.view * vec4(-0.2, -1.0, -0.3, 0.0)));
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 ambient= vec3(0.3, 0.3, 0.3);
        vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
        vec3 result = vec3(0.6, 0.6, 0.6) * (ambient + diffuse);
        FragColor = vec4(result, 1.0);
    }
    float u = TexCoords.x;
    float v = TexCoords.y;
    if(u > 0.95f) u = 0.75f + (u - 0.95f) * 5.0f;
    else if(u < 0.05f) u = u * 5.0f;
    else u = 0.25f + (u - 0.25f) * 9.0f / 14.0f;
    if(v > 0.95f) v = 0.75f + (v - 0.95f) * 5.0f;
    else if(v < 0.05f) v = v * 5.0f;
    else v = 0.25f + (v - 0.25f) * 9.0f / 14.0f;
    FragColor = vec4(u, v, 0.0, 1.0);
}
