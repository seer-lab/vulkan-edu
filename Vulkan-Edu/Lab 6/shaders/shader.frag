#version 450
layout (set = 0, binding = 2) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 outNormal;
layout (location = 1) in vec2 outTex;

layout (location = 0) out vec4 outFragColor;

layout (binding = 1) uniform UBOFS{
vec3 lightPos;
float ambientStrenght;
float specularStrenght;
}uboFS;

void main() {
    vec3 N = normalize(outNormal);
    vec3 L = normalize(uboFS.lightPos);
    vec3 ambient = uboFS.ambientStrenght * L;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * L;
    vec3 results = (ambient + diffuse) * L;

    outFragColor = texture(samplerColorMap, outTex) * vec4(1.0,1.0,1.0, 1.0);
}