#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "LHVulkan.h"

#define WIDTH 512
#define HEIGHT 512

static const char* vertShaderText =
"#version 400\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
"layout (std140, binding = 0) uniform bufferVals {\n"
"    mat4 mvp;\n"
"} myBufferVals;\n"
"layout (location = 0) in vec4 pos;\n"
"layout (location = 1) in vec4 inColor;\n"
"layout (location = 0) out vec4 outColor;\n"
"out gl_PerVertex { \n"
"    vec4 gl_Position;\n"
"};\n"
"void main() {\n"
"   outColor = inColor;\n"
"   gl_Position = myBufferVals.mvp * pos;\n"
"}\n";

static const char* fragShaderText =
"#version 400\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
"layout (location = 0) in vec4 color;\n"
"layout (location = 0) out vec4 outColor;\n"
"void main() {\n"
"   outColor = color;\n"
"}\n";

int main() {
	VkResult res;
	const bool depthPresent = true;


	struct AppState {
		glm::mat4 Projection;
		glm::mat4 View;
		glm::mat4 Model;
		glm::mat4 Clip;
		glm::mat4 MVP;
	} state;


	float fov = glm::radians(45.0f);
	if (WIDTH > HEIGHT) {
		fov *= static_cast<float>(HEIGHT) / static_cast<float>(WIDTH);
	}
	state.Projection = glm::perspective(fov,
		static_cast<float>(WIDTH) /
		static_cast<float>(HEIGHT), 0.1f, 100.0f);
	state.View = glm::lookAt(
		glm::vec3(-5, 3, -10),  // Camera is at (-5,3,-10), in World Space
		glm::vec3(0, 0, 0),  // and looks at the origin
		glm::vec3(0, -1, 0)   // Head is up (set to 0,-1,0 to look upside-down)
	);
	state.Model = glm::mat4(1.0f);
	// Vulkan clip space has inverted Y and half Z.
	state.Clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	state.MVP = state.Clip * state.Projection * state.View * state.Model;


	res = createInstance();
	res = createDeviceInfo();
	createWindowContext(512, 512);
	res = createSwapChainExtention(); //Needs a bit more to add
	res = createDevice();

	VkCommandBuffer cmdbuffer;
	res = createCommandPool();
	res = createCommandBuffer(cmdbuffer);
	res = execute_begin_command_buffer(cmdbuffer);
	createDeviceQueue();
	createSwapChain(17);
	createDepthBuffer(cmdbuffer);;
	Uniform_Data uData;
	setUniformValue<glm::mat4>(state.MVP, uData, 0, VK_SHADER_STAGE_VERTEX_BIT);
	createDescripterLayout();
	createRenderPass(depthPresent);
	init_shaders(vertShaderText, fragShaderText);

}