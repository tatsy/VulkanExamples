#version 450
#extension GL_ARB_separate_shader_objects : enable

precision highp float;

layout(location = 0) in vec3 f_posCameraSpace;
layout(location = 1) in vec3 f_normCameraSpace;
layout(location = 2) in vec3 f_lightPosCameraSpace;
layout(location = 3) in vec2 f_uv;
layout(location = 4) in vec4 f_posScreenLightSpace;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D u_imageTex;
layout(binding = 2) uniform sampler2D u_depthTex;

const int nPCFSamples = 32;
vec3 samples[] = vec3[64](
    vec3(-0.015809, -0.008987, 0.175437),
    vec3(0.664079, -0.286524, 0.151204),
    vec3(-0.221546, 0.289903, 0.863210),
    vec3(-0.018174, -0.024018, 0.668253),
    vec3(0.331269, 0.132583, 0.277102),
    vec3(0.663623, 0.075051, 0.106002),
    vec3(0.084538, 0.007773, 0.954540),
    vec3(-0.685963, -0.571579, 0.221340),
    vec3(-0.093575, -0.418713, 0.086912),
    vec3(0.373525, -0.070778, 0.411393),
    vec3(-0.146223, -0.194811, 0.128132),
    vec3(-0.282008, -0.481534, 0.598684),
    vec3(0.137529, -0.021423, 0.697192),
    vec3(-0.184849, -0.230509, 0.207357),
    vec3(0.057931, -0.261809, 0.514569),
    vec3(-0.626876, -0.142766, 0.015477),
    vec3(0.035749, -0.134544, 0.636266),
    vec3(0.140209, 0.705407, 0.542895),
    vec3(0.451050, 0.065316, 0.397382),
    vec3(-0.075430, -0.589476, 0.501260),
    vec3(-0.014807, -0.791854, 0.124690),
    vec3(0.066264, -0.024522, 0.987511),
    vec3(-0.553396, -0.513869, 0.344234),
    vec3(-0.031567, -0.014055, 0.061356),
    vec3(0.002009, 0.598487, 0.729929),
    vec3(-0.093261, -0.053454, 0.559946),
    vec3(0.000588, 0.011218, 0.983976),
    vec3(0.627568, 0.397864, 0.154045),
    vec3(0.009502, -0.000035, 0.066689),
    vec3(-0.273961, -0.341573, 0.043889),
    vec3(-0.090554, -0.178244, 0.063929),
    vec3(-0.017620, -0.016803, 0.095556),
    vec3(0.370435, 0.560746, 0.284348),
    vec3(0.004712, -0.408754, 0.687112),
    vec3(0.049379, -0.380161, 0.789551),
    vec3(0.955654, 0.048848, 0.234824),
    vec3(0.610503, -0.064041, 0.260610),
    vec3(0.030160, 0.040214, 0.861151),
    vec3(0.062506, 0.462544, 0.024781),
    vec3(0.198067, -0.470264, 0.781367),
    vec3(0.121008, -0.155667, 0.664276),
    vec3(0.383199, -0.171766, 0.437127),
    vec3(-0.020014, -0.006322, 0.662601),
    vec3(-0.385539, -0.004759, 0.081687),
    vec3(-0.123043, -0.719357, 0.540056),
    vec3(0.017499, -0.552628, 0.589243),
    vec3(-0.177693, 0.040355, 0.910881),
    vec3(-0.084348, 0.098606, 0.397857),
    vec3(-0.153964, 0.077838, 0.313922),
    vec3(-0.568381, 0.442173, 0.532646),
    vec3(0.346715, -0.232899, 0.662006),
    vec3(-0.004044, 0.040949, 0.864202),
    vec3(0.446547, 0.580602, 0.220711),
    vec3(0.010531, -0.238324, 0.696604),
    vec3(0.067855, -0.058695, 0.639223),
    vec3(-0.053428, 0.042408, 0.293857),
    vec3(0.008052, -0.068077, 0.524395),
    vec3(0.313861, -0.117145, 0.102384),
    vec3(0.507505, 0.191501, 0.827713),
    vec3(-0.072827, -0.421070, 0.412630),
    vec3(-0.332951, -0.125606, 0.680263),
    vec3(-0.929545, -0.003797, 0.156700),
    vec3(-0.029845, -0.305370, 0.804991),
    vec3(-0.003478, 0.008937, 0.766257)
);

void main() {
    vec3 V = normalize(-f_posCameraSpace);
    vec3 N = normalize(f_normCameraSpace);
    vec3 L = normalize(f_lightPosCameraSpace - f_posCameraSpace);
    vec3 H = normalize(V + L);
    float NdotL = max(0.0, dot(N, L));
    float NdotH = max(0.0, dot(N, H));

    float zValue = f_posScreenLightSpace.z / f_posScreenLightSpace.w;
    vec2 uv = f_posScreenLightSpace.xy / f_posScreenLightSpace.w * 0.5 + 0.5;

    float visibility = 0.0;
    float bias = 0.005 * tan(acos(NdotL));
    bias = clamp(bias, 0.0, 1.0e-5);

    for (int i = 0; i < nPCFSamples; i++) {
        vec2 jitter = samples[i].xy * 0.002;
        vec2 moment = texture(u_depthTex, uv + jitter).xy;
        if (zValue <= moment.x + bias) {
            visibility += 1.0;
        } else {
            float variance = moment.y - moment.x * moment.x;
            float gap = abs(zValue - moment.x);
            visibility += clamp(1.0 - 0.5 * variance / (variance + gap * gap), 0.0, 1.0);
        }
    }
    visibility /= float(nPCFSamples);

    vec3 rhoDiff = vec3(0.75164, 0.60648, 0.22648);
    vec3 rhoSpec = vec3(0.628281, 0.555802, 0.366065);
    vec3 rhoAmbi = vec3(0.24725, 0.1995, 0.0745);
    if (length(f_uv) != 0.0) {
        rhoDiff = texture(u_imageTex, f_uv).rgb;
        rhoSpec = vec3(0.0);
        rhoAmbi = vec3(0.0);
    }

    vec3 diffuse = rhoDiff * NdotL;
    vec3 specular = rhoSpec * pow(NdotH, 64.0);
    vec3 ambient = rhoAmbi;
    vec3 rgb = visibility * (diffuse + specular + ambient);
    out_color = vec4(rgb, 1.0);
}
  