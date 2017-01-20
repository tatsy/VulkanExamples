#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 f_posScreen;

void main(void) {
    gl_FragDepth = f_posScreen.z / f_posScreen.w;
}
