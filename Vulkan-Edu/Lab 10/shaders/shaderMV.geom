#version 450

#extension GL_ARB_viewport_array : enable

layout (triangles, invocations = 2) in;
layout (triangle_strip, max_vertices = 3) out;

layout (binding = 0) uniform UBO {
	mat4 projectionMatrix[2];
	mat4 modelMatrix[2];
	mat4 viewMatrix;
} ubo;

layout (binding = 1) uniform UBOFS{
	vec4 lightPos;
	float ambientStrenght;
	float specularStrenght;
}uboFS;

layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outNormal;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

void main(void){	
	for(int i = 0; i < gl_in.length(); i++){
		outNormal = mat3(ubo.modelMatrix[gl_InvocationID]) * inNormal[i];

		vec4 pos = gl_in[i].gl_Position;
		vec4 worldPos = (ubo.modelMatrix[gl_InvocationID] * pos);
		
		vec3 lPos = vec3(ubo.modelMatrix[gl_InvocationID]  * uboFS.lightPos);
		outLightVec = lPos - worldPos.xyz;
		outViewVec = -worldPos.xyz;	
	
		gl_Position = ubo.projectionMatrix[gl_InvocationID] * worldPos;

		// Set the viewport index that the vertex will be emitted to
		gl_ViewportIndex = gl_InvocationID;
		gl_PrimitiveID = gl_PrimitiveIDIn;
		EmitVertex();
	}
	EndPrimitive();
}