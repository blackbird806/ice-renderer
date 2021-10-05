#version 450

// updated once per frame (descriptor set inside vkContext ?)
// layout(set = 0, binding = 0) uniform viewProj {
//     mat4 view;
//     mat4 proj;
//     float time ?
// };

// updated once per Material "bucket"
// layout(set = 1, binding = 0) uniform material {
//     sampler texture;
//     float shiniess;
//     ...
// };

// updated once per model
// layout(set = 2, binding = 0) uniform model {
//     mat4 model;
// };

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = (ubo.proj * ubo.view * ubo.model * vec4(inNormal, 1.0)).xyz;
    fragTexCoord = inTexCoord;
}