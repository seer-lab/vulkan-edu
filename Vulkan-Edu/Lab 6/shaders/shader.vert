#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTex;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outTex;

layout (binding = 0) uniform UBO {
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;


out gl_PerVertex {
    vec4 gl_Position;   
};

void main() {
	outNormal = inNormal;
	outTex = inTex;
	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos.xyz, 1.0);
}