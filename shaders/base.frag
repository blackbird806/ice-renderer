#version 450

struct Light {
    vec3 pos;
    float padding_00;
    vec3 dir;
    float padding_01;
    vec3 intensity;
};

layout(set = 0, binding = 0) uniform Lights {
    Light lights[16];
    uint nlights;
};

layout(set = 1, binding = 0) uniform PipelineConstants {
    mat4 view;
    mat4 proj;
    float time;
};

layout(set = 2, binding = 0) uniform sampler2D albedoSamplers[64];
layout(set = 2, binding = 1) uniform sampler2D normalSamplers[64];

layout(set = 3, binding = 0) uniform Material {
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
    outColor = texture(albedoSamplers[albedoId], fragTexCoord) * vec4(lights[0].intensity, 1.0);
}