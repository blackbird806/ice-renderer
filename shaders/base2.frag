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
    float padding_1, padding_2, padding_3;
    vec3 ambient;
    float padding_4;
    vec3 diffuse;
    float padding_5;
    vec3 specular;
};

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(ambient, 1.0);
}