#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 f_posView;
layout(location = 1) in vec3 f_normView;
layout(location = 2) in vec3 f_lightPosView;
layout(location = 3) in vec4 f_posScreenLightSpace;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D u_depthMap;

float shadowMap() {
    vec4 P = f_posScreenLightSpace / f_posScreenLightSpace.w;
    float zValue = P.z;
    vec2 texCoord = P.xy * 0.5 + 0.5;
    float dValue = texture(u_depthMap, texCoord).r;

    if (dValue < zValue) {
        return 0.5;
    }
    return 1.0;
}

void main() {
    vec3 V = normalize(-f_posView);
    vec3 N = normalize(f_normView);
    vec3 L = normalize(f_lightPosView - f_posView);
    vec3 H = normalize(V + L);

    float ndotl = max(0.0, dot(N, L));
    float ndoth = max(0.0, dot(N, H));

    float value = 0.1 + 0.5 * ndotl + 0.6 * pow(ndoth, 64.0);
    float visibility = shadowMap();
    value *= visibility;

    outColor = vec4(value, value, value, 1.0);
}
