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

//TODO: Take care of Deinit

void prepareSynchronizationPrimitives(struct LHContext& context, struct appState& state)
{
	VkResult U_ASSERT_ONLY res;

	// Semaphores (Used for correct command ordering)
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	// Semaphore used to ensures that image presentation is complete before starting to submit again
	res = (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &state.presentCompleteSemaphore));

	// Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
	res = (vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &state.renderCompleteSemaphore));

	// Fences (Used to check draw command buffer completion)
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// Create in signaled state so we don't wait on first render of each command buffer
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	state.waitFences.resize(context.cmdBuffer.size());
	for (auto& fence : state.waitFences)
	{
		res = (vkCreateFence(context.device, &fenceCreateInfo, nullptr, &fence));
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
	clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.renderPass = context.render_pass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = context.width;
	renderPassBeginInfo.renderArea.extent.height = context.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < context.cmdBuffer.size(); ++i)
	{
		// Set target frame buffer
		renderPassBeginInfo.framebuffer = context.frameBuffers[i];

		res = (vkBeginCommandBuffer(context.cmdBuffer[i], &cmdBufInfo));

		// Start the first sub pass specified in our default render pass setup by the base class
		// This will clear the color and depth attachment
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

		// Bind descriptor sets describing shader binding points
		vkCmdBindDescriptorSets(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelineLayout, 0, 1, &state.descriptorSet, 0, nullptr);

		// Bind the rendering pipeline
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		vkCmdBindPipeline(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);

		// Bind triangle vertex buffer (contains position and colors)
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(context.cmdBuffer[i], 0, 1, &state.v.buffer, offsets);

		// Bind triangle index buffer
		vkCmdBindIndexBuffer(context.cmdBuffer[i], state.i.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Draw indexed triangle
		vkCmdDrawIndexed(context.cmdBuffer[i], state.i.count, 1, 0, 0, 1);

		vkCmdEndRenderPass(context.cmdBuffer[i]);

		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to 
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

		res = (vkEndCommandBuffer(context.cmdBuffer[i]));
	}
}

void draw(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;
	// Get next image in the swap chain (back/front buffer)
	res = vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX, state.presentCompleteSemaphore, VK_NULL_HANDLE, &context.currentBuffer);

	// Use a fence to wait until the command buffer has finished execution before using it again
	res = (vkWaitForFences(context.device, 1, &state.waitFences[context.currentBuffer], VK_TRUE, UINT64_MAX));
	res = (vkResetFences(context.device, 1, &state.waitFences[context.currentBuffer]));

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

void setupDescriptorPool(struct LHContext& context, struct appState& state)
{
	VkResult U_ASSERT_ONLY res;

	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[1];
	// This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 1;
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

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
}

void setupDescriptorSetLayout(struct LHContext& context, struct appState& state)
{
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

	// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
	// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = nullptr;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &state.descriptorSetLayout;

	res = (vkCreatePipelineLayout(context.device, &pPipelineLayoutCreateInfo, nullptr, &state.pipelineLayout));
}

void setupDescriptorSet(struct LHContext& context, struct appState& state)
{
	VkResult U_ASSERT_ONLY res;

	// Allocate a new descriptor set from the global descriptor pool
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &state.descriptorSetLayout;

	res =(vkAllocateDescriptorSets(context.device, &allocInfo, &state.descriptorSet));

	// Update the descriptor set determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point

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

void setupDepthStencil(struct LHContext& context, struct appState& state){
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	// Create an optimal image used as the depth stencil attachment
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = context.depth.format;
	// Use example's height and width
	image.extent = {static_cast<uint32_t>( context.width), static_cast<uint32_t>(context.height), 1 };
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	res =(vkCreateImage(context.device, &image, nullptr, &context.depth.image));

	// Allocate memory for the image (device local) and bind it to our image
	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(context.device, context.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
	assert(pass && "No mappable coherent memory");	
	res =(vkAllocateMemory(context.device, &memAlloc, nullptr, &context.depth.mem));
	res =(vkBindImageMemory(context.device, context.depth.image, context.depth.mem, 0));

	// Create a view for the depth stencil image
	// Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
	// This allows for multiple views of one image with differing ranges (e.g. for different layers)
	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = context.depth.format;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = context.depth.image;
	res =(vkCreateImageView(context.device, &depthStencilView, nullptr, &context.depth.view));
}

void setupFrameBuffer(struct LHContext& context, struct appState& state, bool useStagingBuffers){
	VkResult U_ASSERT_ONLY res;
	// Create a frame buffer for every image in the swapchain
	context.frameBuffers.resize(context.swapchainImageCount);
	for (size_t i = 0; i < context.frameBuffers.size(); i++){
		std::array<VkImageView, 2> attachments;
		attachments[0] = context.buffers[i].view;									// Color attachment is the view of the swapchain image			
		attachments[1] = context.depth.view;											// Depth/Stencil attachment is the same for all frame buffers			

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// All frame buffers use the same renderpass setup
		frameBufferCreateInfo.renderPass = context.render_pass;
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameBufferCreateInfo.pAttachments = attachments.data();
		frameBufferCreateInfo.width = context.width;
		frameBufferCreateInfo.height = context.height;
		frameBufferCreateInfo.layers = 1;
		// Create the framebuffer
		res =(vkCreateFramebuffer(context.device, &frameBufferCreateInfo, nullptr, &context.frameBuffers[i]));
	}
}

void setupRenderPass(struct LHContext& context, struct appState& state, bool useStagingBuffers){
	VkResult U_ASSERT_ONLY res;
	// This example will use a single render pass with one subpass

	// Descriptors for the attachments used by this renderpass
	std::array<VkAttachmentDescription, 2> attachments = {};

	// Color attachment
	attachments[0].format = context.format;									// Use the color format selected by the swapchain
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;									// We don't use multi sampling in this example
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear this attachment at the start of the render pass
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;							// Keep it's contents after the render pass is finished (for displaying it)
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// We don't use stencil, so don't care for load
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Same for store
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// Layout to which the attachment is transitioned when the render pass is finished
																					// As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR	
	// Depth attachment
	attachments[1].format = context.depth.format;											// A proper depth format is selected in the example base
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at start of first subpass
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						// We don't need depth after render pass has finished (DONT_CARE may result in better performance)
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// No stencil
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// No Stencil
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// Transition to depth/stencil attachment

	// Setup attachment references
	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;													// Attachment 0 is color
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;				// Attachment layout used as color during the subpass

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;													// Attachment 1 is color
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		// Attachment used as depth/stemcil used during the subpass

	// Setup a single subpass reference
	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;									// Subpass uses one color attachment
	subpassDescription.pColorAttachments = &colorReference;							// Reference to the color attachment in slot 0
	subpassDescription.pDepthStencilAttachment = &depthReference;					// Reference to the depth attachment in slot 1
	subpassDescription.inputAttachmentCount = 0;									// Input attachments can be used to sample from contents of a previous subpass
	subpassDescription.pInputAttachments = nullptr;									// (Input attachments not used by this example)
	subpassDescription.preserveAttachmentCount = 0;									// Preserved attachments can be used to loop (and preserve) attachments through subpasses
	subpassDescription.pPreserveAttachments = nullptr;								// (Preserve attachments not used by this example)
	subpassDescription.pResolveAttachments = nullptr;								// Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

	// Setup subpass dependencies
	// These will add the implicit ttachment layout transitionss specified by the attachment descriptions
	// The actual usage layout is preserved through the layout specified in the attachment reference		
	// Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
	// srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
	// Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
	std::array<VkSubpassDependency, 2> dependencies;

	// First dependency at the start of the renderpass
	// Does the transition from final to initial layout 
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency 
	dependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Second dependency at the end the renderpass
	// Does the transition from the initial to the final layout
	dependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		// Number of attachments used by this render pass
	renderPassInfo.pAttachments = attachments.data();								// Descriptions of the attachments used by the render pass
	renderPassInfo.subpassCount = 1;												// We only use one subpass in this example
	renderPassInfo.pSubpasses = &subpassDescription;								// Description of that subpass
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());	// Number of subpass dependencies
	renderPassInfo.pDependencies = dependencies.data();								// Subpass dependencies used by the render pass

	res =(vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &context.render_pass));
}

VkShaderModule loadSPIRVShader(struct LHContext& context,std::string filename)
{
	VkResult U_ASSERT_ONLY res;
	size_t shaderSize;
	char* shaderCode = NULL;

#if defined(__ANDROID__)
	// Load shader from compressed asset
	AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
	assert(asset);
	shaderSize = AAsset_getLength(asset);
	assert(shaderSize > 0);

	shaderCode = new char[shaderSize];
	AAsset_read(asset, shaderCode, shaderSize);
	AAsset_close(asset);
#else
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		shaderSize = is.tellg();
		is.seekg(0, std::ios::beg);
		// Copy file contents into a buffer
		shaderCode = new char[shaderSize];
		is.read(shaderCode, shaderSize);
		is.close();
		assert(shaderSize > 0);
	}
#endif
	if (shaderCode)
	{
		// Create a new shader module that will be used for pipeline creation
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = shaderSize;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;

		VkShaderModule shaderModule;
		res =(vkCreateShaderModule(context.device, &moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
		return VK_NULL_HANDLE;
	}
}

void preparePipelines(struct LHContext& context, struct appState& state)
{
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

	state.uboVS.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));

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
	// This buffer will be used as a uniform buffer
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	// Create a new buffer
	res =(vkCreateBuffer(context.device, &bufferInfo, nullptr, &state.uniformBufferVS.buffer));
	// Get memory requirements including size, alignment and memory type 
	vkGetBufferMemoryRequirements(context.device, state.uniformBufferVS.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;
	// Get the memory type index that supports host visibile memory access
	// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
	// We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
	// Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
	pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocInfo.memoryTypeIndex);
	assert(pass && "No mappable coherent memory");	
	// Allocate memory for the uniform buffer
	res =(vkAllocateMemory(context.device, &allocInfo, nullptr, &(state.uniformBufferVS.memory)));
	// Bind memory to buffer
	res =(vkBindBufferMemory(context.device, state.uniformBufferVS.buffer, state.uniformBufferVS.memory, 0));

	// Store information in the uniform's descriptor that is used by the descriptor set
	state.uniformBufferVS.descriptor.buffer = state.uniformBufferVS.buffer;
	state.uniformBufferVS.descriptor.offset = 0;
	state.uniformBufferVS.descriptor.range = sizeof(state.uboVS);

	updateUniformBuffers(context, state);
}

void prepareVertices(struct LHContext& context, struct appState& state,bool useStagingBuffers){
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	// A note on memory management in Vulkan in general:
	//	This is a very complex topic and while it's fine for an example application to to small individual memory allocations that is not
	//	what should be done a real-world application, where you should allocate large chunkgs of memory at once isntead.

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
		//// Static data like vertex and index buffer should be stored on the device memory 
		//// for optimal (and fastest) access by the GPU
		////
		//// To achieve this we use so-called "staging buffers" :
		//// - Create a buffer that's visible to the host (and can be mapped)
		//// - Copy the data to this buffer
		//// - Create another buffer that's local on the device (VRAM) with the same size
		//// - Copy the data from the host to the device using a command buffer
		//// - Delete the host visible (staging) buffer
		//// - Use the device local buffers for rendering

		//struct StagingBuffer {
		//	VkDeviceMemory memory;
		//	VkBuffer buffer;
		//};

		//struct {
		//	StagingBuffer vertices;
		//	StagingBuffer indices;
		//} stagingBuffers;

		//// Vertex buffer
		//VkBufferCreateInfo vertexBufferInfo = {};
		//vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//vertexBufferInfo.size = vertexBufferSize;
		//// Buffer is used as the copy source
		//vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		//// Create a host-visible buffer to copy the vertex data to (staging buffer)
		//res =(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
		//vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
		//memAlloc.allocationSize = memReqs.size;
		//// Request a host visible memory type that can be used to copy our data do
		//// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
		//memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//res =(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
		//// Map and copy
		//res =(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
		//memcpy(data, vertexBuffer.data(), vertexBufferSize);
		//vkUnmapMemory(device, stagingBuffers.vertices.memory);
		//res =(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

		//// Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
		//vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		//res =(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertices.buffer));
		//vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
		//memAlloc.allocationSize = memReqs.size;
		//memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//res =(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
		//res =(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

		//// Index buffer
		//VkBufferCreateInfo indexbufferInfo = {};
		//indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//indexbufferInfo.size = indexBufferSize;
		//indexbufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		//// Copy index data to a buffer visible to the host (staging buffer)
		//res =(vkCreateBuffer(device, &indexbufferInfo, nullptr, &stagingBuffers.indices.buffer));
		//vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
		//memAlloc.allocationSize = memReqs.size;
		//memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		//res =(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
		//res =(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
		//memcpy(data, indexBuffer.data(), indexBufferSize);
		//vkUnmapMemory(device, stagingBuffers.indices.memory);
		//res =(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

		//// Create destination buffer with device only visibility
		//indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		//res =(vkCreateBuffer(device, &indexbufferInfo, nullptr, &indices.buffer));
		//vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
		//memAlloc.allocationSize = memReqs.size;
		//memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//res =(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
		//res =(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

		//// Buffer copies have to be submitted to a queue, so we need a command buffer for them
		//// Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
		//VkCommandBuffer copyCmd = getCommandBuffer(true);

		//// Put buffer region copies into command buffer
		//VkBufferCopy copyRegion = {};

		//// Vertex buffer
		//copyRegion.size = vertexBufferSize;
		//vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
		//// Index buffer
		//copyRegion.size = indexBufferSize;
		//vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer, 1, &copyRegion);

		//// Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
		//flushCommandBuffer(copyCmd);

		//// Destroy staging buffers
		//// Note: Staging buffer must not be deleted before the copies have been submitted and executed
		//vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
		//vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
		//vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
		//vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
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
		updateUniformBuffers(context, state);
	}

	// Flush device to make sure all resources can be freed
	if (context.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(context.device);
	}
}

int main() {
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

	renderLoop(context, state);

	return 0;
}