#version 450

// updated once per frame 
layout(set = 0, binding = 0) uniform FrameConstants {
    mat4 view;
    mat4 proj;
};

// updated once per drawcall
layout(set = 2, binding = 0) uniform Drawcall {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() 
{
    const mat4 mvp = proj * view * model;
    gl_Position = mvp * vec4(inPosition, 1.0);
    fragColor = (mvp * vec4(inNormal, 1.0)).xyz;
    fragTexCoord = inTexCoord;
}