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

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    vec2 uv = fragTexCoord;
    if (show_uv)
        outColor = vec4(uv, 0.5, 1.0);
    else
        outColor = texture(albedo, uv) * vec4(color, 1.0);
}