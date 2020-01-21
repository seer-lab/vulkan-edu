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

VkResult mesh_init() {
	int i;
	int k;
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	float vertices[8][4] = {
		{ -1.0, -1.0, -1.0, 1.0 }, //0
		{ -1.0, -1.0, 1.0, 1.0 }, //1
		{ -1.0, 1.0, -1.0, 1.0 }, //2
		{ -1.0, 1.0, 1.0, 1.0 }, //3
		{ 1.0, -1.0, -1.0, 1.0 }, //4
		{ 1.0, -1.0, 1.0, 1.0 }, //5
		{ 1.0, 1.0, -1.0, 1.0 }, //6
		{ 1.0, 1.0, 1.0, 1.0 } //7
	};

	float normals[8][3] = {
		{ -1.0, -1.0, -1.0 }, //0
		{ -1.0, -1.0, 1.0 }, //1
		{ -1.0, 1.0, -1.0 }, //2
		{ -1.0, 1.0, 1.0 }, //3
		{ 1.0, -1.0, -1.0 }, //4
		{ 1.0, -1.0, 1.0 }, //5
		{ 1.0, 1.0, -1.0 }, //6
		{ 1.0, 1.0, 1.0 } //7
	};

	uint16_t indices[36] = { 3, 1, 0, 0, 2, 3,
		0, 4, 5, 5, 1, 0,
		2, 6, 7, 7, 3, 2,
		0, 4, 6, 0, 2, 6,
		1, 5, 7, 7, 3, 1,
		7, 5, 4, 4, 6, 7 };

	vBuffer = new float[7 * 8];
	k = 0;
	for (i = 0; i < 8; i++) {
		vBuffer[k++] = vertices[i][0];
		vBuffer[k++] = vertices[i][1];
		vBuffer[k++] = vertices[i][2];
		vBuffer[k++] = vertices[i][3];
		vBuffer[k++] = normals[i][0];
		vBuffer[k++] = normals[i][1];
		vBuffer[k++] = normals[i][2];
	}

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	buf_info.size = sizeof(indices);
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	res = vkCreateBuffer(device, &buf_info, NULL, &iBuffer);
	assert(res == VK_SUCCESS);
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, iBuffer, &mem_reqs);
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;
	alloc_info.allocationSize = mem_reqs.size;
	pass = memory_type_from_properties(memory_properties, mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex);
	assert(pass && "No mappable coherent memory");
	VkDeviceMemory mem;
	res = vkAllocateMemory(device, &alloc_info, NULL, &mem);
	assert(res == VK_SUCCESS);
	uint8_t* pData;
	res = vkMapMemory(device, mem, 0, mem_reqs.size, 0, (void**)
		&pData);
	assert(res == VK_SUCCESS);
	memcpy(pData, indices, sizeof(indices));
	vkUnmapMemory(device, mem);
	res = vkBindBufferMemory(device, iBuffer, mem, 0);
	assert(res == VK_SUCCESS);
	return res;
}

void init_vertex_buffer(const void* vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buf_info.size = dataSize;
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	res = vkCreateBuffer(device, &buf_info, NULL, &vertex_buffer.buf);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, vertex_buffer.buf, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.memoryTypeIndex = 0;

	alloc_info.allocationSize = mem_reqs.size;
	pass = memory_type_from_properties(memory_properties, mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&alloc_info.memoryTypeIndex);
	assert(pass && "No mappable, coherent memory");

	res = vkAllocateMemory(device, &alloc_info, NULL, &(vertex_buffer.mem));
	assert(res == VK_SUCCESS);

	vertex_buffer.buffer_info.range = mem_reqs.size;
	vertex_buffer.buffer_info.offset = 0;

	uint8_t* pData;
	res = vkMapMemory(device, vertex_buffer.mem, 0, mem_reqs.size, 0,
		(void**)&pData);
	assert(res == VK_SUCCESS);

	memcpy(pData, vertexData, dataSize);

	vkUnmapMemory(device, vertex_buffer.mem);

	res = vkBindBufferMemory(device, vertex_buffer.buf, vertex_buffer.mem, 0);
	assert(res == VK_SUCCESS);

	vi_binding.binding = 0;
	vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vi_binding.stride = dataStride;

	vi_attribs[0].binding = 0;
	vi_attribs[0].location = 0;
	vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vi_attribs[0].offset = 0;
	vi_attribs[1].binding = 0;
	vi_attribs[1].location = 1;
	vi_attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vi_attribs[1].offset = 16;

}

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
	init_framebuffers(depthPresent);
	mesh_init();
	init_vertex_buffer(vBuffer, 7 * 8 * sizeof(float), 7 * sizeof(float), false);
	createDescriptorPool(false);
	createDescriptorSet(false);
	init_pipeline_cache();
	init_pipeline(depthPresent, true);
	renderObjects(cmdbuffer);
}