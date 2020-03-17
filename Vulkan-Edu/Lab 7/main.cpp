#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

#include "LHVulkan.h"
#include "cube_data.h"
#include "tiny_obj_loader.h"

#define PLANE_MESH
#define OBJ_MESH
#define WIDTH 512
#define HEIGHT 512


//Custom States depending on what is needed
struct appState {

	//Vertex buffer
	struct vertices v[2];
	// Index buffer
	struct indices i[2];
	float* vBuffer;

	// Uniform buffer block object
	struct {
		VkDeviceMemory memory;
		VkBuffer buffer;
		VkDescriptorBufferInfo descriptor;
	}  uniformBufferVS[3];

	glm::vec3 lighPos = glm::vec3();

	struct {
		glm::mat4 depthMVP;
	} uboOffscreenVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVSquad;

	struct {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::vec3 lightPos;
		glm::mat4 depthBiasMVP;
	} uboVSscene;


	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	};

	struct OffscreenPass {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment depth;
		VkRenderPass renderPass;
		VkSampler depthSampler;
		VkDescriptorImageInfo descriptor;
	} offscreenPass;

	VkPipelineLayout pipelineLayout;
	struct {
		VkPipeline quad;
		VkPipeline offscreen;
		VkPipeline sceneShadow;
		VkPipeline sceneShadowPCF;
	} pipelines;
	struct {
		VkPipelineLayout quad;
		VkPipelineLayout offscreen;
	} pipelineLayouts;

	struct {
		VkDescriptorSet offscreen;
		VkDescriptorSet scene;
	} descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	VkVertexInputBindingDescription vertexInputBinding[2];
	std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributs;
};

glm::vec3 rotation = glm::vec3();
glm::vec3 cameraPos = glm::vec3();
glm::vec2 mousePos;
bool filterPCF = true;
bool update = false;
float eyex, eyey, eyez;	// current user position

double theta, phi;		// user's position  on a sphere centered on the object
double r;
int triangles;			// number of triangles


// Set up a separate render pass for the offscreen frame buffer
// This is necessary as the offscreen frame buffer attachments use formats different to those from the example render pass
void prepareOffscreenRenderpass(struct LHContext &context, struct appState &state){
	VkResult U_ASSERT_ONLY res;
	
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = VK_FORMAT_D16_UNORM;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassCreateInfo.pDependencies = dependencies.data();

	res = (vkCreateRenderPass(context.device, &renderPassCreateInfo, nullptr, &state.offscreenPass.renderPass));
	assert(res == VK_SUCCESS);
}

// Setup the offscreen framebuffer for rendering the scene from light's point-of-view to
// The depth attachment of this framebuffer will then be used to sample from in the fragment shader of the shadowing pass
void prepareShadowFramebuffer(struct LHContext &context, struct appState&state) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	state.offscreenPass.width = 1280;
	state.offscreenPass.height = 720;

	// For shadow mapping we only need a depth attachment
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = state.offscreenPass.width;
	image.extent.height = state.offscreenPass.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = VK_FORMAT_D16_UNORM;																// Depth stencil attachment
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping
	res =(vkCreateImage(context.device, &image, nullptr, &state.offscreenPass.depth.image));
	assert(res == VK_SUCCESS);

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(context.device, state.offscreenPass.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
	assert(pass);

	res = (vkAllocateMemory(context.device, &memAlloc, nullptr, &state.offscreenPass.depth.mem));
	assert(res == VK_SUCCESS);
	res = (vkBindImageMemory(context.device, state.offscreenPass.depth.image, state.offscreenPass.depth.mem, 0));
	assert(res == VK_SUCCESS);

	VkImageViewCreateInfo depthStencilView = {};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = VK_FORMAT_D16_UNORM;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = state.offscreenPass.depth.image;
	res = (vkCreateImageView(context.device, &depthStencilView, nullptr, &state.offscreenPass.depth.view));
	assert(res == VK_SUCCESS);
	// Create sampler to sample from to depth attachment 
	// Used to sample in the fragment shader for shadowed rendering
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.maxAnisotropy = 1.0f;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	res = (vkCreateSampler(context.device, &sampler, nullptr, &state.offscreenPass.depthSampler));
	assert(res == VK_SUCCESS);

	prepareOffscreenRenderpass(context, state);

	// Create frame buffer
	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = state.offscreenPass.renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = &state.offscreenPass.depth.view;
	fbufCreateInfo.width = state.offscreenPass.width;
	fbufCreateInfo.height = state.offscreenPass.height;
	fbufCreateInfo.layers = 1;

	res = (vkCreateFramebuffer(context.device, &fbufCreateInfo, nullptr, &state.offscreenPass.frameBuffer));
	assert(res == VK_SUCCESS);
}

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
		{
			clearValues[0].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = state.offscreenPass.renderPass;
			renderPassBeginInfo.framebuffer = state.offscreenPass.frameBuffer;
			renderPassBeginInfo.renderArea.extent.width = state.offscreenPass.width;
			renderPassBeginInfo.renderArea.extent.height = state.offscreenPass.height;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(context.cmdBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			createViewports(context, context.cmdBuffer[i], viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			createScisscor(context, context.cmdBuffer[i], scissor);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			vkCmdSetDepthBias(
				context.cmdBuffer[i],
				1.25f,
				0.0f,
				1.75f);
			VkDeviceSize offsets[1] = { 0 };

			vkCmdBindPipeline(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelines.offscreen);
			vkCmdBindDescriptorSets(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelineLayouts.offscreen, 0, 1, &state.descriptorSets.offscreen, 0, NULL);

			vkCmdBindVertexBuffers(context.cmdBuffer[i], 0, 1, &state.v[0].buffer, offsets);
			vkCmdBindIndexBuffer(context.cmdBuffer[i], state.i[0].buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(context.cmdBuffer[i], state.i[0].count, 1, 0, 0, 0);

			vkCmdEndRenderPass(context.cmdBuffer[i]);
		}
		{
			{
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = context.render_pass;
				renderPassBeginInfo.framebuffer = context.frameBuffers[i];
				renderPassBeginInfo.renderArea.extent.width = context.width;
				renderPassBeginInfo.renderArea.extent.height = context.height;
				renderPassBeginInfo.clearValueCount = 2;
				renderPassBeginInfo.pClearValues = clearValues;

				vkCmdBeginRenderPass(context.cmdBuffer[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Update dynamic viewport state
				VkViewport viewport = {};
				createViewports(context, context.cmdBuffer[i], viewport);

				// Update dynamic scissor state
				VkRect2D scissor = {};
				createScisscor(context, context.cmdBuffer[i], scissor);
				VkDeviceSize offsets[1] = { 0 };

				// Visualize shadow map
				if (true) {
					vkCmdBindDescriptorSets(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelineLayouts.quad, 0, 1, &state.descriptorSet, 0, NULL);
					vkCmdBindPipeline(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelines.quad);
					vkCmdBindVertexBuffers(context.cmdBuffer[i], 0, 1, &state.v[1].buffer, offsets);
					vkCmdBindIndexBuffer(context.cmdBuffer[i], state.i[1].buffer, 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(context.cmdBuffer[i], state.i[1].count, 1, 0, 0, 0);
				}

				// 3D scene
				vkCmdBindDescriptorSets(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipelineLayouts.quad, 0, 1, &state.descriptorSets.scene, 0, NULL);
				vkCmdBindPipeline(context.cmdBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (filterPCF) ? state.pipelines.sceneShadowPCF : state.pipelines.sceneShadow);

				vkCmdBindVertexBuffers(context.cmdBuffer[i], 0, 1, &state.v[0].buffer, offsets);
				vkCmdBindIndexBuffer(context.cmdBuffer[i], state.i[0].buffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(context.cmdBuffer[i], state.i[0].count, 1, 0, 0, 0);

				vkCmdEndRenderPass(context.cmdBuffer[i]);
			}

			res = (vkEndCommandBuffer(context.cmdBuffer[i]));
			assert(res == VK_SUCCESS);
		}
	}
}

void setupDescriptorPool(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	// We need to tell the API the number of max. requested descriptors per type
	VkDescriptorPoolSize typeCounts[2];
	// This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
	typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	typeCounts[0].descriptorCount = 6;

	typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	typeCounts[1].descriptorCount = 4;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = 2;
	descriptorPoolInfo.pPoolSizes = typeCounts;
	// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
	descriptorPoolInfo.maxSets = 3;

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
		layout (set = 0, binding = 1) uniform imagesampler ...;

	*/


	// Binding 0: Uniform buffer (Vertex shader)
	std::array<VkDescriptorSetLayoutBinding, 2>layoutBinding = {};
	layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding[0].descriptorCount = 1;
	layoutBinding[0].binding = 0;
	layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding[0].pImmutableSamplers = nullptr;

	// Binding 0: Uniform buffer (Fragment shader)
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

void setupDescriptorSet(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	// Allocate a new descriptor set from the global descriptor pool
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &state.descriptorSetLayout;

	res = (vkAllocateDescriptorSets(context.device, &allocInfo, &state.descriptorSet));
	assert(res == VK_SUCCESS);

	VkDescriptorImageInfo texDescriptor = {};
	texDescriptor.sampler = state.offscreenPass.depthSampler;
	texDescriptor.imageView = state.offscreenPass.depth.view;
	texDescriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	std::array<VkWriteDescriptorSet, 2> writeDescriptorSet = {};

	// Binding 0 : Vertex shader uniform buffer
	writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[0].dstSet = state.descriptorSet;
	writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet[0].dstBinding = 0;
	writeDescriptorSet[0].pBufferInfo = &state.uniformBufferVS[1].descriptor;
	writeDescriptorSet[0].descriptorCount = 1;
		
		
	// Binding 1 : Fragment shader texture sampler
	writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[1].dstSet = state.descriptorSet;
	writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet[1].dstBinding = 1;
	writeDescriptorSet[1].pImageInfo = &texDescriptor;;
	writeDescriptorSet[1].descriptorCount = 1;
	vkUpdateDescriptorSets(context.device, writeDescriptorSet.size(), writeDescriptorSet.data(), 0, NULL);

	// Offscreen
	res = (vkAllocateDescriptorSets(context.device, &allocInfo, &state.descriptorSets.offscreen));

	std::array<VkWriteDescriptorSet, 1> writeDescriptorSets = {};
	
	// Binding 0 : Vertex shader uniform buffer
	writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[0].dstSet = state.descriptorSets.offscreen;
	writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet[0].dstBinding = 0;
	writeDescriptorSet[0].pBufferInfo = &state.uniformBufferVS[2].descriptor;
	writeDescriptorSet[0].descriptorCount = 1;

	vkUpdateDescriptorSets(context.device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

	// 3D scene
	res = (vkAllocateDescriptorSets(context.device, &allocInfo, &state.descriptorSets.scene));

	// Image descriptor for the shadow map attachment
	texDescriptor.sampler = state.offscreenPass.depthSampler;
	texDescriptor.imageView = state.offscreenPass.depth.view;

	std::array<VkWriteDescriptorSet, 2> writeDescriptorSetss = {};
		// Binding 0 : Vertex shader uniform buffer
	writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[0].dstSet = state.descriptorSets.scene;
	writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet[0].dstBinding = 0;
	writeDescriptorSet[0].pBufferInfo = &state.uniformBufferVS[0].descriptor;
	writeDescriptorSet[0].descriptorCount = 1;


	// Binding 1 : Fragment shader texture sampler
	writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet[1].dstSet = state.descriptorSets.scene;
	writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet[1].dstBinding = 1;
	writeDescriptorSet[1].pImageInfo = &texDescriptor;;
	writeDescriptorSet[1].descriptorCount = 1;
	vkUpdateDescriptorSets(context.device, writeDescriptorSetss.size(), writeDescriptorSetss.data(), 0, NULL);
}

void preparePipelines(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
	pipelineCreateInfo.layout = state.pipelineLayout;
	// Renderpass this pipeline is attached to
	pipelineCreateInfo.renderPass = context.render_pass;

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
	vertexInputState.pVertexBindingDescriptions = &state.vertexInputBinding[0];
	vertexInputState.vertexAttributeDescriptionCount = 3;
	vertexInputState.pVertexAttributeDescriptions = state.vertexInputAttributs.data();

	// Set pipeline shader stage info
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(state.shaderStages.size());
	pipelineCreateInfo.pStages = state.shaderStages.data();

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

	//Takes care of the QUAD
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	// Vertex shader
	createShaderStage(context, "./shaders/shaderquad.vert", VK_SHADER_STAGE_VERTEX_BIT, state.shaderStages[0]);
	assert(state.shaderStages[0].module != VK_NULL_HANDLE);

	// Fragment shader
	createShaderStage(context, "./shaders/shaderquad.frag", VK_SHADER_STAGE_FRAGMENT_BIT, state.shaderStages[1]);
	assert(state.shaderStages[1].module != VK_NULL_HANDLE);

	VkPipelineVertexInputStateCreateInfo emptyInputState = {};
	emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineCreateInfo.pVertexInputState = &emptyInputState;

	// Create rendering pipeline using the specified states
	res = (vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineCreateInfo, nullptr, &state.pipelines.quad));
	assert(res == VK_SUCCESS);

	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

	// Vertex shader
	createShaderStage(context, "./shaders/shader.vert", VK_SHADER_STAGE_VERTEX_BIT, state.shaderStages[0]);
	assert(state.shaderStages[0].module != VK_NULL_HANDLE);

	// Fragment shader
	createShaderStage(context, "./shaders/shader.frag", VK_SHADER_STAGE_FRAGMENT_BIT, state.shaderStages[1]);
	assert(state.shaderStages[1].module != VK_NULL_HANDLE);

	uint32_t enablePCF = 0;
	VkSpecializationMapEntry specializationMapEntry = {};
	specializationMapEntry.constantID = 0;
	specializationMapEntry.offset = 0;
	specializationMapEntry.size = sizeof(uint32_t);
	VkSpecializationInfo specializationInfo = {};
	specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &specializationMapEntry;
	specializationInfo.dataSize = sizeof(uint32_t);
	specializationInfo.pData = &enablePCF;
	state.shaderStages[1].pSpecializationInfo = &specializationInfo;
	// No filtering
	res = (vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineCreateInfo, nullptr, &state.pipelines.sceneShadow));
	assert(res == VK_SUCCESS);
	// PCF filtering
	enablePCF = 1;
	res = (vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineCreateInfo, nullptr, &state.pipelines.sceneShadowPCF));
	assert(res == VK_SUCCESS);

	// Offscreen pipeline (vertex shader only)
	createShaderStage(context, "./shaders/shaderOffscree.vert", VK_SHADER_STAGE_VERTEX_BIT, state.shaderStages[0]);
	assert(state.shaderStages[0].module != VK_NULL_HANDLE);
	pipelineCreateInfo.stageCount = 1;
	// No blend attachment states (no color attachments used)
	colorBlendState.attachmentCount = 0;
	// Cull front faces
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	// Enable depth bias
	rasterizationState.depthBiasEnable = VK_TRUE;
	// Add depth bias to dynamic state, so we can change it at runtime
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
	dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	dynamicState.dynamicStateCount = dynamicStateEnables.size();
	dynamicState.flags = 0;

	pipelineCreateInfo.layout = state.pipelineLayouts.offscreen;
	pipelineCreateInfo.renderPass = state.offscreenPass.renderPass;
	res = (vkCreateGraphicsPipelines(context.device, context.pipelineCache, 1, &pipelineCreateInfo, nullptr, &state.pipelines.offscreen));
	assert(res == VK_SUCCESS);
}

void updateUniformBuffers(struct LHContext& context, struct appState& state) {
	VkResult U_ASSERT_ONLY res;
	uint8_t* pData;
	//setup Light Pos
	state.lighPos = glm::vec3(1.0, 0.0, 0.5);

	// Matrix from light's point of view
	glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 96.0f);
	glm::mat4 depthViewMatrix = glm::lookAt(state.lighPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	glm::mat4 depthModelMatrix = glm::mat4(1.0f);

	state.uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	// Map uniform buffer and update it
	res = (vkMapMemory(context.device, state.uniformBufferVS[2].memory, 0, sizeof(state.uboOffscreenVS), 0, (void**)&pData));
	memcpy(pData, &state.uboOffscreenVS, sizeof(state.uboOffscreenVS));
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vkUnmapMemory(context.device, state.uniformBufferVS[2].memory);

	float AR = (float)context.height / (float)context.width;

	// 3D scene
	state.uboVSscene.projectionMatrix = glm::perspective(glm::radians(45.0f), (float)context.width / (float)context.height, 1.0f, 96.0f);

	state.uboVSscene.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0));
	state.uboVSscene.viewMatrix = glm::rotate(state.uboVSscene.viewMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	state.uboVSscene.viewMatrix = glm::rotate(state.uboVSscene.viewMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	state.uboVSscene.viewMatrix = glm::rotate(state.uboVSscene.viewMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	state.uboVSscene.modelMatrix = glm::mat4(1.0f);

	state.uboVSscene.lightPos = state.lighPos;

	state.uboVSscene.depthBiasMVP = state.uboOffscreenVS.depthMVP;


	// Map uniform buffer and update it
	res = (vkMapMemory(context.device, state.uniformBufferVS[0].memory, 0, sizeof(state.uboVSscene), 0, (void**)&pData));
	memcpy(pData, &state.uboVSscene, sizeof(state.uboVSscene));
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vkUnmapMemory(context.device, state.uniformBufferVS[0].memory);

}

void prepareUniformBuffers(struct LHContext& context, struct appState& state) {
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
	bufferInfo.size = sizeof(state.uboVSscene);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	bindBufferToMem(context, bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		state.uniformBufferVS[0].buffer, state.uniformBufferVS[0].memory);

	// Store information in the uniform's descriptor that is used by the descriptor set
	state.uniformBufferVS[0].descriptor.buffer = state.uniformBufferVS[0].buffer;
	state.uniformBufferVS[0].descriptor.offset = 0;
	state.uniformBufferVS[0].descriptor.range = sizeof(state.uboVSscene);

	bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(state.uboVSquad);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	bindBufferToMem(context, bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		state.uniformBufferVS[1].buffer, state.uniformBufferVS[1].memory);

	// Store information in the uniform's descriptor that is used by the descriptor set
	state.uniformBufferVS[1].descriptor.buffer = state.uniformBufferVS[1].buffer;
	state.uniformBufferVS[1].descriptor.offset = 0;
	state.uniformBufferVS[1].descriptor.range = sizeof(state.uboVSquad);

	bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(state.uboOffscreenVS);
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	bindBufferToMem(context, bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		state.uniformBufferVS[2].buffer, state.uniformBufferVS[2].memory);

	// Store information in the uniform's descriptor that is used by the descriptor set
	state.uniformBufferVS[2].descriptor.buffer = state.uniformBufferVS[2].buffer;
	state.uniformBufferVS[2].descriptor.offset = 0;
	state.uniformBufferVS[2].descriptor.range = sizeof(state.uboOffscreenVS);

	updateUniformBuffers(context, state);
}


#ifdef OBJ_MESH
void prepareVertices(struct LHContext& context, struct appState& state, std::string filepath,int index,bool useStagingBuffers) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	float* vertices;
	float* normals;
	float* textcoord;
	uint32_t* indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	int nv;
	int nn;
	int ni;
	int nt;
	int i;

	VkBufferCreateInfo buf_info = {};
	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo alloc_info = {};
	uint8_t* pData;

	std::string err = tinyobj::LoadObj(shapes, materials, filepath.c_str(), 0);

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

	/*  Retrieve the textcoord */

	nt = (int)shapes[0].mesh.texcoords.size();
	textcoord = new float[nt];
	for (i = 0; i < nt; i++) {
		textcoord[i] = shapes[0].mesh.texcoords[i];
	}


	state.vBuffer = new float[nv + nn + nt];
	int k = 0;
	for (i = 0; i < nv / 3; i++) {
		state.vBuffer[k++] = vertices[3 * i];
		state.vBuffer[k++] = vertices[3 * i + 1];
		state.vBuffer[k++] = vertices[3 * i + 2];
		state.vBuffer[k++] = normals[3 * i];
		state.vBuffer[k++] = normals[3 * i + 1];
		state.vBuffer[k++] = normals[3 * i + 2];
		state.vBuffer[k++] = textcoord[2 * i];
		state.vBuffer[k++] = textcoord[2 * i + 1];
	}

	uint32_t dataSize = (nv + nn + nt) * sizeof(state.vBuffer[0]);
	uint32_t dataStride = 8 * (sizeof(float));

	state.i[index].count = ni;

	mapIndiciesToGPU(context, indices, sizeof(indices[0]) * ni, state.i[index].buffer, state.i[index].memory);
	mapVerticiesToGPU(context, state.vBuffer, dataSize, state.v[index].buffer, state.v[index].memory);

	//// Vertex input descriptions 
	//// Specifies the vertex input parameters for a pipeline

	//// Vertex input binding
	//// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)

	state.vertexInputBinding[0].binding = 0;
	state.vertexInputBinding[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	state.vertexInputBinding[0].stride = dataStride;


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
	state.vertexInputAttributs[2].binding = 0;
	state.vertexInputAttributs[2].location = 2;
	state.vertexInputAttributs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	state.vertexInputAttributs[2].offset = 2*sizeof(float);

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
	prepareShadowFramebuffer(context, state);
	prepareVertices(context, state, "angryteapot.obj",0,false);
	prepareVertices(context, state, "plane.obj",1,false);
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