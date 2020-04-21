#ifndef L_H_VULKAN_H
#define L_H_VULKAN_H

#define _CRT_SECURE_NO_WARNINGS

#ifndef NOMINMAX
#define NOMINMAX /* Don't let Windows define min() or max() */
#endif

//#ifndef VK_USE_PLATFORM_WIN32_KHR
//#define VK_USE_PLATFORM_WIN32_KHR
//#endif

#define GLFW_EXPOSE_NATIVE_WIN32

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <SPIRV/GlslangToSpv.h>
#include <iostream>
#include <fstream>
#include <limits>

//#define GLFW_INCLUDE_VULKAN
//#include "glfw_m/include/GLFW_M/glfw3.h"
//#include "#include "GLFW/glfw3.h"
#include "GLFW/glfw3.h"

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
void setupRenderPass(struct LHContext& context, bool useStagingBuffers = false);
void cleanUpResources(struct LHContext& context);
void createScisscor(struct LHContext& context, VkCommandBuffer& cmd, VkRect2D& sc);
void createViewports(struct LHContext& context, VkCommandBuffer& cmd, VkViewport& vp);
VkResult prepareSynchronizationPrimitives(struct LHContext& context);
//----------------------------> Optional Functions

VkResult mapVerticiesToGPU(struct LHContext& context, const void* vertexInput, uint32_t dataSize,
	VkBuffer& vertexBuffer, VkDeviceMemory& memory);
VkResult mapIndiciesToGPU(struct LHContext& context, const void* indiciesInput, uint32_t dataSize,
	VkBuffer& indexBuffer, VkDeviceMemory& memory);
VkResult bindBufferToMem(struct LHContext& context, VkBufferCreateInfo& bufferInfo, VkFlags flags,
	VkBuffer& inputBuffer, VkDeviceMemory& memory);
void createClearColor(struct LHContext& context, VkClearValue* clear_values);
void createRenderPassCreateInfo(struct LHContext& context, VkRenderPassBeginInfo& rp_begin);
void createAttachmentDescription(struct LHContext& context, VkAttachmentDescription* attachments);
void draw(struct LHContext& context);

void createShaderStage(struct LHContext& context, std::string filename, VkShaderStageFlagBits flag, VkPipelineShaderStageCreateInfo& shaderStage);
//----------------------------> Helper Function code
std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
bool memory_type_from_properties(struct LHContext& context, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);
void init_glslang();
void finalize_glslang();
VkShaderModule loadSPIRVShader(struct LHContext& context, std::string filename);
bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char* pshader,
	std::vector<unsigned int>& spirv);
EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
void init_resources(TBuiltInResource& Resources);
#endif // !L_H_VULKAN_H