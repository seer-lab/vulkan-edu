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
" mat4 mvp;\n"
"} myBufferVals;\n"
"layout (location = 0) in vec4 pos;\n"
"layout (location = 1) in vec3 inNormal;\n"
"layout (location = 0) out vec3 outNormal;\n"
"out gl_PerVertex { \n"
" vec4 gl_Position;\n"
"};\n"
"void main() {\n"
" outNormal = inNormal;\n"
" gl_Position = myBufferVals.mvp * pos;\n"
"}\n";

static const char* fragShaderText =
"#version 400\n"
"#extension GL_ARB_separate_shader_objects : enable\n"
"#extension GL_ARB_shading_language_420pack : enable\n"
"layout (location = 0) in vec3 Normal;\n"
"layout (location = 0) out vec4 outColor;\n"
"void main() {\n"
"vec3 N = normalize(Normal);\n"
"vec3 L = normalize(vec3(1.0,-0.5,0.0));\n"
"float ambientStrength = 0.1;\n"
"vec3 ambient = ambientStrength * L;\n"
"float diff = max(dot(N, L), 0.0);\n"
"vec3 diffuse = diff * L;\n"
"float specularStrength = 0.5;\n"
"vec3 results = (ambient + diffuse) * L;\n"
" outColor = vec4(results, 1.0) * 7.0f;\n"
"}\n";


void mesh_init() {
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

	state.vBuffer = new float[7 * 8];
	k = 0;
	for (i = 0; i < 8; i++) {
		state.vBuffer[k++] = vertices[i][0];
		state.vBuffer[k++] = vertices[i][1];
		state.vBuffer[k++] = vertices[i][2];
		state.vBuffer[k++] = vertices[i][3];
		state.vBuffer[k++] = normals[i][0];
		state.vBuffer[k++] = normals[i][1];
		state.vBuffer[k++] = normals[i][2];
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
	res = vkCreateBuffer(device, &buf_info, NULL, &state.iBuffer);
	assert(res == VK_SUCCESS);
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, state.iBuffer, &mem_reqs);
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
	res = vkBindBufferMemory(device, state.iBuffer, mem, 0);
	assert(res == VK_SUCCESS);

}

void init_pipeline( struct AppState& state, VkBool32 include_depth,
	VkBool32 include_vi = true);

void init_uniform_buffer( struct AppState& state) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	float fov = glm::radians(45.0f);
	if (width > height) {
		fov *= static_cast<float>(height) / static_cast<float>(width);
	}
	state.Projection = glm::perspective(fov,
		static_cast<float>(width) /
		static_cast<float>(height), 0.1f, 100.0f);
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

	/* VULKAN_KEY_START */
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(state.MVP);
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	res = vkCreateBuffer(device, &buf_info, NULL, &state.uniform_data.buf);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, state.uniform_data.buf,
		&mem_reqs);

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

	res = vkAllocateMemory(device, &alloc_info, NULL,
		&(state.uniform_data.mem));
	assert(res == VK_SUCCESS);

	uint8_t* pData;
	res = vkMapMemory(device, state.uniform_data.mem, 0, mem_reqs.size, 0,
		(void**)&pData);
	assert(res == VK_SUCCESS);

	memcpy(pData, &state.MVP, sizeof(state.MVP));

	vkUnmapMemory(device, state.uniform_data.mem);

	res = vkBindBufferMemory(device, state.uniform_data.buf,
		state.uniform_data.mem, 0);
	assert(res == VK_SUCCESS);

	state.uniform_data.buffer_info.buffer = state.uniform_data.buf;
	state.uniform_data.buffer_info.offset = 0;
	state.uniform_data.buffer_info.range = sizeof(state.MVP);
}

void init_descriptor_and_pipeline_layouts(
	struct AppState& state) {
	VkDescriptorSetLayoutBinding layout_bindings[2];
	layout_bindings[0].binding = 0;
	layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_bindings[0].descriptorCount = 1;
	layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_bindings[0].pImmutableSamplers = NULL;

	/* Next take layout bindings and use them to create a descriptor set layout
	*/
	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
	descriptor_layout.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout.pNext = NULL;
	descriptor_layout.bindingCount = 1;
	descriptor_layout.pBindings = layout_bindings;

	VkResult U_ASSERT_ONLY res;

	state.num_descriptor_sets = 1;
	state.desc_layout.resize(state.num_descriptor_sets);
	res = vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL,
		state.desc_layout.data());
	assert(res == VK_SUCCESS);

	/* Now use the descriptor layout to create a pipeline layout */
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = state.num_descriptor_sets;
	pPipelineLayoutCreateInfo.pSetLayouts = state.desc_layout.data();

	res = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, NULL,
		&state.pipeline_layout);
	assert(res == VK_SUCCESS);
}

void init_vertex_buffer( struct AppState& state, const void* vertexData,
	uint32_t dataSize, uint32_t dataStride,
	bool use_texture) {
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
	res = vkCreateBuffer(device, &buf_info, NULL, &state.vertex_buffer.buf);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, state.vertex_buffer.buf,
		&mem_reqs);

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

	res = vkAllocateMemory(device, &alloc_info, NULL,
		&(state.vertex_buffer.mem));
	assert(res == VK_SUCCESS);
	state.vertex_buffer.buffer_info.range = mem_reqs.size;
	state.vertex_buffer.buffer_info.offset = 0;

	uint8_t* pData;
	res = vkMapMemory(device, state.vertex_buffer.mem, 0, mem_reqs.size, 0,
		(void**)&pData);
	assert(res == VK_SUCCESS);

	memcpy(pData, vertexData, dataSize);

	vkUnmapMemory(device, state.vertex_buffer.mem);

	res = vkBindBufferMemory(device, state.vertex_buffer.buf,
		state.vertex_buffer.mem, 0);
	assert(res == VK_SUCCESS);

	state.vi_binding.binding = 0;
	state.vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	state.vi_binding.stride = dataStride;

	state.vi_attribs[0].binding = 0;
	state.vi_attribs[0].location = 0;
	state.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	state.vi_attribs[0].offset = 0;
	state.vi_attribs[1].binding = 0;
	state.vi_attribs[1].location = 1;
	state.vi_attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	state.vi_attribs[1].offset = 16;

}

void init_descriptor_pool( struct AppState& state, bool use_texture) {
	/* DEPENDS on init_uniform_buffer() and
	* init_descriptor_and_pipeline_layouts() */

	VkResult U_ASSERT_ONLY res;
	VkDescriptorPoolSize type_count[2];
	type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[0].descriptorCount = 1;
	if (use_texture) {
		type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		type_count[1].descriptorCount = 1;
	}

	VkDescriptorPoolCreateInfo descriptor_pool = {};
	descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool.pNext = NULL;
	descriptor_pool.maxSets = 1;
	descriptor_pool.poolSizeCount = 1;
	descriptor_pool.pPoolSizes = type_count;

	res = vkCreateDescriptorPool(device, &descriptor_pool, NULL,
		&state.desc_pool);
	assert(res == VK_SUCCESS);
}

void init_descriptor_set( struct AppState& state, bool use_texture) {
	/* DEPENDS on init_descriptor_pool() */

	VkResult U_ASSERT_ONLY res;

	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = NULL;
	alloc_info[0].descriptorPool = state.desc_pool;
	alloc_info[0].descriptorSetCount = state.num_descriptor_sets;
	alloc_info[0].pSetLayouts = state.desc_layout.data();

	state.desc_set.resize(state.num_descriptor_sets);
	res =
		vkAllocateDescriptorSets(device, alloc_info, state.desc_set.data());
	assert(res == VK_SUCCESS);

	VkWriteDescriptorSet writes[2];

	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = state.desc_set[0];
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &state.uniform_data.buffer_info;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;

	vkUpdateDescriptorSets(device, use_texture ? 2 : 1, writes, 0, NULL);
}

void init_pipeline( struct AppState& state, VkBool32 include_depth,
	VkBool32 include_vi) {
	VkResult U_ASSERT_ONLY res;

	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = NULL;
	dynamicState.pDynamicStates = dynamicStateEnables;
	dynamicState.dynamicStateCount = 0;

	VkPipelineVertexInputStateCreateInfo vi;
	memset(&vi, 0, sizeof(vi));
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	if (include_vi) {
		vi.pNext = NULL;
		vi.flags = 0;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &state.vi_binding;
		vi.vertexAttributeDescriptionCount = 2;
		vi.pVertexAttributeDescriptions = state.vi_attribs;
	}
	VkPipelineInputAssemblyStateCreateInfo ia;
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.pNext = NULL;
	ia.flags = 0;
	ia.primitiveRestartEnable = VK_FALSE;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rs;
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.pNext = NULL;
	rs.flags = 0;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rs.depthClampEnable = include_depth;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.depthBiasConstantFactor = 0;
	rs.depthBiasClamp = 0;
	rs.depthBiasSlopeFactor = 0;
	rs.lineWidth = 1.0f;

	VkPipelineColorBlendStateCreateInfo cb;
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.flags = 0;
	cb.pNext = NULL;
	VkPipelineColorBlendAttachmentState att_state[1];
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;
	cb.logicOpEnable = VK_FALSE;
	cb.logicOp = VK_LOGIC_OP_NO_OP;
	cb.blendConstants[0] = 1.0f;
	cb.blendConstants[1] = 1.0f;
	cb.blendConstants[2] = 1.0f;
	cb.blendConstants[3] = 1.0f;

	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.pNext = NULL;
	vp.flags = 0;
#ifndef __ANDROID__
	vp.viewportCount = NUM_VIEWPORTS;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = NUM_SCISSORS;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
		VK_DYNAMIC_STATE_SCISSOR;
	vp.pScissors = NULL;
	vp.pViewports = NULL;
#else
	// Temporary disabling dynamic viewport on Android because some of drivers doesn't
	// support the feature.
	VkViewport viewports;
	viewports.minDepth = 0.0f;
	viewports.maxDepth = 1.0f;
	viewports.x = 0;
	viewports.y = 0;
	viewports.width = width;
	viewports.height = height;
	VkRect2D scissor;
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vp.viewportCount = NUM_VIEWPORTS;
	vp.scissorCount = NUM_SCISSORS;
	vp.pScissors = &scissor;
	vp.pViewports = &viewports;
#endif
	VkPipelineDepthStencilStateCreateInfo ds;
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.pNext = NULL;
	ds.flags = 0;
	ds.depthTestEnable = include_depth;
	ds.depthWriteEnable = include_depth;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.stencilTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.back.compareMask = 0;
	ds.back.reference = 0;
	ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
	ds.back.writeMask = 0;
	ds.minDepthBounds = 0;
	ds.maxDepthBounds = 0;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	VkPipelineMultisampleStateCreateInfo ms;
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pNext = NULL;
	ms.flags = 0;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = NUM_SAMPLES;
	ms.sampleShadingEnable = VK_FALSE;
	ms.alphaToCoverageEnable = VK_FALSE;
	ms.alphaToOneEnable = VK_FALSE;
	ms.minSampleShading = 0.0;

	VkGraphicsPipelineCreateInfo pipeline;
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.pNext = NULL;
	pipeline.layout = state.pipeline_layout;
	pipeline.basePipelineHandle = VK_NULL_HANDLE;
	pipeline.basePipelineIndex = 0;
	pipeline.flags = 0;
	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pTessellationState = NULL;
	pipeline.pMultisampleState = &ms;
	pipeline.pDynamicState = &dynamicState;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.pStages = state.shaderStages;
	pipeline.stageCount = 2;
	pipeline.renderPass = render_pass;
	pipeline.subpass = 0;

	res = vkCreateGraphicsPipelines(device, pipelineCaches, 1,
		&pipeline, NULL, &state.pipeline);
	assert(res == VK_SUCCESS);
}

int main() {
	VkResult U_ASSERT_ONLY res;
	const bool depthPresent = true;

	createInstance("Sample Program");
	createDeviceInfo();
	init_connection();
	createWindowContext(512, 512);
	createSwapChainExtention();
	createDevice();

	createCommandPool();
	createCommandBuffer(state.cmd);
	execute_begin_command_buffer(state.cmd);
	createDeviceQueue();
	createSwapChain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	createDepthBuffer( state.cmd);
	init_uniform_buffer( state);
	init_descriptor_and_pipeline_layouts( state);
	createRenderPass( depthPresent, true, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	init_shaders(state, vertShaderText, fragShaderText);
	createFrameBuffer( depthPresent);

	mesh_init();
	init_vertex_buffer( state, state.vBuffer, 7 * 8 * sizeof(float), 7 * sizeof(float), false);

	init_descriptor_pool( state, false);
	init_descriptor_set( state, false);
	createPipeLineCache();
	init_pipeline( state, depthPresent);
	renderObject(state);

	//	destroy_instance();
	return 0;
	
}