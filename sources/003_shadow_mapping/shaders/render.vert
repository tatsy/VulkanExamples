#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mvMat;
    mat4 normMat;
	mat4 mvpMatLightSpace;
	vec3 lightPos;
} ubo;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 f_posCameraSpace;
layout(location = 1) out vec3 f_normCameraSpace;
layout(location = 2) out vec3 f_lightPosCameraSpace;
layout(location = 3) out vec2 f_uv;
layout(location = 4) out vec4 f_posScreenLightSpace;

void main() {
    gl_Position = ubo.mvpMat * vec4(in_pos, 1.0);
	f_posCameraSpace = (ubo.mvMat * vec4(in_pos, 1.0)).xyz;
	f_normCameraSpace = (ubo.normMat * vec4(in_normal, 0.0)).xyz;
	f_lightPosCameraSpace = (ubo.mvMat * vec4(ubo.lightPos, 1.0)).xyz;
	f_uv = in_uv;
	f_posScreenLightSpace = ubo.mvpMatLightSpace * vec4(in_pos, 1.0);
}
