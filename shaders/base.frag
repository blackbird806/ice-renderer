#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoSamplers;

// updated once per Material "bucket"
layout(set = 2, binding = 0) uniform Material {
    float brightness;
    vec3 color;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(albedoSamplers, fragTexCoord);
    // outColor = vec4(color, 1.0);
}