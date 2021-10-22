#version 450

layout(set = 0, binding = 0) uniform PipelineConstants {
    mat4 view;
    mat4 proj;
    float time;
};

layout(set = 3, binding = 0) uniform Drawcall {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() 
{
    const mat4 mvp = proj * view * model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}