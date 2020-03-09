#ifndef L_H_VULKAN_H
#define L_H_VULKAN_H

#define _CRT_SECURE_NO_WARNINGS

#ifndef NOMINMAX
#define NOMINMAX /* Don't let Windows define min() or max() */
#endif

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_EXPOSE_NATIVE_WIN32

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <SPIRV/GlslangToSpv.h>
#include <iostream>
#include <limits>

//#define GLFW_INCLUDE_VULKAN
#include "glfw_m/include/GLFW_M/glfw3.h"
#include "glfw_m/include/GLFW_M/glfw3native.h"

#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <string>
#include <assert.h>
#include <vector>

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               \
    {                                                                          \
        info.fp##entrypoint =                                                  \
            (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        if (info.fp##entrypoint == NULL) {                                     \
            std::cout << "vkGetDeviceProcAddr failed to find vk" #entrypoint;  \
            exit(-1);                                                          \
        }                                                                      \
    }

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                  \
    {                                                                          \
        info.fp##entrypoint =                                                  \
            (PFN_vk##entrypoint)vkGetDeviceProcAddr(dev, "vk" #entrypoint);    \
        if (info.fp##entrypoint == NULL) {                                     \
            std::cout << "vkGetDeviceProcAddr failed to find vk" #entrypoint;  \
            exit(-1);                                                          \
        }                                                                      \
    }

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

typedef struct {
	VkLayerProperties properties;
	std::vector<VkExtensionProperties> instance_extensions;
	std::vector<VkExtensionProperties> device_extensions;
} layer_properties;

typedef struct _swap_chain_buffers {
	VkImage image;
	VkImageView view;
} swap_chain_buffer;

// Vertex buffer and attributes
struct vertices {
	VkDeviceMemory memory;															// Handle to the device memory for this buffer
	VkBuffer buffer;																// Handle to the Vulkan buffer object that the memory is bound to
	VkDescriptorBufferInfo buffer_info;
	uint32_t dataSize;
	uint32_t dataStride;
	float* vBuffer;
};

struct indices{
	VkDeviceMemory memory;
	VkBuffer buffer;
	uint32_t count;
	uint16_t* indices;
};


struct LHContext {
	std::string name;
	VkInstance instance;
	std::vector<const char*> instance_layer_names;
	std::vector<const char*> instance_extension_names;
	std::vector<layer_properties> instance_layer_properties;
	std::vector<VkExtensionProperties> instance_extension_properties;

	std::vector<const char*> device_extension_names;
	std::vector<VkExtensionProperties> device_extension_properties;
	uint32_t selectedGPU;
	std::vector<VkPhysicalDevice> gpus;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	std::vector<VkQueueFamilyProperties> queue_props;
	VkPhysicalDeviceMemoryProperties memory_properties;
	VkPhysicalDeviceProperties gpu_props;
	VkFormat format;
	struct {
		VkFormat format;
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depth;
	VkColorSpaceKHR colorSpace;

	uint32_t queue_family_count;
	uint32_t graphics_queue_family_index;
	uint32_t present_queue_family_index;

	int width;
	int height;
	GLFWwindow* window;
	VkSurfaceKHR surface;

	struct {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
	} semaphores;

	VkSubmitInfo submitInfo;
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkCommandPool cmd_pool;
	VkSwapchainKHR swapChain;
	uint32_t swapchainImageCount;
	std::vector<swap_chain_buffer> buffers;
	std::vector<VkImage> images;
	std::vector<VkFence> waitFences;
	VkRenderPass render_pass;
	VkPipelineCache pipelineCache;
	std::vector<VkFramebuffer> frameBuffers;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	uint32_t currentBuffer = 0;
	VkQueue queue;
	VkQueue present_queue;
	//---------------------------------> Optional
	std::vector<VkCommandBuffer> cmdBuffer;
};

VkResult init_global_extension_propertiesT(layer_properties& layer_props);
VkResult globalLayerProperties(struct LHContext& context);
void init_device_extension_names(struct LHContext& context);
VkResult createInstance(struct LHContext &context ,std::string appName = "Sample App", std::string engineName = "Sample Engine", bool enableValidation = false);
VkResult createDeviceInfo(struct LHContext& context, bool selectGPU = false);
void createWindowContext(struct LHContext &context,int w = 512, int h = 512);
VkResult createSwapChainExtention(struct LHContext& context);
VkResult createDevice(struct LHContext& context);
VkResult createSynchObject(struct LHContext& context);
VkResult createCommandPool(struct LHContext& context);
VkResult createSwapChain(struct LHContext& context);
VkResult createCommandBuffer(struct LHContext& context);
VkResult createSynchPrimitive(struct LHContext& context);
VkResult createDepthBuffers(struct LHContext& context);
VkResult createRenderPass(struct LHContext& context, bool includeDepth = true);
void createPipeLineCache(struct LHContext& context);
VkResult createFrameBuffer(struct LHContext& context, bool includeDepth = true);
void createDeviceQueue(struct LHContext& context);
//----------------------------> Optional Functions

VkResult mapVerticiesToGPU(struct LHContext& context, struct vertices& v, struct indices& i, bool useStaging);
//----------------------------> Helper Function code
std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
bool memory_type_from_properties(struct LHContext& context, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);

//--------------IGNORE FROM HERE---------------------------------------------------------------------->

//
//void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkCommandBuffer cmd) {
//	/* DEPENDS on cmd and queue initialized */
//
//	assert(cmd != VK_NULL_HANDLE);
//	assert(graphics_queue != VK_NULL_HANDLE);
//
//	VkImageMemoryBarrier image_memory_barrier = {};
//	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//	image_memory_barrier.pNext = NULL;
//	image_memory_barrier.srcAccessMask = 0;
//	image_memory_barrier.dstAccessMask = 0;
//	image_memory_barrier.oldLayout = old_image_layout;
//	image_memory_barrier.newLayout = new_image_layout;
//	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//	image_memory_barrier.image = image;
//	image_memory_barrier.subresourceRange.aspectMask = aspectMask;
//	image_memory_barrier.subresourceRange.baseMipLevel = 0;
//	image_memory_barrier.subresourceRange.levelCount = 1;
//	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
//	image_memory_barrier.subresourceRange.layerCount = 1;
//
//	if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
//		image_memory_barrier.srcAccessMask =
//			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	}
//
//	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
//		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	}
//
//	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
//		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//	}
//
//	if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
//		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	}
//
//	if (old_image_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
//		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
//	}
//
//	if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
//		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//	}
//
//	if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
//		image_memory_barrier.dstAccessMask =
//			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//	}
//
//	if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
//		image_memory_barrier.dstAccessMask =
//			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//	}
//
//	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//
//	vkCmdPipelineBarrier(cmd, src_stages, dest_stages, 0, 0, NULL, 0, NULL,
//		1, &image_memory_barrier);
//}
//
//void init_glslang() {
//	glslang::InitializeProcess();
//
//}
//
//void finalize_glslang() {
//	glslang::FinalizeProcess();
//
//}
//
////
//// Compile a given string containing GLSL into SPV for use by VK
//// Return value of false means an error was encountered.
////
//bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char* pshader,
//	std::vector<unsigned int>& spirv) {
//	EShLanguage stage = FindLanguage(shader_type);
//	glslang::TShader shader(stage);
//	glslang::TProgram program;
//	const char* shaderStrings[1];
//	TBuiltInResource Resources;
//	init_resources(Resources);
//
//	// Enable SPIR-V and Vulkan rules when parsing GLSL
//	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
//
//	shaderStrings[0] = pshader;
//	shader.setStrings(shaderStrings, 1);
//
//	if (!shader.parse(&Resources, 100, false, messages)) {
//		puts(shader.getInfoLog());
//		puts(shader.getInfoDebugLog());
//		return false; // something didn't work
//	}
//
//	program.addShader(&shader);
//
//	//
//	// Program-level processing...
//	//
//
//	if (!program.link(messages)) {
//		puts(shader.getInfoLog());
//		puts(shader.getInfoDebugLog());
//		fflush(stdout);
//		return false;
//	}
//
//	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);
//
//	return true;
//}
//
//EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type) {
//	switch (shader_type) {
//	case VK_SHADER_STAGE_VERTEX_BIT:
//		return EShLangVertex;
//
//	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
//		return EShLangTessControl;
//
//	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
//		return EShLangTessEvaluation;
//
//	case VK_SHADER_STAGE_GEOMETRY_BIT:
//		return EShLangGeometry;
//
//	case VK_SHADER_STAGE_FRAGMENT_BIT:
//		return EShLangFragment;
//
//	case VK_SHADER_STAGE_COMPUTE_BIT:
//		return EShLangCompute;
//
//	default:
//		return EShLangVertex;
//	}
//}
//
//void init_resources(TBuiltInResource& Resources) {
//	Resources.maxLights = 32;
//	Resources.maxClipPlanes = 6;
//	Resources.maxTextureUnits = 32;
//	Resources.maxTextureCoords = 32;
//	Resources.maxVertexAttribs = 64;
//	Resources.maxVertexUniformComponents = 4096;
//	Resources.maxVaryingFloats = 64;
//	Resources.maxVertexTextureImageUnits = 32;
//	Resources.maxCombinedTextureImageUnits = 80;
//	Resources.maxTextureImageUnits = 32;
//	Resources.maxFragmentUniformComponents = 4096;
//	Resources.maxDrawBuffers = 32;
//	Resources.maxVertexUniformVectors = 128;
//	Resources.maxVaryingVectors = 8;
//	Resources.maxFragmentUniformVectors = 16;
//	Resources.maxVertexOutputVectors = 16;
//	Resources.maxFragmentInputVectors = 15;
//	Resources.minProgramTexelOffset = -8;
//	Resources.maxProgramTexelOffset = 7;
//	Resources.maxClipDistances = 8;
//	Resources.maxComputeWorkGroupCountX = 65535;
//	Resources.maxComputeWorkGroupCountY = 65535;
//	Resources.maxComputeWorkGroupCountZ = 65535;
//	Resources.maxComputeWorkGroupSizeX = 1024;
//	Resources.maxComputeWorkGroupSizeY = 1024;
//	Resources.maxComputeWorkGroupSizeZ = 64;
//	Resources.maxComputeUniformComponents = 1024;
//	Resources.maxComputeTextureImageUnits = 16;
//	Resources.maxComputeImageUniforms = 8;
//	Resources.maxComputeAtomicCounters = 8;
//	Resources.maxComputeAtomicCounterBuffers = 1;
//	Resources.maxVaryingComponents = 60;
//	Resources.maxVertexOutputComponents = 64;
//	Resources.maxGeometryInputComponents = 64;
//	Resources.maxGeometryOutputComponents = 128;
//	Resources.maxFragmentInputComponents = 128;
//	Resources.maxImageUnits = 8;
//	Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
//	Resources.maxCombinedShaderOutputResources = 8;
//	Resources.maxImageSamples = 0;
//	Resources.maxVertexImageUniforms = 0;
//	Resources.maxTessControlImageUniforms = 0;
//	Resources.maxTessEvaluationImageUniforms = 0;
//	Resources.maxGeometryImageUniforms = 0;
//	Resources.maxFragmentImageUniforms = 8;
//	Resources.maxCombinedImageUniforms = 8;
//	Resources.maxGeometryTextureImageUnits = 16;
//	Resources.maxGeometryOutputVertices = 256;
//	Resources.maxGeometryTotalOutputComponents = 1024;
//	Resources.maxGeometryUniformComponents = 1024;
//	Resources.maxGeometryVaryingComponents = 64;
//	Resources.maxTessControlInputComponents = 128;
//	Resources.maxTessControlOutputComponents = 128;
//	Resources.maxTessControlTextureImageUnits = 16;
//	Resources.maxTessControlUniformComponents = 1024;
//	Resources.maxTessControlTotalOutputComponents = 4096;
//	Resources.maxTessEvaluationInputComponents = 128;
//	Resources.maxTessEvaluationOutputComponents = 128;
//	Resources.maxTessEvaluationTextureImageUnits = 16;
//	Resources.maxTessEvaluationUniformComponents = 1024;
//	Resources.maxTessPatchComponents = 120;
//	Resources.maxPatchVertices = 32;
//	Resources.maxTessGenLevel = 64;
//	Resources.maxViewports = 16;
//	Resources.maxVertexAtomicCounters = 0;
//	Resources.maxTessControlAtomicCounters = 0;
//	Resources.maxTessEvaluationAtomicCounters = 0;
//	Resources.maxGeometryAtomicCounters = 0;
//	Resources.maxFragmentAtomicCounters = 8;
//	Resources.maxCombinedAtomicCounters = 8;
//	Resources.maxAtomicCounterBindings = 1;
//	Resources.maxVertexAtomicCounterBuffers = 0;
//	Resources.maxTessControlAtomicCounterBuffers = 0;
//	Resources.maxTessEvaluationAtomicCounterBuffers = 0;
//	Resources.maxGeometryAtomicCounterBuffers = 0;
//	Resources.maxFragmentAtomicCounterBuffers = 1;
//	Resources.maxCombinedAtomicCounterBuffers = 1;
//	Resources.maxAtomicCounterBufferSize = 16384;
//	Resources.maxTransformFeedbackBuffers = 4;
//	Resources.maxTransformFeedbackInterleavedComponents = 64;
//	Resources.maxCullDistances = 8;
//	Resources.maxCombinedClipAndCullDistances = 8;
//	Resources.maxSamples = 4;
//	Resources.limits.nonInductiveForLoops = 1;
//	Resources.limits.whileLoops = 1;
//	Resources.limits.doWhileLoops = 1;
//	Resources.limits.generalUniformIndexing = 1;
//	Resources.limits.generalAttributeMatrixVectorIndexing = 1;
//	Resources.limits.generalVaryingIndexing = 1;
//	Resources.limits.generalSamplerIndexing = 1;
//	Resources.limits.generalVariableIndexing = 1;
//	Resources.limits.generalConstantMatrixVectorIndexing = 1;
//}
//
//void wait_seconds(int seconds) {
//#ifdef WIN32
//	Sleep(seconds * 1000);
//#else
//	sleep(seconds);
//#endif
//}
//
//timestamp_t get_milliseconds() {
//#ifdef WIN32
//	LARGE_INTEGER frequency;
//	BOOL useQPC = QueryPerformanceFrequency(&frequency);
//	if (useQPC) {
//		LARGE_INTEGER now;
//		QueryPerformanceCounter(&now);
//		return (1000LL * now.QuadPart) / frequency.QuadPart;
//	}
//	else {
//		return GetTickCount();
//	}
//#else
//	struct timeval now;
//	gettimeofday(&now, NULL);
//	return (now.tv_usec / 1000) + (timestamp_t)now.tv_sec;
//#endif
//}
//
#endif // !L_H_VULKAN_H