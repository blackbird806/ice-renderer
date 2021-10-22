#version 450

layout(set = 0, binding = 0) uniform PipelineConstants {
    mat4 view;
    mat4 proj;
    float time;
};

layout(set = 1, binding = 0) uniform sampler2D albedoSamplers[64];
layout(set = 1, binding = 1) uniform sampler2D normalSamplers[64];

layout(set = 2, binding = 0) uniform Material {
    vec3 color;
    float padding_0;
    int albedoId;
};

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    vec3 ncolor = color;
    ncolor.r = sin(time/10.0);
    ncolor.g = cos(time/10.0);
    ncolor.b = cos(time/10.0);
    outColor = texture(albedoSamplers[albedoId], fragTexCoord) * vec4(ncolor, 1.0);
}