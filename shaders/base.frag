#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoSamplers[64];

// updated once per Material "bucket"
layout(set = 2, binding = 1) uniform Material {
    int textureId;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(albedoSamplers[textureId], fragTexCoord);
    // outColor = vec4(fragColor, 1.0);
}