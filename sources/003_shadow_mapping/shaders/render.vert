#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(binding = 0) uniform UniformBufferObjectVS {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normal;
    mat4 depthBiasMVP;
    vec4 lightPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 f_posView;
layout(location = 1) out vec3 f_normView;
layout(location = 2) out vec2 f_texCoord;
layout(location = 3) out vec3 f_lightPosView;
layout(location = 4) out vec4 f_posScreenLightSpace;

void main() {
    mat4 mvMat = ubo.view * ubo.model;
    mat4 mvpMat = ubo.proj * mvMat;

    gl_Position = mvpMat * vec4(inPosition, 1.0);
    f_posView = (mvMat * vec4(inPosition, 1.0)).xyz;
    f_normView = (ubo.normal * vec4(inNormal, 0.0)).xyz;
    f_lightPosView = (ubo.view * vec4(ubo.lightPos.xyz, 1.0)).xyz;

    f_posScreenLightSpace = ubo.depthBiasMVP * vec4(inPosition, 1.0);

    f_texCoord = inTexCoord;
}
