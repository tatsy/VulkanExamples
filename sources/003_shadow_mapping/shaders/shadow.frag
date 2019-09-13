#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 f_posScreenSpace;

layout(location = 0) out vec4 out_color;

void main(void) {
    float depth = f_posScreenSpace.z / f_posScreenSpace.w;
    float dzdx = dFdx(depth);
    float dzdy = dFdy(depth);
    float momentum = depth * depth + 0.25 * (dzdx * dzdx + dzdy * dzdy);
	out_color = vec4(depth, momentum, 0.0, 1.0);
}
