#version 450

layout (location = 0) in vec3 outNormal;
layout (location = 0) out vec4 outFragColor;

void main() {
    vec3 N = normalize(outNormal);
    vec3 L = normalize(vec3(1.0,-0.5,0.0));
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * L;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * L;
    float specularStrength = 0.5;
    vec3 results = (ambient + diffuse) * L;

    outFragColor = vec4(results, 1.0);
}