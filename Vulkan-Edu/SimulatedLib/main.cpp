#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

#include "LHVulkan.h"

#define VK_USE_PLATFORM_WIN32_KHR

#define WIDTH 512
#define HEIGHT 512
struct Vertex {
	float position[3];
	float color[3];
};

//Custom States depending on what is needed
struct appState{
	struct Vertex {
		float position[3];
		float color[3];
	}vertex;

	struct vertices v;
	// Index buffer
	struct indices i;

	// Uniform buffer block object
	struct {
		VkDeviceMemory memory;
		VkBuffer buffer;
		VkDescriptorBufferInfo descriptor;
	}  uniformBufferVS;

	struct {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	} uboVS;

	glm::mat4 Projection;
	glm::mat4 Clip;

	float* vBuffer;
	VkBuffer iBuffer;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkSemaphore presentCompleteSemaphore;
	VkSemaphore renderCompleteSemaphore;
	std::vector<VkFence> waitFences;
};

glm::vec3 rotation = glm::vec3();
glm::vec3 cameraPos = glm::vec3();
glm::vec2 mousePos;

bool update = false;
float eyex, eyey, eyez;	// current user position

double theta, phi;		// user's position  on a sphere centered on the object
double r;

//TODO: Take care of Deinit

void prepareSynchronizationPrimitives(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;

	// Semaphores (Used for correct command ordering)
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	// Semaphore used to ensures that image presentation is complete before starting to submit again
	res = (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &state.presentCompleteSemaphore));
	assert(res == VK_SUCCESS);
	// Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
	res = (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &state.renderCompleteSemaphore));
	assert(res == VK_SUCCESS);

	// Fences (Used to check draw command buffer completion)
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// Create in signaled state so we don't wait on first render of each command buffer
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	state.waitFences.resize(context.cmdBuffer.size());
	for (auto& fence : state.waitFences){
		res = (vkCreateFence(context.device, &fenceCreateInfo, nullptr, &fence));
		assert(res == VK_SUCCESS);
	}
}

void buildCommandBuffers(struct LHContext& context, struct appState& state){
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

	for (int32_t i = 0; i < context.cmdBuffer.size(); ++i){
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = context.frameBuffers[i];

		res = (vkBeginCommandBuffer(context.cmdBuffer[i], &cmdBufInfo));
		assert(res == VK_SUCCESS);

		vkCmdBeginRenderPass(context.cmdBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Update dynamic viewport state
		VkViewport viewport = {};
		viewport.height = (float)context.height;
		viewport.width = (float)context.width;
		viewport.minDepth = (float)0.0f;
		viewport.maxDepth = (float)1.0f;
		vkCmdSetViewport(context.cmdBuffer[i], 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = context.width;
		scissor.extent.height = context.height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(context.cmdBuffer[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelineLayout, 0, 1, &state.descriptorSet, 0, nullptr);

		vkCmdBindPipeline(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(context.cmdBuffer[i], 0, 1, &state.v.buffer, offsets);

		vkCmdBindIndexBuffer(context.cmdBuffer[i], state.i.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(context.cmdBuffer[i], state.i.count, 1, 0, 0, 1);

		vkCmdEndRenderPass(context.cmdBuffer[i]);

		res = (vkEndCommandBuffer(context.cmdBuffer[i]));
		assert(res == VK_SUCCESS);
	}
}

void draw(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;
	// Get next image in the swap chain (back/front buffer)
	res = vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX, state.presentCompleteSemaphore, VK_NULL_HANDLE, &context.currentBuffer);
	assert(res == VK_SUCCESS);
	// Use a fence to wait until the command buffer has finished execution before using it again
	res = (vkWaitForFences(context.device, 1, &state.waitFences[context.currentBuffer], VK_TRUE, UINT64_MAX));
	assert(res == VK_SUCCESS);
	res = (vkResetFences(context.device, 1, &state.waitFences[context.currentBuffer]));
	assert(res == VK_SUCCESS);

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &state.presentCompleteSemaphore;							// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &state.renderCompleteSemaphore;						// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &context.cmdBuffer[context.currentBuffer];					// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;												// One command buffer

	// Submit to the graphics queue passing a wait fence
	res = (vkQueueSubmit(context.queue, 1, &submitInfo, state.waitFences[context.currentBuffer]));
	assert(res == VK_SUCCESS);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &context.swapChain;
	presentInfo.pImageIndices = &context.currentBuffer;
	// Check if a wait semaphore has been specified to wait for before presenting the image
	if (state.renderCompleteSemaphore != VK_NULL_HANDLE){
		presentInfo.pWaitSemaphores = &state.renderCompleteSemaphore;
		presentInfo.waitSemaphoreCount = 1;
	}
	res = vkQueuePresentKHR(context.queue, &presentInfo);
	assert(res == VK_SUCCESS);
}

void setupDescriptorPool(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;

	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[1];
	// This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
	descriptorPoolInfo.maxSets = 1;

	res = (vkCreateDescriptorPool(context.device, &descriptorPoolInfo, nullptr, &context.descriptorPool));
	assert(res == VK_SUCCESS);
}

void setupDescriptorSetLayout(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;

	// Setup layout of descriptors used in this example
	// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
	// So every shader binding should map to one descriptor set layout binding

	// Binding 0: Uniform buffer (Vertex shader)
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = nullptr;
	descriptorLayout.bindingCount = 1;
	descriptorLayout.pBindings = &layoutBinding;

	res = (vkCreateDescriptorSetLayout(context.device, &descriptorLayout, nullptr, &state.descriptorSetLayout));
	assert(res == VK_SUCCESS);

	// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
	// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = nullptr;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &state.descriptorSetLayout;

	res = (vkCreatePipelineLayout(context.device, &pPipelineLayoutCreateInfo, nullptr, &state.pipelineLayout));
	assert(res == VK_SUCCESS);
}

void setupDescriptorSet(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;

	// Allocate a new descriptor set from the global descriptor pool
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &state.descriptorSetLayout;

	res =(vkAllocateDescriptorSets(context.device, &allocInfo, &state.descriptorSet));
	assert(res == VK_SUCCESS);

	VkWriteDescriptorSet writeDescriptorSet = {};

	// Binding 0 : Uniform buffer
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = state.descriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &state.uniformBufferVS.descriptor;
	// Binds this uniform buffer to binding point 0
	writeDescriptorSet.dstBinding = 0;

	vkUpdateDescriptorSets(context.device, 1, &writeDescriptorSet, 0, nullptr);
}

void preparePipelines(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;
	// Create the graphics pipeline used in this example
	// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
	// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
	// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
	pipelineCreateInfo.layout = state.pipelineLayout;
	// Renderpass this pipeline is attached to
	pipelineCreateInfo.renderPass = context.render_pass;

	// Construct the differnent states making up the pipeline

	// Input assembly state describes how primitives are assembled
	// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
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

	// Vertex input descriptions 
	// Specifies the vertex input parameters for a pipeline

	// Vertex input binding
	// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(Vertex);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Inpute attribute bindings describe shader attribute locations and memory layouts
	std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
	// These match the following shader layout (see triangle.vert):
	//	layout (location = 0) in vec3 inPos;
	//	layout (location = 1) in vec3 inColor;
	// Attribute location 0: Position
	vertexInputAttributs[0].binding = 0;
	vertexInputAttributs[0].location = 0;
	// Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
	vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs[0].offset = offsetof(Vertex, position);
	// Attribute location 1: Color
	vertexInputAttributs[1].binding = 0;
	vertexInputAttributs[1].location = 1;
	// Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
	vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs[1].offset = offsetof(Vertex, color);

	// Vertex input state used for pipeline creation
	VkPipelineVertexInputStateCreateInfo vertexInputState = {};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputState.vertexAttributeDescriptionCount = 2;
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

	// Shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

	// Vertex shader
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	// Set pipeline stage for this shader
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	// Load binary SPIR-V shader
	shaderStages[0].module = loadSPIRVShader(context, "./../data/shaders/triangle/triangle.vert.spv");
	// Main entry point for the shader
	shaderStages[0].pName = "main";
	assert(shaderStages[0].module != VK_NULL_HANDLE);

	// Fragment shader
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	// Set pipeline stage for this shader
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	// Load binary SPIR-V shader
	shaderStages[1].module = loadSPIRVShader(context, "./../data/shaders/triangle/triangle.frag.spv");
	// Main entry point for the shader
	shaderStages[1].pName = "main";
	assert(shaderStages[1].module != VK_NULL_HANDLE);

	// Set pipeline shader stage info
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	// Assign the pipeline states to the pipeline creation info structure
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.renderPass = context.render_pass;
	pipelineCreateInfo.pDynamicState = &dynamicState;

	// Create rendering pipeline using the specified states
	res =(vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineCreateInfo, nullptr, &state.pipeline));

	// Shader modules are no longer needed once the graphics pipeline has been created
	vkDestroyShaderModule(context.device, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(context.device, shaderStages[1].module, nullptr);
}

void updateUniformBuffers(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;
	// Update matrices
	state.uboVS.projectionMatrix = glm::perspective(glm::radians(60.0f), (float)context.width / (float)context.height, 0.1f, 256.0f);

	//state.uboVS.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));

	state.uboVS.viewMatrix = glm::lookAt(glm::vec3(eyex, eyey, eyez),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	state.uboVS.modelMatrix = glm::mat4(1.0f);
	state.uboVS.modelMatrix = glm::rotate(state.uboVS.modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	state.uboVS.modelMatrix = glm::rotate(state.uboVS.modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	state.uboVS.modelMatrix = glm::rotate(state.uboVS.modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// Map uniform buffer and update it
	uint8_t* pData;
	res = (vkMapMemory(context.device, state.uniformBufferVS.memory, 0, sizeof(state.uboVS), 0, (void**)&pData));
	memcpy(pData, &state.uboVS, sizeof(state.uboVS));
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vkUnmapMemory(context.device, state.uniformBufferVS.memory);
}

void prepareUniformBuffers(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	// Prepare and initialize a uniform buffer block containing shader uniforms
	// Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks
	VkMemoryRequirements memReqs;

	// Vertex shader uniform buffer block
	VkBufferCreateInfo bufferInfo = {};
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = 0;
	allocInfo.memoryTypeIndex = 0;

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(state.uboVS);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	// Create a new buffer
	res =(vkCreateBuffer(context.device, &bufferInfo, nullptr, &state.uniformBufferVS.buffer));
	assert(res == VK_SUCCESS);
	vkGetBufferMemoryRequirements(context.device, state.uniformBufferVS.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;
	pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocInfo.memoryTypeIndex);
	assert(pass && "No mappable coherent memory");	
	res =(vkAllocateMemory(context.device, &allocInfo, nullptr, &(state.uniformBufferVS.memory)));
	assert(res == VK_SUCCESS);
	res =(vkBindBufferMemory(context.device, state.uniformBufferVS.buffer, state.uniformBufferVS.memory, 0));
	assert(res == VK_SUCCESS);
	// Store information in the uniform's descriptor that is used by the descriptor set
	state.uniformBufferVS.descriptor.buffer = state.uniformBufferVS.buffer;
	state.uniformBufferVS.descriptor.offset = 0;
	state.uniformBufferVS.descriptor.range = sizeof(state.uboVS);

	updateUniformBuffers(context, state);
}

void prepareVertices(struct LHContext& context, struct appState& state,bool useStagingBuffers){
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	// Setup vertices
	std::vector<Vertex> vertexBuffer ={
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};
	uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

	// Setup indices
	std::vector<uint32_t> indexBuffer = { 0, 1, 2 };
	state.i.count = static_cast<uint32_t>(indexBuffer.size());
	uint32_t indexBufferSize = state.i.count * sizeof(uint32_t);

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;

	void* data;

	if (useStagingBuffers){
		//TODO Make Staging code
	}
	else{
		// Don't use staging
		// Create host-visible buffers only and use these for rendering. This is not advised and will usually result in lower rendering performance

		// Vertex buffer
		VkBufferCreateInfo vertexBufferInfo = {};
		vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertexBufferInfo.size = vertexBufferSize;
		vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		
		// Copy vertex data to a buffer visible to the host
		res = (vkCreateBuffer(context.device, &vertexBufferInfo, nullptr, &state.v.buffer));
		assert(res == VK_SUCCESS);
		vkGetBufferMemoryRequirements(context.device, state.v.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;

		pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memAlloc.memoryTypeIndex);
		assert(pass && "No mappable coherent memory");		
		res = (vkAllocateMemory(context.device, &memAlloc, nullptr, &state.v.memory));
		assert(res == VK_SUCCESS);
		res = (vkMapMemory(context.device, state.v.memory, 0, memAlloc.allocationSize, 0, &data));
		assert(res == VK_SUCCESS);
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(context.device, state.v.memory);
		res = (vkBindBufferMemory(context.device, state.v.buffer, state.v.memory, 0));
		assert(res == VK_SUCCESS);

		// Index buffer
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.size = indexBufferSize;
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		// Copy index data to a buffer visible to the host
		res = (vkCreateBuffer(context.device, &indexbufferInfo, nullptr, &state.i.buffer));
		assert(res == VK_SUCCESS);
		vkGetBufferMemoryRequirements(context.device, state.i.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memAlloc.memoryTypeIndex);
		assert(pass && "No mappable coherent memory");
		res = (vkAllocateMemory(context.device, &memAlloc, nullptr, &state.i.memory));
		assert(res == VK_SUCCESS);
		res = (vkMapMemory(context.device, state.i.memory, 0, indexBufferSize, 0, &data));
		assert(res == VK_SUCCESS);
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(context.device, state.i.memory);
		res = (vkBindBufferMemory(context.device, state.i.buffer, state.i.memory, 0));
		assert(res == VK_SUCCESS);
	}
}

void renderLoop(struct LHContext &context, struct appState& state){

	while (!glfwWindowShouldClose(context.window)) {
		glfwPollEvents();
		draw(context, state);
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
	if (key == GLFW_KEY_D && action == GLFW_PRESS){
		phi += 0.1;
		update = true;
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS){
		theta += 0.1;
		update = true;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS){
		theta -= 0.1;
		update = true;
	}

	eyex = (float)(r * sin(theta) * cos(phi));
	eyey = (float)(r * sin(theta) * sin(phi));
	eyez = (float)(r * cos(theta));

}

int main() {
	eyex = 0.0;
	eyez = 0.0;
	eyey = 7.0;

	theta = 1.5;
	phi = 1.5;
	r = 10.0;


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

	//---> Implement our own functions
	prepareSynchronizationPrimitives(context, state);
	prepareVertices(context, state, false);
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