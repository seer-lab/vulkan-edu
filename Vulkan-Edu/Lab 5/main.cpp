#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

#include "LHVulkan.h"
#include "cube_data.h"
#include "tiny_obj_loader.h"

#define OBJ_MESH
#define WIDTH 512
#define HEIGHT 512


//Custom States depending on what is needed
struct appState {
	struct Model {
		struct Matricies {
			glm::mat4 projectionMatrix;
			glm::mat4 viewMatrix;
			glm::mat4 modelMatrix;
		}uboVS;

		struct vertices v;
		struct indices i;
		float* vBuffer;

		VkDescriptorSet descriptorSet;

		// Uniform buffer block object
		struct UniformBuffer {
			VkDeviceMemory memory;
			VkBuffer buffer;
			VkDescriptorBufferInfo descriptor;
		}  uniformBuffer[2];

		struct Lighting {
			glm::vec3 lightPos;
			float ambientStrenght;
			float specularStrenght;
		}uboFS;
	};
	std::array<Model, 2> cubes;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorSetLayout descriptorSetLayout;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	VkVertexInputBindingDescription vertexInputBinding{};
	std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
};

glm::vec3 rotation = glm::vec3();
glm::vec3 cameraPos = glm::vec3();
glm::vec2 mousePos;

bool update = false;
float eyex, eyey, eyez;	// current user position

double theta, phi;		// user's position  on a sphere centered on the object
double r;
int triangles;			// number of triangles

void buildCommandBuffers(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	// Set clear values for all framebuffer attachments with loadOp set to clear
	// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
	VkClearValue clearValues[2];
	createClearColor(context, clearValues);

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	createRenderPassCreateInfo(context, renderPassBeginInfo);
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < context.cmdBuffer.size(); ++i) {
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = context.frameBuffers[i];

		res = (vkBeginCommandBuffer(context.cmdBuffer[i], &cmdBufInfo));
		assert(res == VK_SUCCESS);

		vkCmdBeginRenderPass(context.cmdBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
		
		// Update dynamic viewport state
		VkViewport viewport = {};
		createViewports(context, context.cmdBuffer[i], viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		createScisscor(context, context.cmdBuffer[i], scissor);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(context.cmdBuffer[i], 0, 1, &state.cubes[0].v.buffer, offsets);
		vkCmdBindIndexBuffer(context.cmdBuffer[i], state.cubes[0].i.buffer, 0, VK_INDEX_TYPE_UINT32);

		for (auto& cube : state.cubes) {
			// Bind the cube's descriptor set. This tells the command buffer to use the uniform buffer and image set for this cube
			vkCmdBindDescriptorSets(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelineLayout, 0, 1, &cube.descriptorSet, 0, nullptr);
			vkCmdDrawIndexed(context.cmdBuffer[i], state.cubes[0].i.count, 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(context.cmdBuffer[i]);

		res = (vkEndCommandBuffer(context.cmdBuffer[i]));
		assert(res == VK_SUCCESS);
	}
}

void setupDescriptorPool(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	/*

	Descriptor pool

	Actual descriptors are allocated from a descriptor pool telling the driver what types and how many
	descriptors this application will use

	An application can have multiple pools (e.g. for multiple threads) with any number of descriptor types
	as long as device limits are not surpassed

	It's good practice to allocate pools with actually required descriptor types and counts

	*/


	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[2];
	// This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1 + static_cast<uint32_t>(state.cubes.size());

	typeCounts[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[1].descriptorCount = static_cast<uint32_t>(state.cubes.size());

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
	descriptorPoolInfo.maxSets = static_cast<uint32_t>(state.cubes.size());

	res = (vkCreateDescriptorPool(context.device, &descriptorPoolInfo, nullptr, &context.descriptorPool));
	assert(res == VK_SUCCESS);
}

void setupDescriptorSetLayout(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	// Setup layout of descriptors used in this example
	// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout binding

	/*

	Descriptor set layout

	The layout describes the shader bindings and types used for a certain descriptor layout and as such must match the shader bindings

	Shader bindings used in this example:

	VS:
		layout (set = 0, binding = 0) uniform uboVS ...

	FS :
		layout (set = 0, binding = 1) uniform uboFS ...;

	*/


	// Binding 0: Uniform buffer (Vertex shader)
	std::array<VkDescriptorSetLayoutBinding, 2>layoutBinding = {};
	layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding[0].descriptorCount = 1;
	layoutBinding[0].binding = 0;
	layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding[0].pImmutableSamplers = nullptr;

	// Binding 1: Uniform buffer (Fragment shader)
	layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding[1].descriptorCount = 1;
	layoutBinding[1].binding = 1;
	layoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layoutBinding[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = nullptr;
	descriptorLayout.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	descriptorLayout.pBindings = layoutBinding.data();

	res = (vkCreateDescriptorSetLayout(context.device, &descriptorLayout, nullptr, &state.descriptorSetLayout));
	assert(res == VK_SUCCESS);
}

void setupDescriptorSet(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	/*

	Descriptor sets

	Using the shared descriptor set layout and the descriptor pool we will now allocate the descriptor sets.

	Descriptor sets contain the actual descriptor fo the objects (buffers, images) used at render time.

	*/

	for (auto& cube : state.cubes) {
		// Allocate a new descriptor set from the global descriptor pool
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = context.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &state.descriptorSetLayout;

		res = (vkAllocateDescriptorSets(context.device, &allocInfo, &cube.descriptorSet));
		assert(res == VK_SUCCESS);

		std::array<VkWriteDescriptorSet, 2> writeDescriptorSet = {};

		// Binding 0 : Uniform buffer VS
		writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet[0].dstSet = cube.descriptorSet;
		writeDescriptorSet[0].descriptorCount = 1;
		writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet[0].pBufferInfo = &cube.uniformBuffer[0].descriptor;
		// Binds this uniform buffer to binding point 0
		writeDescriptorSet[0].dstBinding = 0;

		// Binding 0 : Uniform buffer FS
		writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet[1].dstSet = cube.descriptorSet;
		writeDescriptorSet[1].descriptorCount = 1;
		writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet[1].pBufferInfo = &cube.uniformBuffer[1].descriptor;
		// Binds this uniform buffer to binding point 0
		writeDescriptorSet[1].dstBinding = 1;

		// Execute the writes to update descriptors for this set
		// Note that it's also possible to gather all writes and only run updates once, even for multiple sets
		// This is possible because each VkWriteDescriptorSet also contains the destination set to be updated
		// For simplicity we will update once per set instead

		vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, nullptr);
	}
}

void preparePipelines(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	VkPipelineLayoutCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// The pipeline layout is based on the descriptor set layout we created above
	pipelineCreateInfo.setLayoutCount = 1;
	pipelineCreateInfo.pSetLayouts = &state.descriptorSetLayout;
	res = (vkCreatePipelineLayout(context.device, &pipelineCreateInfo, nullptr, &state.pipelineLayout));
	assert(res == VK_SUCCESS);

	// Construct the differnent states making up the pipeline

	// Input assembly state describes how primitives are assembled
	// This pipeline will assemble vertex data as a triangle lists
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	// Color blend state describes how blend factors are calculated (if used)
	// We need one blend attachment state per color attachment (even if blending is not used
	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
	blendAttachmentState[0].colorWriteMask = 0xf;
	blendAttachmentState[0].blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

	// Viewport state sets the number of viewports and scissor used in this pipeline
	// Note: This is actually overriden by the dynamic states (see below)
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Enable dynamic states
	// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
	// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
	// For this example we will set the viewport and scissor using dynamic states
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

	// Depth and stencil state containing depth and stencil compare and test operations
	// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = depthStencilState.back;

	// Multi sampling state
	// This example does not make use fo multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.pSampleMask = nullptr;


	// Vertex input state used for pipeline creation
	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &state.vertexInputBinding;
	vertexInputState.vertexAttributeDescriptionCount = 2;
	vertexInputState.pVertexAttributeDescriptions = state.vertexInputAttributs.data();

	VkGraphicsPipelineCreateInfo pipelineGraphicCreateInfo = {};
	pipelineGraphicCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
	pipelineGraphicCreateInfo.layout = state.pipelineLayout;
	// Renderpass this pipeline is attached to
	pipelineGraphicCreateInfo.renderPass = context.render_pass;

	// Assign the pipeline states to the pipeline creation info structure
	pipelineGraphicCreateInfo.pVertexInputState = &vertexInputState;
	pipelineGraphicCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineGraphicCreateInfo.pRasterizationState = &rasterizationState;
	pipelineGraphicCreateInfo.pColorBlendState = &colorBlendState;
	pipelineGraphicCreateInfo.pMultisampleState = &multisampleState;
	pipelineGraphicCreateInfo.pViewportState = &viewportState;
	pipelineGraphicCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineGraphicCreateInfo.renderPass = context.render_pass;
	pipelineGraphicCreateInfo.pDynamicState = &dynamicState;

	// Vertex shader
	createShaderStage(context, "./shaders/shader.vert", VK_SHADER_STAGE_VERTEX_BIT, state.shaderStages[0]);
	assert(state.shaderStages[0].module != VK_NULL_HANDLE);

	// Fragment shader
	createShaderStage(context, "./shaders/shader.frag", VK_SHADER_STAGE_FRAGMENT_BIT, state.shaderStages[1]);
	assert(state.shaderStages[1].module != VK_NULL_HANDLE);

	// Set pipeline shader stage info
	pipelineGraphicCreateInfo.stageCount = static_cast<uint32_t>(state.shaderStages.size());
	pipelineGraphicCreateInfo.pStages = state.shaderStages.data();

	// Create rendering pipeline using the specified states
	res = (vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineGraphicCreateInfo, nullptr, &state.pipeline));
	assert(res == VK_SUCCESS);
}

void updateUniformBuffers(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;
	uint8_t* pData;

	state.cubes[0].uboVS.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
	state.cubes[1].uboVS.modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));

	state.cubes[0].uboFS.lightPos = glm::vec3(1.0, -0.5, 0.0);
	state.cubes[0].uboFS.ambientStrenght = 0.1;
	state.cubes[0].uboFS.specularStrenght = 0.5;

	state.cubes[1].uboFS.lightPos = glm::vec3(0.0, -0.5, 1.0);
	state.cubes[1].uboFS.ambientStrenght = 0.3;
	state.cubes[1].uboFS.specularStrenght = 0.5;
	
	for (auto& cube : state.cubes) {
		cube.uboVS.projectionMatrix = glm::perspective(glm::radians(60.0f), (float)context.width / (float)context.height, 0.1f, 256.0f);
		cube.uboVS.viewMatrix = glm::lookAt(glm::vec3(eyex, eyey, eyez),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));

		cube.uboVS.modelMatrix = glm::translate(cube.uboVS.modelMatrix, glm::vec3(1.0, 0.0, 0.0));
		cube.uboVS.modelMatrix = glm::rotate(cube.uboVS.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		cube.uboVS.modelMatrix = glm::rotate(cube.uboVS.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		cube.uboVS.modelMatrix = glm::rotate(cube.uboVS.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	
		// Map uniform buffer and update it
		res = (vkMapMemory(context.device, cube.uniformBuffer[0].memory, 0, sizeof(cube.uboVS), 0, (void**)&pData));
		memcpy(pData, &cube.uboVS, sizeof(cube.uboVS));
		// Unmap after data has been copied
		// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
		vkUnmapMemory(context.device, cube.uniformBuffer[0].memory);

		res = (vkMapMemory(context.device, cube.uniformBuffer[1].memory, 0, sizeof(cube.uboFS), 0, (void**)&pData));
		memcpy(pData, &cube.uboFS, sizeof(cube.uboFS));
		// Unmap after data has been copied
		// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
		vkUnmapMemory(context.device, cube.uniformBuffer[1].memory);
	}
}

void prepareUniformBuffers(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	// Prepare and initialize a uniform buffer block containing shader uniforms
	// Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks
	VkMemoryRequirements memReqs;
	// Vertex shader uniform buffer block
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	for (auto& cube : state.cubes) {
		bufferInfo.size = sizeof(cube.uboVS);
		bindBufferToMem(context, bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			cube.uniformBuffer[0].buffer, cube.uniformBuffer[0].memory);
	
		// Store information in the uniform's descriptor that is used by the descriptor set
		cube.uniformBuffer[0].descriptor.buffer = cube.uniformBuffer[0].buffer;
		cube.uniformBuffer[0].descriptor.offset = 0;
		cube.uniformBuffer[0].descriptor.range = sizeof(cube.uboVS);

		bufferInfo.size = sizeof(cube.uboFS);
		bindBufferToMem(context, bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			cube.uniformBuffer[1].buffer, cube.uniformBuffer[1].memory);
	
		// Store information in the uniform's descriptor that is used by the descriptor set
		cube.uniformBuffer[1].descriptor.buffer = cube.uniformBuffer[1].buffer;
		cube.uniformBuffer[1].descriptor.offset = 0;
		cube.uniformBuffer[1].descriptor.range = sizeof(cube.uboFS);
	}

	updateUniformBuffers(context, state);
}

#ifdef OBJ_MESH
void prepareVertices(struct LHContext& context, struct appState& state, int index,bool useStagingBuffers) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	float* vertices;
	float* normals;
	uint32_t* indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	int nv;
	int nn;
	int ni;
	int i;

	VkBufferCreateInfo buf_info = {};
	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo alloc_info = {};
	uint8_t* pData;

	std::string err = tinyobj::LoadObj(shapes, materials, "cube.obj", 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	/*  Retrieve the vertex coordinate data */

	nv = (int)shapes[0].mesh.positions.size();
	vertices = new GLfloat[nv];
	for (i = 0; i < nv; i++) {
		vertices[i] = shapes[0].mesh.positions[i];
	}

	/*  Retrieve the vertex normals */

	nn = (int)shapes[0].mesh.normals.size();
	normals = new GLfloat[nn];
	for (i = 0; i < nn; i++) {
		normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = (int)shapes[0].mesh.indices.size();
	triangles = ni / 3;
	indices = new uint32_t[ni];
	for (i = 0; i < ni; i++) {
		indices[i] = shapes[0].mesh.indices[i];
	}


	state.cubes[index].vBuffer = new float[nv + nn];
	int k = 0;
	for (i = 0; i < nv / 3; i++) {
		state.cubes[index].vBuffer[k++] = vertices[3 * i];
		state.cubes[index].vBuffer[k++] = vertices[3 * i + 1];
		state.cubes[index].vBuffer[k++] = vertices[3 * i + 2];
		state.cubes[index].vBuffer[k++] = normals[3 * i];
		state.cubes[index].vBuffer[k++] = normals[3 * i + 1];
		state.cubes[index].vBuffer[k++] = normals[3 * i + 2];
	}

	uint32_t dataSize = (nv + nn) * sizeof(state.cubes[index].vBuffer[0]);
	uint32_t dataStride = 6 * (sizeof(float));

	state.cubes[index].i.count = ni;

	mapIndiciesToGPU(context, indices, sizeof(indices[0]) * ni, state.cubes[index].i.buffer, state.cubes[index].i.memory);
	mapVerticiesToGPU(context, state.cubes[index].vBuffer, dataSize, state.cubes[index].v.buffer, state.cubes[index].v.memory);

	//// Vertex input descriptions 
	//// Specifies the vertex input parameters for a pipeline

	//// Vertex input binding
	//// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)

	state.vertexInputBinding.binding = 0;
	state.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	state.vertexInputBinding.stride = dataStride;


	// Inpute attribute bindings describe shader attribute locations and memory layouts
	// These match the following shader layout
	//	layout (location = 0) in vec3 inPos;
	//	layout (location = 1) in vec3 inNormal;
	// Attribute location 0: Position
	//// Attribute location 1: Normal
	state.vertexInputAttributs[0].binding = 0;
	state.vertexInputAttributs[0].location = 0;
	state.vertexInputAttributs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	state.vertexInputAttributs[0].offset = 0;
	state.vertexInputAttributs[1].binding = 0;
	state.vertexInputAttributs[1].location = 1;
	state.vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	state.vertexInputAttributs[1].offset = sizeof(float);

}
#endif // OBJ_MESH

void renderLoop(struct LHContext& context, struct appState& state) {

	while (!glfwWindowShouldClose(context.window)) {
		glfwPollEvents();
		draw(context);
		if (update) {
			updateUniformBuffers(context, state);
			update = false;
		}
	}

	// Flush device to make sure all resources can be freed
	if (context.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(context.device);
	}
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		phi -= 0.1;
		update = true;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		phi += 0.1;
		update = true;
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		theta += 0.1;
		update = true;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		theta -= 0.1;
		update = true;
	}

	eyex = (float)(r * sin(theta) * cos(phi));
	eyey = (float)(r * sin(theta) * sin(phi));
	eyez = (float)(r * cos(theta));

}

int main() {
	theta = 1.5;
	phi = 1.5;
	r = 10.0;

	eyey = 2.0;
	eyez = 7.0;


	struct LHContext context = {};
	struct appState state = {};
	createInstance(context);
	createDeviceInfo(context);
	createWindowContext(context, 1280, 720);
	createSwapChainExtention(context);
	createDevice(context);
	createDeviceQueue(context);
	createSynchObject(context);
	createCommandPool(context);
	createSwapChain(context);
	createCommandBuffer(context);
	createSynchPrimitive(context);
	createDepthBuffers(context);
	createRenderPass(context);
	createPipeLineCache(context);
	createFrameBuffer(context);
	prepareSynchronizationPrimitives(context);

	//---> Implement our own functions
	for (int i = 0; i < state.cubes.size(); i++) {
		prepareVertices(context, state, i,false);
	}
	prepareUniformBuffers(context, state);
	setupDescriptorSetLayout(context, state);
	preparePipelines(context, state);
	setupDescriptorPool(context, state);
	setupDescriptorSet(context, state);
	buildCommandBuffers(context, state);

	glfwSetKeyCallback(context.window, key_callback);

	renderLoop(context, state);

	return 0;
}