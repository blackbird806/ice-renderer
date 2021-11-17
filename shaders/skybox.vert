#version 450

layout(set = 0, binding = 0) uniform camera
{
    mat4 view;
    mat4 proj;
};
layout(set = 0, binding = 1) uniform sampler2D equirectangularMap;

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 localPos;

void main()
{
    localPos = inPosition;
    mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    vec4 clipPos = proj * rotView * vec4(localPos, 1.0);
    clipPos.y *= -1.0;
    gl_Position = clipPos.xyww;
}