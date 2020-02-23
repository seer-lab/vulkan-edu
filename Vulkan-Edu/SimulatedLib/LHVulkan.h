#define _CRT_SECURE_NO_WARNINGS

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <SPIRV/GlslangToSpv.h>
#include <iostream>

#include "glfw_m/include/GLFW_M/glfw3.h"

#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include <string>
#include <assert.h>
#include <vector>


#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

typedef unsigned long long timestamp_t;

typedef struct {
	VkLayerProperties properties;
	std::vector<VkExtensionProperties> extensions;
} layer_properties;

typedef struct uniformVariable {
	VkShaderStageFlags shaderFlags;
	uint32_t binding;
	VkDescriptorType descripterType;
	std::string uniformName;
	uint32_t descriptorCount;
};

typedef struct _swap_chain_buffers {
	VkImage image;
	VkImageView view;
} swap_chain_buffer;

typedef struct {
	VkFormat format;
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
} depth;

typedef struct uniform_data {
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDescriptorBufferInfo buffer_info;
} Uniform_Data;

struct {
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDescriptorBufferInfo buffer_info;
} vertex_buffer;

float* vBuffer;
VkBuffer iBuffer;

VkResult init_global_extension_propertiesT(layer_properties& layer_props);
VkResult globalLayerProperties();
void deviceExtentionName();
void instanceExtentionName();
VkResult createInstance(std::string appName = "Sample App", std::string engineName = "Sample Engine");
VkResult createDeviceInfo();
VkResult createSwapChainExtention();
VkResult createDevice();
VkResult createRenderPass(bool include_depth, bool clear = true, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
VkResult createCommandPool();
VkResult createCommandBuffer(VkCommandBuffer& newCmd);
VkResult execute_begin_command_buffer(VkCommandBuffer cmd);
VkResult execute_end_command_buffer(VkCommandBuffer cmd);
void createDeviceQueue();
void createSwapChain(VkImageUsageFlags usageFlags);
void createDepthBuffer(VkCommandBuffer cmd);
//template <class T>
//VkResult setUniformValue(T uniformVal,
//	Uniform_Data& uni_data,
//	uint32_t pBinding,
//	VkShaderStageFlags sFlags,
//	uint32_t descriptorCount = 1);
VkResult createDescripterLayout();
VkResult createRenderPass(bool include_depth, bool clear, VkImageLayout finalLayout);
void init_shaders(struct AppState& state, const char* vertShaderText, const char* fragShaderText);
VkResult createFrameBuffer(bool include_depth);
void createDescriptorPool(bool use_texture);
void createDescriptorSet(bool use_texture);
void createPipeLineCache();
void init_pipeline(VkBool32 include_depth, VkBool32 include_vi);
void renderObject(struct AppState& state);
void init_viewports(VkCommandBuffer cmd);
void init_scissors(VkCommandBuffer cmd);

//Make a utililty file;
void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkCommandBuffer cmd);
bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);
void init_glslang();
void finalize_glslang();
bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char* pshader, std::vector<unsigned int>& spirv);
EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
void init_resources(TBuiltInResource& Resources);
timestamp_t get_milliseconds();
void wait_seconds(int seconds);
#define LAYER_COUNT 0
#define LAYER_NAME NULL
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT
#define NUM_VIEWPORTS 1
#define NUM_SCISSORS NUM_VIEWPORTS
#define FENCE_TIMEOUT 100000000

//TODO Change this such that it can handle Multiplatform code
#define EXTENSION_COUNT 2
#define EXTENSION_NAMES VK_KHR_SURFACE_EXTENSION_NAME | VK_KHR_WIN32_SURFACE_EXTENSION_NAME

//Global Variable
static VkInstance inst;
std::string name;
uint32_t queue_family_count;
uint32_t graphics_queue_family_index;
uint32_t present_queue_family_index;
uint32_t swapchainImageCount;
uint32_t current_buffer;
VkPhysicalDeviceMemoryProperties memory_properties;
VkPhysicalDeviceProperties gpu_props;
VkCommandPool cmdpool;
VkDevice device;
VkSurfaceKHR surface;
VkFormat format;
VkQueue graphics_queue;
VkQueue present_queue;
VkSwapchainKHR swap_chain;
int num_descriptor_sets;
VkPipelineLayout pipeline_layout;
VkRenderPass render_pass;
VkPipelineShaderStageCreateInfo shaderStages[2];
VkFramebuffer *framebuffers;
VkDescriptorPool desc_pool;
VkPipelineCache pipelineCaches;
VkVertexInputBindingDescription vi_binding;
VkVertexInputAttributeDescription vi_attribs[2];
VkPipeline pipeLines;
VkViewport viewport;
VkRect2D scissor;

std::vector<VkDescriptorSetLayout> desc_layout;
std::vector<const char*> device_extension_names;
std::vector<const char*> instance_layer_names;
std::vector<const char*> instance_extension_names;
std::vector<VkPhysicalDevice> gpus;
std::vector<VkQueueFamilyProperties> queue_props;
std::vector<layer_properties> instance_layer_properties;
std::vector<swap_chain_buffer> scBuffer;
std::vector<VkDescriptorSet> desc_set;

std::vector<uniformVariable> uniformVar;
std::vector<uniform_data> uniform_d;

//TODO MAKE MULTIPLATFORM (Surface Creation)
static HINSTANCE connection;
static HWND window;
long info;
int width;
int height;

depth depths;

struct AppState {
	VkCommandBuffer cmd;
	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkDescriptorBufferInfo buffer_info;
	} uniform_data;
	int num_descriptor_sets;
	std::vector<VkDescriptorSetLayout> desc_layout;
	VkPipelineLayout pipeline_layout;
	VkPipelineShaderStageCreateInfo shaderStages[2];
	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkDescriptorBufferInfo buffer_info;
	} vertex_buffer;
	VkVertexInputBindingDescription vi_binding;
	VkVertexInputAttributeDescription vi_attribs[2];
	VkDescriptorPool desc_pool;
	std::vector<VkDescriptorSet> desc_set;
	VkPipeline pipeline;
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;
	glm::mat4 Clip;
	glm::mat4 MVP;
	float* vBuffer;
	VkBuffer iBuffer;
} state;

VkResult init_global_extension_propertiesT(layer_properties& layer_props) {
	VkExtensionProperties* instance_extensions;
	uint32_t instance_extension_count;
	VkResult res;
	char* layer_name = NULL;

	layer_name = layer_props.properties.layerName;

	do {
		res = vkEnumerateInstanceExtensionProperties(
			layer_name, &instance_extension_count, NULL);
		if (res)
			return res;

		if (instance_extension_count == 0) {
			return VK_SUCCESS;
		}

		layer_props.extensions.resize(instance_extension_count);
		instance_extensions = layer_props.extensions.data();
		res = vkEnumerateInstanceExtensionProperties(
			layer_name, &instance_extension_count, instance_extensions);
	} while (res == VK_INCOMPLETE);

	return res;
}
//
VkResult globalLayerProperties() {
	uint32_t instance_layer_count;
	VkLayerProperties* vk_props = NULL;
	VkResult res;
	do {
		res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
		if (res)
			return res;

		if (instance_layer_count == 0) {
			return VK_SUCCESS;
		}

		vk_props = (VkLayerProperties*)realloc(
			vk_props, instance_layer_count * sizeof(VkLayerProperties));

		res =
			vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
	} while (res == VK_INCOMPLETE);

	/*
	* Now gather the extension list for each instance layer.
	*/
	for (uint32_t i = 0; i < instance_layer_count; i++) {
		layer_properties layer_props;
		layer_props.properties = vk_props[i];
		res = init_global_extension_propertiesT(layer_props);
		if (res)
			return res;
		instance_layer_properties.push_back(layer_props);
	}
	free(vk_props);

	return res;
}

void instanceExtentionName() {
	instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(_WIN32)
	instance_extension_names.push_back(
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
	instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
}

void deviceExtentionName() {
	device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

VkResult createInstance(std::string appName,std::string engineName) {
	globalLayerProperties();
	instanceExtentionName();
	deviceExtentionName();

	name = appName;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = appName.c_str();
	app_info.applicationVersion = 1;
	app_info.pEngineName = engineName.c_str();
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledLayerCount = instance_layer_names.size();
	inst_info.ppEnabledLayerNames = instance_layer_names.size()
		? instance_layer_names.data()
		: NULL;
	inst_info.enabledExtensionCount = instance_extension_names.size();
	inst_info.ppEnabledExtensionNames = instance_extension_names.data();

	VkResult res = vkCreateInstance(&inst_info, NULL, &inst);
	assert(res == VK_SUCCESS);

	for (int i = 0; i < instance_extension_names.size(); i++) {
		std::cout << instance_extension_names[i] << std::endl;
	}

	std::cout << "\n";

	return res;
}

//TODO Make this function select anyother GPU for the user if the user chooses to do so
VkResult createDeviceInfo() {

	uint32_t gpu_count = 1;
	uint32_t const U_ASSERT_ONLY req_count = gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(inst, &gpu_count, NULL);
	assert(gpu_count);

	gpus.resize(gpu_count);

	res = vkEnumeratePhysicalDevices(inst, &gpu_count, gpus.data());
	assert(!res && gpu_count >= req_count);

	vkGetPhysicalDeviceQueueFamilyProperties(gpus[0],
		&queue_family_count, NULL);
	assert(queue_family_count >= 1);

	queue_props.resize(queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(
		gpus[0], &queue_family_count, queue_props.data());
	assert(queue_family_count >= 1);

	/* This is as good a place as any to do this */
	vkGetPhysicalDeviceMemoryProperties(gpus[0], &memory_properties);
	vkGetPhysicalDeviceProperties(gpus[0], &gpu_props);

	return res;
}

void init_connection() {
#if !defined(_WIN32)
	const xcb_setup_t* setup;
	xcb_screen_iterator_t iter;
	int scr;

	connection = xcb_connect(NULL, &scr);
	if (connection == NULL || xcb_connection_has_error(connection)) {
		std::cout << "Cannot find a compatible Vulkan ICD.\n";
		exit(-1);
	}

	setup = xcb_get_setup(connection);
	iter = xcb_setup_roots_iterator(setup);
	while (scr-- > 0)
		xcb_screen_next(&iter);

	screen = iter.data;
#endif //__Android__
}
#ifdef _WIN32

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	struct sample_info* info = reinterpret_cast<struct sample_info*>(
		GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (uMsg) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		return 0;
	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void createWindowContext(int w, int h) {
	width = w;
	height = h;
	WNDCLASSEX win_class;
	assert(width > 0);
	assert(height > 0);

	connection = GetModuleHandle(NULL);

	// Initialize the window class structure:
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WndProc;
	win_class.cbClsExtra = 0;
	win_class.cbWndExtra = 0;
	win_class.hInstance = connection; // hInstance
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	wchar_t wname[256];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name.c_str(), -1, wname, 256);
	win_class.lpszClassName = (LPCWSTR)wname;
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	// Register window class:
	if (!RegisterClassEx(&win_class)) {
		// It didn't work, so try to give a useful error:
		printf("Unexpected error trying to start the application!\n");
		fflush(stdout);
		exit(1);
	}
	// Create window with the registered class:
	RECT wr = { 0, 0, width, height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	window = CreateWindowEx(0,
		(LPCWSTR)wname,            // class name
		(LPCWSTR)wname,            // app name
		WS_OVERLAPPEDWINDOW | // window style
		WS_VISIBLE | WS_SYSMENU,
		100, 100,           // x/y coords
		wr.right - wr.left, // width
		wr.bottom - wr.top, // height
		NULL,               // handle to parent
		NULL,               // handle to menu
		connection,    // hInstance
		NULL);              // no extra parameters
	if (!window) {
		// It didn't work, so try to give a useful error:
		printf("Cannot create a window in which to draw!\n");
		fflush(stdout);
		exit(1);
	}
	SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)&info);
}

void destroy_window() {
	vkDestroySurfaceKHR(inst, surface, NULL);
	DestroyWindow(window);
}
#else
void createWindowContext(int w, int h) {
	width = w;
	height = h;

	assert(width > 0);
	assert(height > 0);

	uint32_t value_mask, value_list[32];

	window = xcb_generate_id(connection);

	value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value_list[0] = screen->black_pixel;
	value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

	xcb_create_window(connection, XCB_COPY_FROM_PARENT, window,
		screen->root, 0, 0, width, height, 0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
		value_mask, value_list);

	/* Magic code that will send notification when window is destroyed */
	xcb_intern_atom_cookie_t cookie =
		xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t* reply =
		xcb_intern_atom_reply(connection, cookie, 0);

	xcb_intern_atom_cookie_t cookie2 =
		xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
	atom_wm_delete_window =
		xcb_intern_atom_reply(connection, cookie2, 0);

	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window,
		(*reply).atom, 4, 32, 1,
		&(*atom_wm_delete_window).atom);
	free(reply);

	xcb_map_window(connection, window);

	// Force the x/y coordinates to 100,100 results are identical in consecutive
	// runs
	const uint32_t coords[] = { 100, 100 };
	xcb_configure_window(connection, window,
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
	xcb_flush(connection);

	xcb_generic_event_t* e;
	while ((e = xcb_wait_for_event(connection))) {
		if ((e->response_type & ~0x80) == XCB_EXPOSE)
			break;
	}
}

void destroy_window() {
	vkDestroySurfaceKHR(inst, surface, NULL);
	xcb_destroy_window(connection, window);
	xcb_disconnect(connection);
}
#endif

void createWindowContext_2(int w, int h) {
	if (!glfwInit()) {
		std::cout << "Error INIT FAILED\n";
		exit(-1);
	}
	if (glfwVulkanSupported()) {
		std::cout << "Vulkan is supported\n";
	}
	else {
		std::cout << "Error Vulkan is not supported\n";
	}

	uint32_t ext_count;
	const char** extName = glfwGetRequiredInstanceExtensions(&ext_count);

	for (int i = 0; i < ext_count; i++) {
		std::cout << extName[i] << std::endl;
	}

	GLFWwindow* window = glfwCreateWindow(w, h, name.c_str() , NULL, NULL);
}


VkResult createSwapChainExtention() {
	VkResult U_ASSERT_ONLY res;

	// Construct the surface description:
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.hinstance = connection;
	createInfo.hwnd = window;
	res = vkCreateWin32SurfaceKHR(inst, &createInfo,NULL, &surface);
#else  // !__ANDROID__ && !_WIN32
	VkXcbSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.connection = connection;
	createInfo.window = window;
	res = vkCreateXcbSurfaceKHR(inst, &createInfo,
		NULL, &surface);
#endif // __ANDROID__  && _WIN32
	assert(res == VK_SUCCESS);

	// Iterate over each queue to learn whether it supports presenting:
	VkBool32* pSupportsPresent =
		(VkBool32*)malloc(queue_family_count * sizeof(VkBool32));
	for (uint32_t i = 0; i < queue_family_count; i++) {
		vkGetPhysicalDeviceSurfaceSupportKHR(gpus[0], i, surface,
			&pSupportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	graphics_queue_family_index = UINT32_MAX;
	present_queue_family_index = UINT32_MAX;
	for (uint32_t i = 0; i < queue_family_count; ++i) {
		if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (graphics_queue_family_index == UINT32_MAX)
				graphics_queue_family_index = i;

			if (pSupportsPresent[i] == VK_TRUE) {
				graphics_queue_family_index = i;
				present_queue_family_index = i;
				break;
			}
		}
	}

	if (present_queue_family_index == UINT32_MAX) {
		// If didn't find a queue that supports both graphics and present, then
		// find a separate present queue.
		for (size_t i = 0; i < queue_family_count; ++i)
			if (pSupportsPresent[i] == VK_TRUE) {
				present_queue_family_index = i;
				break;
			}
	}
	free(pSupportsPresent);

	// Generate error if could not find queues that support graphics
	// and present
	if (graphics_queue_family_index == UINT32_MAX ||
		present_queue_family_index == UINT32_MAX) {
		std::cout << "Could not find a queues for both graphics and present";
		exit(-1);
	}

	// Get the list of VkFormats that are supported:
	uint32_t formatCount;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[0], surface,
		&formatCount, NULL);
	assert(res == VK_SUCCESS);
	VkSurfaceFormatKHR* surfFormats =
		(VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(gpus[0], surface,
		&formatCount, surfFormats);
	assert(res == VK_SUCCESS);
	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
		format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else {
		assert(formatCount >= 1);
		format = surfFormats[0].format;
	}
	free(surfFormats);
	return res;
}

VkResult createDevice() {
	VkResult res;
	VkDeviceQueueCreateInfo queue_info = {};

	float queue_priorities[1] = { 0.0 };
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = queue_priorities;
	queue_info.queueFamilyIndex = graphics_queue_family_index;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = NULL;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = device_extension_names.size();
	device_info.ppEnabledExtensionNames =
	device_info.enabledExtensionCount ? device_extension_names.data()
		: NULL;
	device_info.pEnabledFeatures = NULL;

	res = vkCreateDevice(gpus[0], &device_info, NULL, &device);
	assert(res == VK_SUCCESS);

	return res;
}

VkResult createCommandPool() {
	VkResult U_ASSERT_ONLY res;

	VkCommandPoolCreateInfo cmd_pool_info = {};
	cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.pNext = NULL;
	cmd_pool_info.queueFamilyIndex = graphics_queue_family_index;
	cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	res = vkCreateCommandPool(device, &cmd_pool_info, NULL, &cmdpool);
	assert(res == VK_SUCCESS);

	return res;
}

VkResult createCommandBuffer(VkCommandBuffer& newCmd) {
	/* DEPENDS on init_swapchain_extension() and init_command_pool() */
	VkResult U_ASSERT_ONLY res;

	VkCommandBufferAllocateInfo cmd = {};
	cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd.pNext = NULL;
	cmd.commandPool = cmdpool;
	cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd.commandBufferCount = 1;

	res = vkAllocateCommandBuffers(device, &cmd, &newCmd);
	assert(res == VK_SUCCESS);
	return res;
}

VkResult execute_begin_command_buffer(VkCommandBuffer cmd) {
	/* DEPENDS on init_command_buffer() */
	VkResult U_ASSERT_ONLY res;

	VkCommandBufferBeginInfo cmd_buf_info = {};
	cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_info.pNext = NULL;
	cmd_buf_info.flags = 0;
	cmd_buf_info.pInheritanceInfo = NULL;

	res = vkBeginCommandBuffer(cmd, &cmd_buf_info);
	assert(res == VK_SUCCESS);
	return res;
}

VkResult execute_end_command_buffer(VkCommandBuffer cmd) {
	VkResult U_ASSERT_ONLY res;

	res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);
	return res;
}

void createDeviceQueue() {
	/* DEPENDS on init_swapchain_extension() */

	vkGetDeviceQueue(device, graphics_queue_family_index, 0,
		&graphics_queue);
	if (graphics_queue_family_index == present_queue_family_index) {
		present_queue = graphics_queue;
	}
	else {
		vkGetDeviceQueue(device, present_queue_family_index, 0,
			&present_queue);
	}
}

void createSwapChain(VkImageUsageFlags usageFlags) {

	VkResult U_ASSERT_ONLY res;
	VkSurfaceCapabilitiesKHR surfCapabilities;

	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpus[0], surface,&surfCapabilities);
	assert(res == VK_SUCCESS);

	uint32_t presentModeCount;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[0], surface, &presentModeCount, NULL);
	assert(res == VK_SUCCESS);

	VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
	assert(presentModes);
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(gpus[0], surface, &presentModeCount, presentModes);
	assert(res == VK_SUCCESS);

	VkExtent2D swapchainExtent;
	// width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
	if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = width;
		swapchainExtent.height = height;
		if (swapchainExtent.width < surfCapabilities.minImageExtent.width) {
			swapchainExtent.width = surfCapabilities.minImageExtent.width;
		}
		else if (swapchainExtent.width >
			surfCapabilities.maxImageExtent.width) {
			swapchainExtent.width = surfCapabilities.maxImageExtent.width;
		}

		if (swapchainExtent.height < surfCapabilities.minImageExtent.height) {
			swapchainExtent.height = surfCapabilities.minImageExtent.height;
		}
		else if (swapchainExtent.height >
			surfCapabilities.maxImageExtent.height) {
			swapchainExtent.height = surfCapabilities.maxImageExtent.height;
		}
	}
	else {
		// If the surface size is defined, the swap chain size must match
		swapchainExtent = surfCapabilities.currentExtent;
	}

	// If mailbox mode is available, use it, as is the lowest-latency non-
	// tearing mode.  If not, try IMMEDIATE which will usually be available,
	// and is fastest (though it tears).  If not, fall back to FIFO which is
	// always available.
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < presentModeCount; i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
			(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
			swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	// Determine the number of VkImage's to use in the swap chain.
	// We need to acquire only 1 presentable image at at time.
	// Asking for minImageCount images ensures that we can acquire
	// 1 presentable image as long as we present it before attempting
	// to acquire another.
	uint32_t desiredNumberOfSwapChainImages = surfCapabilities.minImageCount;

	VkSurfaceTransformFlagBitsKHR preTransform;
	if (surfCapabilities.supportedTransforms &
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else {
		preTransform = surfCapabilities.currentTransform;
	}

	VkSwapchainCreateInfoKHR swapchain_ci = {};
	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.pNext = NULL;
	swapchain_ci.surface = surface;
	swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
	swapchain_ci.imageFormat = format;
	swapchain_ci.imageExtent.width = swapchainExtent.width;
	swapchain_ci.imageExtent.height = swapchainExtent.height;
	swapchain_ci.preTransform = preTransform;
	swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.presentMode = swapchainPresentMode;
	swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
	swapchain_ci.clipped = true;
	swapchain_ci.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchain_ci.imageUsage = usageFlags;
	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = 0;
	swapchain_ci.pQueueFamilyIndices = NULL;
	uint32_t queueFamilyIndices[2] = {
		(uint32_t)graphics_queue_family_index,
		(uint32_t)present_queue_family_index };
	if (graphics_queue_family_index != present_queue_family_index) {

		swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_ci.queueFamilyIndexCount = 2;
		swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
	}

	res = vkCreateSwapchainKHR(device, &swapchain_ci, NULL,&swap_chain);
	assert(res == VK_SUCCESS);

	res = vkGetSwapchainImagesKHR(device, swap_chain,&swapchainImageCount, NULL);
	assert(res == VK_SUCCESS);

	VkImage* swapchainImages =
		(VkImage*)malloc(swapchainImageCount * sizeof(VkImage));
	assert(swapchainImages);

	res = vkGetSwapchainImagesKHR(device, swap_chain, &swapchainImageCount, swapchainImages);
	assert(res == VK_SUCCESS);

	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		swap_chain_buffer sc_buffer;

		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.format = format;
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.flags = 0;

		sc_buffer.image = swapchainImages[i];

		color_image_view.image = sc_buffer.image;

		res = vkCreateImageView(device, &color_image_view, NULL,
			&sc_buffer.view);
		scBuffer.push_back(sc_buffer);
		assert(res == VK_SUCCESS);
	}
	free(swapchainImages);
	current_buffer = 0;

	if (NULL != presentModes) {
		free(presentModes);
	}
}

void createDepthBuffer(VkCommandBuffer cmd) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;
	VkImageCreateInfo image_info = {};

	/* allow custom depth formats */
	if (depths.format == VK_FORMAT_UNDEFINED)
		depths.format = VK_FORMAT_D16_UNORM;

	const VkFormat depth_format = depths.format;

	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(gpus[0], depth_format, &props);
	if (props.linearTilingFeatures &
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		image_info.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (props.optimalTilingFeatures &
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	}
	else {
		/* Try other depth formats? */
		std::cout << "depth_format " << depth_format << " Unsupported.\n";
		exit(-1);
	}

	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = NULL;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = depth_format;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = NUM_SAMPLES;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices = NULL;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.flags = 0;

	VkMemoryAllocateInfo mem_alloc = {};
	mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_alloc.pNext = NULL;
	mem_alloc.allocationSize = 0;
	mem_alloc.memoryTypeIndex = 0;

	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext = NULL;
	view_info.image = VK_NULL_HANDLE;
	view_info.format = depth_format;
	view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.flags = 0;

	if (depth_format == VK_FORMAT_D16_UNORM_S8_UINT ||
		depth_format == VK_FORMAT_D24_UNORM_S8_UINT ||
		depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
		view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkMemoryRequirements mem_reqs;

	/* Create image */
	res = vkCreateImage(device, &image_info, NULL, &depths.image);
	assert(res == VK_SUCCESS);

	vkGetImageMemoryRequirements(device, depths.image, &mem_reqs);

	mem_alloc.allocationSize = mem_reqs.size;
	/* Use the memory properties to determine the type of memory required */

	pass = false;

	VkFlags requirement_mask = 0;

	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
		if ((mem_reqs.memoryTypeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((memory_properties.memoryTypes[i].propertyFlags & requirement_mask) == requirement_mask) {
				mem_alloc.memoryTypeIndex = i;
				pass = true;
				break;
			}
		}
		mem_reqs.memoryTypeBits >>= 1;
	}

	//pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits,
	//	0, /* No requirements */
	//	&mem_alloc.memoryTypeIndex);
	assert(pass);

	/* Allocate memory */
	res = vkAllocateMemory(device, &mem_alloc, NULL, &depths.mem);
	assert(res == VK_SUCCESS);

	/* Bind memory */
	res = vkBindImageMemory(device, depths.image, depths.mem, 0);
	assert(res == VK_SUCCESS);

	/* Set the image layout to depth stencil optimal */
	set_image_layout(depths.image,
		view_info.subresourceRange.aspectMask,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		cmd);


	/* Create image view */
	view_info.image = depths.image;
	res = vkCreateImageView(device, &view_info, NULL, &depths.view);
	assert(res == VK_SUCCESS);
}

//template <class T>
//VkResult setUniformValue(T uniformVal, 
//						Uniform_Data& uni_data, 
//						uint32_t pBinding, 
//						VkShaderStageFlags sFlags,
//						uint32_t descriptorCount) {
//	VkResult U_ASSERT_ONLY res = VK_SUCCESS;
//	bool U_ASSERT_ONLY pass;
//
//	VkBufferCreateInfo buf_info = {};
//	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	buf_info.pNext = NULL;
//	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
//	buf_info.size = sizeof(uniformVal);
//	buf_info.queueFamilyIndexCount = 0;
//	buf_info.pQueueFamilyIndices = NULL;
//	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//	buf_info.flags = 0;
//
//	uniform_data ud;
//	uniform_d.push_back(ud);
//	int size = uniform_d.size() - 1;
//	res = vkCreateBuffer(device, &buf_info, NULL, &uniform_d[size].buf);
//	assert(res == VK_SUCCESS);
//
//	VkMemoryRequirements mem_reqs;
//	vkGetBufferMemoryRequirements(device, uniform_d[size].buf,&mem_reqs);
//
//	VkMemoryAllocateInfo alloc_info = {};
//	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	alloc_info.pNext = NULL;
//	alloc_info.memoryTypeIndex = 0;
//
//	alloc_info.allocationSize = mem_reqs.size;
//	pass = memory_type_from_properties(memory_properties, mem_reqs.memoryTypeBits,
//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//		&alloc_info.memoryTypeIndex);
//	assert(pass && "No mappable, coherent memory");
//	
//	res = vkAllocateMemory(device, &alloc_info, NULL,&(uniform_d[size].mem));
//	assert(res == VK_SUCCESS);
//	
//	uint8_t* pData;
//	res = vkMapMemory(device, uniform_d[size].mem, 0, mem_reqs.size, 0,(void**)&pData);
//	assert(res == VK_SUCCESS);
//	
//	memcpy(pData, &uniformVal, sizeof(uniformVal));
//	
//	vkUnmapMemory(device, uniform_d[size].mem);
//	
//	res = vkBindBufferMemory(device, uniform_d[size].buf, uniform_d[size].mem, 0);
//	assert(res == VK_SUCCESS);
//	
//	uniformVariable uvar;
//	uvar.binding = pBinding;
//	uvar.shaderFlags = sFlags;
//	uvar.descripterType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//	uvar.descriptorCount = descriptorCount;
//	uniformVar.push_back(uvar);
//	
//	return res;
//}
//
//
//VkResult createDescripterLayout() {
//	VkResult U_ASSERT_ONLY res;
//	const int size = uniformVar.size();
//	VkDescriptorSetLayoutBinding *layout_bindings = new VkDescriptorSetLayoutBinding[size];
//
//	for (int i = 0; i < size; i++) {
//		layout_bindings[i].binding = uniformVar[i].binding;
//		layout_bindings[i].descriptorType = uniformVar[i].descripterType;
//		layout_bindings[i].descriptorCount = uniformVar[i].descriptorCount;
//		layout_bindings[i].stageFlags = uniformVar[i].shaderFlags;
//		layout_bindings[i].pImmutableSamplers = NULL;
//	}
//
//	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
//	descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//	descriptor_layout.pNext = NULL;
//	descriptor_layout.bindingCount = size;
//	descriptor_layout.pBindings = layout_bindings;
//	num_descriptor_sets = 1;
//	desc_layout.resize(num_descriptor_sets);
//	res = vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, desc_layout.data());
//	assert(res == VK_SUCCESS);
//	/* Now use the descriptor layout to create a pipeline layout */
//	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
//	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//	pPipelineLayoutCreateInfo.pNext = NULL;
//	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
//	pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
//	pPipelineLayoutCreateInfo.setLayoutCount = 1;
//	pPipelineLayoutCreateInfo.pSetLayouts = desc_layout.data();
//	res = vkCreatePipelineLayout(device,&pPipelineLayoutCreateInfo, NULL,&pipeline_layout);
//	assert(res == VK_SUCCESS);
//
//	return res;
//}
//
VkResult createRenderPass(bool include_depth, bool clear,  VkImageLayout finalLayout) {
	/* DEPENDS on init_swap_chain() and init_depth_buffer() */

	VkResult U_ASSERT_ONLY res;
	/* Need attachments for render target and depth buffer */
	VkAttachmentDescription attachments[2];
	attachments[0].format = format;
	attachments[0].samples = NUM_SAMPLES;
	attachments[0].loadOp =
		clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = finalLayout;
	attachments[0].flags = 0;

	if (include_depth) {
		attachments[1].format = depths.format;
		attachments[1].samples = NUM_SAMPLES;
		attachments[1].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR
			: VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].initialLayout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].flags = 0;
	}

	VkAttachmentReference color_reference = {};
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference = {};
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = include_depth ? &depth_reference : NULL;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo rp_info = {};
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.pNext = NULL;
	rp_info.attachmentCount = include_depth ? 2 : 1;
	rp_info.pAttachments = attachments;
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;
	rp_info.dependencyCount = 0;
	rp_info.pDependencies = NULL;

	res = vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
	assert(res == VK_SUCCESS);
	return res;
}


void init_shaders(struct AppState& state, const char* vertShaderText, const char* fragShaderText) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY retVal;

	// If no shaders were submitted, just return
	if (!(vertShaderText || fragShaderText))
		return;

	init_glslang();
	VkShaderModuleCreateInfo moduleCreateInfo;

	if (vertShaderText) {
		std::vector<unsigned int> vtx_spv;
		state.shaderStages[0].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		state.shaderStages[0].pNext = NULL;
		state.shaderStages[0].pSpecializationInfo = NULL;
		state.shaderStages[0].flags = 0;
		state.shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		state.shaderStages[0].pName = "main";

		retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertShaderText, vtx_spv);
		assert(retVal);

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.flags = 0;
		moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
		moduleCreateInfo.pCode = vtx_spv.data();
		res = vkCreateShaderModule(device, &moduleCreateInfo, NULL,
			&state.shaderStages[0].module);
		assert(res == VK_SUCCESS);
	}

	if (fragShaderText) {
		std::vector<unsigned int> frag_spv;
		state.shaderStages[1].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		state.shaderStages[1].pNext = NULL;
		state.shaderStages[1].pSpecializationInfo = NULL;
		state.shaderStages[1].flags = 0;
		state.shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		state.shaderStages[1].pName = "main";

		retVal =
			GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderText, frag_spv);
		assert(retVal);

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.flags = 0;
		moduleCreateInfo.codeSize = frag_spv.size() * sizeof(unsigned int);
		moduleCreateInfo.pCode = frag_spv.data();
		res = vkCreateShaderModule(device, &moduleCreateInfo, NULL,
			&state.shaderStages[1].module);
		assert(res == VK_SUCCESS);
	}

	finalize_glslang();
}

VkResult createFrameBuffer( bool include_depth) {
	/* DEPENDS on init_depth_buffer(), init_renderpass() and
	* init_swapchain_extension() */

	VkResult U_ASSERT_ONLY res;
	VkImageView attachments[2];
	attachments[1] = depths.view;

	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = NULL;
	fb_info.renderPass = render_pass;
	fb_info.attachmentCount = include_depth ? 2 : 1;
	fb_info.pAttachments = attachments;
	fb_info.width = width;
	fb_info.height = height;
	fb_info.layers = 1;

	uint32_t i;

	framebuffers = (VkFramebuffer*)malloc(swapchainImageCount *sizeof(VkFramebuffer));

	for (i = 0; i < swapchainImageCount; i++) {
		attachments[0] = scBuffer[i].view;
		res = vkCreateFramebuffer(device, &fb_info, NULL, &framebuffers[i]);
		assert(res == VK_SUCCESS);
	}

	return res;
}
//
//void createDescriptorPool(bool use_texture) {
//	/* DEPENDS on init_uniform_buffer() and
//	* init_descriptor_and_pipeline_layouts() */
//	VkResult U_ASSERT_ONLY res;
//	int size = uniformVar.size();
//
//	VkDescriptorPoolSize *type_count = new VkDescriptorPoolSize[size];
//
//	for (int i = 0; i < size; i++) {
//		type_count[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//		type_count[i].descriptorCount = 1;
//	}
//	VkDescriptorPoolCreateInfo descriptor_pool = {};
//	descriptor_pool.sType =
//		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//	descriptor_pool.pNext = NULL;
//	descriptor_pool.maxSets = 1;
//	descriptor_pool.poolSizeCount = size;
//	descriptor_pool.pPoolSizes = type_count;
//	res = vkCreateDescriptorPool(device, &descriptor_pool, NULL, &desc_pool);
//	assert(res == VK_SUCCESS);
//}
//
//void createDescriptorSet(bool use_texture) {
//	/* DEPENDS on init_descriptor_pool() */
//	VkResult U_ASSERT_ONLY res;
//	VkDescriptorSetAllocateInfo alloc_info[1];
//	alloc_info[0].sType =
//		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//	alloc_info[0].pNext = NULL;
//	alloc_info[0].descriptorPool = desc_pool;
//	alloc_info[0].descriptorSetCount = 1;
//	alloc_info[0].pSetLayouts = desc_layout.data();
//	desc_set.resize(num_descriptor_sets);
//	res =
//		vkAllocateDescriptorSets(device, alloc_info, desc_set.data());
//	assert(res == VK_SUCCESS);
//	int size = uniformVar.size() - 1;
//
//	VkWriteDescriptorSet *writes = new VkWriteDescriptorSet[size];
//	for (int i = 0; i < size; i++) {
//		writes[i] = {};
//		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		writes[i].pNext = NULL;
//		writes[i].dstSet = desc_set[0];
//		writes[i].descriptorCount = 1;
//		writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//		writes[i].pBufferInfo = &uniform_d[i].buffer_info;
//		writes[i].dstArrayElement = 0;
//		writes[i].dstBinding = 0;
//	}
//
//	vkUpdateDescriptorSets(device, size, writes, 0, NULL);
//}
//
void createPipeLineCache() {
	VkResult U_ASSERT_ONLY res;

	VkPipelineCacheCreateInfo pipelineCache;
	pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCache.pNext = NULL;
	pipelineCache.initialDataSize = 0;
	pipelineCache.pInitialData = NULL;
	pipelineCache.flags = 0;
	res = vkCreatePipelineCache(device, &pipelineCache, NULL,
		&pipelineCaches);
	assert(res == VK_SUCCESS);
}
//
//void init_pipeline(VkBool32 include_depth, VkBool32 include_vi) {
//	VkResult U_ASSERT_ONLY res;
//
//	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
//	VkPipelineDynamicStateCreateInfo dynamicState = {};
//	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
//	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//	dynamicState.pNext = NULL;
//	dynamicState.pDynamicStates = dynamicStateEnables;
//	dynamicState.dynamicStateCount = 0;
//
//	VkPipelineVertexInputStateCreateInfo vi;
//	memset(&vi, 0, sizeof(vi));
//	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//	if (include_vi) {
//		vi.pNext = NULL;
//		vi.flags = 0;
//		vi.vertexBindingDescriptionCount = 1;
//		vi.pVertexBindingDescriptions = &vi_binding;
//		vi.vertexAttributeDescriptionCount = 2;
//		vi.pVertexAttributeDescriptions = vi_attribs;
//	}
//	VkPipelineInputAssemblyStateCreateInfo ia;
//	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//	ia.pNext = NULL;
//	ia.flags = 0;
//	ia.primitiveRestartEnable = VK_FALSE;
//	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//
//	VkPipelineRasterizationStateCreateInfo rs;
//	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//	rs.pNext = NULL;
//	rs.flags = 0;
//	rs.polygonMode = VK_POLYGON_MODE_FILL;
//	rs.cullMode = VK_CULL_MODE_BACK_BIT;
//	rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
//	rs.depthClampEnable = include_depth;
//	rs.rasterizerDiscardEnable = VK_FALSE;
//	rs.depthBiasEnable = VK_FALSE;
//	rs.depthBiasConstantFactor = 0;
//	rs.depthBiasClamp = 0;
//	rs.depthBiasSlopeFactor = 0;
//	rs.lineWidth = 1.0f;
//
//	VkPipelineColorBlendStateCreateInfo cb;
//	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//	cb.flags = 0;
//	cb.pNext = NULL;
//	VkPipelineColorBlendAttachmentState att_state[1];
//	att_state[0].colorWriteMask = 0xf;
//	att_state[0].blendEnable = VK_FALSE;
//	att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
//	att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
//	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
//	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
//	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//	cb.attachmentCount = 1;
//	cb.pAttachments = att_state;
//	cb.logicOpEnable = VK_FALSE;
//	cb.logicOp = VK_LOGIC_OP_NO_OP;
//	cb.blendConstants[0] = 1.0f;
//	cb.blendConstants[1] = 1.0f;
//	cb.blendConstants[2] = 1.0f;
//	cb.blendConstants[3] = 1.0f;
//
//	VkPipelineViewportStateCreateInfo vp = {};
//	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//	vp.pNext = NULL;
//	vp.flags = 0;
//
//	// Temporary disabling dynamic viewport on Android because some of drivers doesn't
//	// support the feature.
//	VkViewport viewports;
//	viewports.minDepth = 0.0f;
//	viewports.maxDepth = 1.0f;
//	viewports.x = 0;
//	viewports.y = 0;
//	viewports.width = width;
//	viewports.height = height;
//	VkRect2D scissor;
//	scissor.extent.width = width;
//	scissor.extent.height = height;
//	scissor.offset.x = 0;
//	scissor.offset.y = 0;
//	vp.viewportCount = NUM_VIEWPORTS;
//	vp.scissorCount = NUM_SCISSORS;
//	vp.pScissors = &scissor;
//	vp.pViewports = &viewports;
//
//	VkPipelineDepthStencilStateCreateInfo ds;
//	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//	ds.pNext = NULL;
//	ds.flags = 0;
//	ds.depthTestEnable = include_depth;
//	ds.depthWriteEnable = include_depth;
//	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
//	ds.depthBoundsTestEnable = VK_FALSE;
//	ds.stencilTestEnable = VK_FALSE;
//	ds.back.failOp = VK_STENCIL_OP_KEEP;
//	ds.back.passOp = VK_STENCIL_OP_KEEP;
//	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
//	ds.back.compareMask = 0;
//	ds.back.reference = 0;
//	ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
//	ds.back.writeMask = 0;
//	ds.minDepthBounds = 0;
//	ds.maxDepthBounds = 0;
//	ds.stencilTestEnable = VK_FALSE;
//	ds.front = ds.back;
//
//	VkPipelineMultisampleStateCreateInfo ms;
//	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//	ms.pNext = NULL;
//	ms.flags = 0;
//	ms.pSampleMask = NULL;
//	ms.rasterizationSamples = NUM_SAMPLES;
//	ms.sampleShadingEnable = VK_FALSE;
//	ms.alphaToCoverageEnable = VK_FALSE;
//	ms.alphaToOneEnable = VK_FALSE;
//	ms.minSampleShading = 0.0;
//
//	VkGraphicsPipelineCreateInfo pipeline;
//	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//	pipeline.pNext = NULL;
//	pipeline.layout = pipeline_layout;
//	pipeline.basePipelineHandle = VK_NULL_HANDLE;
//	pipeline.basePipelineIndex = 0;
//	pipeline.flags = 0;
//	pipeline.pVertexInputState = &vi;
//	pipeline.pInputAssemblyState = &ia;
//	pipeline.pRasterizationState = &rs;
//	pipeline.pColorBlendState = &cb;
//	pipeline.pTessellationState = NULL;
//	pipeline.pMultisampleState = &ms;
//	pipeline.pDynamicState = &dynamicState;
//	pipeline.pViewportState = &vp;
//	pipeline.pDepthStencilState = &ds;
//	pipeline.pStages = shaderStages;
//	pipeline.stageCount = 2;
//	pipeline.renderPass = render_pass;
//	pipeline.subpass = 0;
//
//	res = vkCreateGraphicsPipelines(device, pipelineCaches, 1,
//		&pipeline, NULL, &pipeLines);
//	assert(res == VK_SUCCESS);
//}
//

void renderObject(struct AppState& state) {
	VkResult U_ASSERT_ONLY res;
	VkClearValue clear_values[2];
	clear_values[0].color.float32[0] = 0.2f;
	clear_values[0].color.float32[1] = 0.2f;
	clear_values[0].color.float32[2] = 0.2f;
	clear_values[0].color.float32[3] = 0.2f;
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	VkSemaphore imageAcquiredSemaphore;
	VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
	imageAcquiredSemaphoreCreateInfo.sType =
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	imageAcquiredSemaphoreCreateInfo.pNext = NULL;
	imageAcquiredSemaphoreCreateInfo.flags = 0;

	res = vkCreateSemaphore(device, &imageAcquiredSemaphoreCreateInfo,
		NULL, &imageAcquiredSemaphore);
	assert(res == VK_SUCCESS);

	// Get the index of the next available swapchain image:
	res = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
		imageAcquiredSemaphore, VK_NULL_HANDLE,
		&current_buffer);
	// TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
	// return codes
	assert(res == VK_SUCCESS);

	set_image_layout(scBuffer[current_buffer].image,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, state.cmd);

	VkRenderPassBeginInfo rp_begin;
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.pNext = NULL;
	rp_begin.renderPass = render_pass;
	rp_begin.framebuffer = framebuffers[current_buffer];
	rp_begin.renderArea.offset.x = 0;
	rp_begin.renderArea.offset.y = 0;
	rp_begin.renderArea.extent.width = width;
	rp_begin.renderArea.extent.height = height;
	rp_begin.clearValueCount = 2;
	rp_begin.pClearValues = clear_values;

	vkCmdBeginRenderPass(state.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(state.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
	vkCmdBindDescriptorSets(state.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		state.pipeline_layout, 0, state.num_descriptor_sets,
		state.desc_set.data(), 0, NULL);

	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(state.cmd, 0, 1, &state.vertex_buffer.buf, offsets);

	init_viewports(state.cmd);
	init_scissors(state.cmd);

	vkCmdBindIndexBuffer(state.cmd, state.iBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(state.cmd, 36, 1, 0, 0, 0);
	vkCmdEndRenderPass(state.cmd);

	res = vkEndCommandBuffer(state.cmd);
	const VkCommandBuffer cmd_bufs[] = { state.cmd };
	VkFenceCreateInfo fenceInfo;
	VkFence drawFence;
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = NULL;
	fenceInfo.flags = 0;
	vkCreateFence(device, &fenceInfo, NULL, &drawFence);

	VkPipelineStageFlags pipe_stage_flags =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info[1] = {};
	submit_info[0].pNext = NULL;
	submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info[0].waitSemaphoreCount = 1;
	submit_info[0].pWaitSemaphores = &imageAcquiredSemaphore;
	submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
	submit_info[0].commandBufferCount = 1;
	submit_info[0].pCommandBuffers = cmd_bufs;
	submit_info[0].signalSemaphoreCount = 0;
	submit_info[0].pSignalSemaphores = NULL;

	/* Queue the command buffer for execution */
	res = vkQueueSubmit(graphics_queue, 1, submit_info, drawFence);
	assert(res == VK_SUCCESS);

	/* Now present the image in the window */

	VkPresentInfoKHR present;
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = NULL;
	present.swapchainCount = 1;
	present.pSwapchains = &swap_chain;
	present.pImageIndices = &current_buffer;
	present.pWaitSemaphores = NULL;
	present.waitSemaphoreCount = 0;
	present.pResults = NULL;

	/* Make sure command buffer is finished before presenting */
	do {
		res =
			vkWaitForFences(device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
	} while (res == VK_TIMEOUT);

	assert(res == VK_SUCCESS);
	res = vkQueuePresentKHR(present_queue, &present);
	assert(res == VK_SUCCESS);

	/* VULKAN_KEY_END */

	//cout << "ready to sleep" << endl;

	wait_seconds(5);
}

void init_viewports(VkCommandBuffer cmd) {
	viewport.height = (float)height;
	viewport.width = (float)width;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	viewport.x = 0;
	viewport.y = 0;
	vkCmdSetViewport(cmd, 0, NUM_VIEWPORTS, &viewport);
}

void init_scissors(VkCommandBuffer cmd) {
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(cmd, 0, NUM_SCISSORS, &scissor);
}

/*------------------------------------------------------------------------------------------------->*/
bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties memory_properties, uint32_t typeBits,
	VkFlags requirements_mask,
	uint32_t* typeIndex) {
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((memory_properties.memoryTypes[i].propertyFlags &
				requirements_mask) == requirements_mask) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}

void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkCommandBuffer cmd) {
	/* DEPENDS on cmd and queue initialized */

	assert(cmd != VK_NULL_HANDLE);
	assert(graphics_queue != VK_NULL_HANDLE);

	VkImageMemoryBarrier image_memory_barrier = {};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = 0;
	image_memory_barrier.oldLayout = old_image_layout;
	image_memory_barrier.newLayout = new_image_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = aspectMask;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.srcAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		image_memory_barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmd, src_stages, dest_stages, 0, 0, NULL, 0, NULL,
		1, &image_memory_barrier);
}

void init_glslang() {
	glslang::InitializeProcess();

}

void finalize_glslang(){
	glslang::FinalizeProcess();

}

//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char* pshader,
	std::vector<unsigned int>& spirv) {
	EShLanguage stage = FindLanguage(shader_type);
	glslang::TShader shader(stage);
	glslang::TProgram program;
	const char* shaderStrings[1];
	TBuiltInResource Resources;
	init_resources(Resources);

	// Enable SPIR-V and Vulkan rules when parsing GLSL
	EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

	shaderStrings[0] = pshader;
	shader.setStrings(shaderStrings, 1);

	if (!shader.parse(&Resources, 100, false, messages)) {
		puts(shader.getInfoLog());
		puts(shader.getInfoDebugLog());
		return false; // something didn't work
	}

	program.addShader(&shader);

	//
	// Program-level processing...
	//

	if (!program.link(messages)) {
		puts(shader.getInfoLog());
		puts(shader.getInfoDebugLog());
		fflush(stdout);
		return false;
	}

	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

	return true;
}

EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type) {
	switch (shader_type) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return EShLangVertex;

	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
		return EShLangTessControl;

	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
		return EShLangTessEvaluation;

	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return EShLangGeometry;

	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return EShLangFragment;

	case VK_SHADER_STAGE_COMPUTE_BIT:
		return EShLangCompute;

	default:
		return EShLangVertex;
	}
}

void init_resources(TBuiltInResource& Resources) {
	Resources.maxLights = 32;
	Resources.maxClipPlanes = 6;
	Resources.maxTextureUnits = 32;
	Resources.maxTextureCoords = 32;
	Resources.maxVertexAttribs = 64;
	Resources.maxVertexUniformComponents = 4096;
	Resources.maxVaryingFloats = 64;
	Resources.maxVertexTextureImageUnits = 32;
	Resources.maxCombinedTextureImageUnits = 80;
	Resources.maxTextureImageUnits = 32;
	Resources.maxFragmentUniformComponents = 4096;
	Resources.maxDrawBuffers = 32;
	Resources.maxVertexUniformVectors = 128;
	Resources.maxVaryingVectors = 8;
	Resources.maxFragmentUniformVectors = 16;
	Resources.maxVertexOutputVectors = 16;
	Resources.maxFragmentInputVectors = 15;
	Resources.minProgramTexelOffset = -8;
	Resources.maxProgramTexelOffset = 7;
	Resources.maxClipDistances = 8;
	Resources.maxComputeWorkGroupCountX = 65535;
	Resources.maxComputeWorkGroupCountY = 65535;
	Resources.maxComputeWorkGroupCountZ = 65535;
	Resources.maxComputeWorkGroupSizeX = 1024;
	Resources.maxComputeWorkGroupSizeY = 1024;
	Resources.maxComputeWorkGroupSizeZ = 64;
	Resources.maxComputeUniformComponents = 1024;
	Resources.maxComputeTextureImageUnits = 16;
	Resources.maxComputeImageUniforms = 8;
	Resources.maxComputeAtomicCounters = 8;
	Resources.maxComputeAtomicCounterBuffers = 1;
	Resources.maxVaryingComponents = 60;
	Resources.maxVertexOutputComponents = 64;
	Resources.maxGeometryInputComponents = 64;
	Resources.maxGeometryOutputComponents = 128;
	Resources.maxFragmentInputComponents = 128;
	Resources.maxImageUnits = 8;
	Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	Resources.maxCombinedShaderOutputResources = 8;
	Resources.maxImageSamples = 0;
	Resources.maxVertexImageUniforms = 0;
	Resources.maxTessControlImageUniforms = 0;
	Resources.maxTessEvaluationImageUniforms = 0;
	Resources.maxGeometryImageUniforms = 0;
	Resources.maxFragmentImageUniforms = 8;
	Resources.maxCombinedImageUniforms = 8;
	Resources.maxGeometryTextureImageUnits = 16;
	Resources.maxGeometryOutputVertices = 256;
	Resources.maxGeometryTotalOutputComponents = 1024;
	Resources.maxGeometryUniformComponents = 1024;
	Resources.maxGeometryVaryingComponents = 64;
	Resources.maxTessControlInputComponents = 128;
	Resources.maxTessControlOutputComponents = 128;
	Resources.maxTessControlTextureImageUnits = 16;
	Resources.maxTessControlUniformComponents = 1024;
	Resources.maxTessControlTotalOutputComponents = 4096;
	Resources.maxTessEvaluationInputComponents = 128;
	Resources.maxTessEvaluationOutputComponents = 128;
	Resources.maxTessEvaluationTextureImageUnits = 16;
	Resources.maxTessEvaluationUniformComponents = 1024;
	Resources.maxTessPatchComponents = 120;
	Resources.maxPatchVertices = 32;
	Resources.maxTessGenLevel = 64;
	Resources.maxViewports = 16;
	Resources.maxVertexAtomicCounters = 0;
	Resources.maxTessControlAtomicCounters = 0;
	Resources.maxTessEvaluationAtomicCounters = 0;
	Resources.maxGeometryAtomicCounters = 0;
	Resources.maxFragmentAtomicCounters = 8;
	Resources.maxCombinedAtomicCounters = 8;
	Resources.maxAtomicCounterBindings = 1;
	Resources.maxVertexAtomicCounterBuffers = 0;
	Resources.maxTessControlAtomicCounterBuffers = 0;
	Resources.maxTessEvaluationAtomicCounterBuffers = 0;
	Resources.maxGeometryAtomicCounterBuffers = 0;
	Resources.maxFragmentAtomicCounterBuffers = 1;
	Resources.maxCombinedAtomicCounterBuffers = 1;
	Resources.maxAtomicCounterBufferSize = 16384;
	Resources.maxTransformFeedbackBuffers = 4;
	Resources.maxTransformFeedbackInterleavedComponents = 64;
	Resources.maxCullDistances = 8;
	Resources.maxCombinedClipAndCullDistances = 8;
	Resources.maxSamples = 4;
	Resources.limits.nonInductiveForLoops = 1;
	Resources.limits.whileLoops = 1;
	Resources.limits.doWhileLoops = 1;
	Resources.limits.generalUniformIndexing = 1;
	Resources.limits.generalAttributeMatrixVectorIndexing = 1;
	Resources.limits.generalVaryingIndexing = 1;
	Resources.limits.generalSamplerIndexing = 1;
	Resources.limits.generalVariableIndexing = 1;
	Resources.limits.generalConstantMatrixVectorIndexing = 1;
}

void wait_seconds(int seconds) {
#ifdef WIN32
	Sleep(seconds * 1000);
#else
	sleep(seconds);
#endif
}

timestamp_t get_milliseconds() {
#ifdef WIN32
	LARGE_INTEGER frequency;
	BOOL useQPC = QueryPerformanceFrequency(&frequency);
	if (useQPC) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / frequency.QuadPart;
	}
	else {
		return GetTickCount();
	}
#else
	struct timeval now;
	gettimeofday(&now, NULL);
	return (now.tv_usec / 1000) + (timestamp_t)now.tv_sec;
#endif
}

//***********************************************************************************************

void destroy_instance() {
	vkDestroyInstance(inst, NULL);
}