#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
} ubo;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 f_posScreenSpace;

void main() {
    gl_Position = ubo.mvpMat * vec4(in_pos, 1.0);
    f_posScreenSpace = gl_Position;
}
