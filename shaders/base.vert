#version 450

layout(set = 0, binding = 0) uniform Parameters {
    mat4 view;
    mat4 proj;

    vec3 color;
    bool show_uv;
    vec2 t;
};

layout(set = 0, binding = 1) uniform sampler2D albedo;
layout(set = 0, binding = 2) uniform sampler2D normal;
layout(set = 0, binding = 3) uniform sampler2D roughness;

layout(set = 1, binding = 0) uniform Drawcall {
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