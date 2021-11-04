#version 450

layout(set = 0, binding = 0) uniform camera
{
    mat4 view;
    mat4 proj;
};

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 localPos;

void main()
{
    localPos = inPosition;
    gl_Position =  proj * view * vec4(localPos, 1.0);
}