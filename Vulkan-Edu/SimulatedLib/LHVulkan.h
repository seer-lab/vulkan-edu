#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <iostream>

#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#include "Window_Factory.h"
#endif

#include <string>
#include <assert.h>
#include <vector>


#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

typedef struct {
	VkLayerProperties properties;
	std::vector<VkExtensionProperties> extensions;
} layer_properties;

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

VkResult init_global_extension_propertiesT(layer_properties& layer_props);
VkResult globalLayerProperties();
void deviceExtentionName();
void instanceExtentionName();
VkResult createInstance(std::string appName = "Sample App", std::string engineName = "Sample Engine");
VkResult createDeviceInfo();
VkResult createSwapChainExtention();
VkResult createDevice();

//Make a utililty file;
void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout, VkCommandBuffer cmd);
bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties memory_properties, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);
#define LAYER_COUNT 0
#define LAYER_NAME NULL
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

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

std::vector<const char*> device_extension_names;
std::vector<const char*> instance_layer_names;
std::vector<const char*> instance_extension_names;
std::vector<VkPhysicalDevice> gpus;
std::vector<VkQueueFamilyProperties> queue_props;
std::vector<layer_properties> instance_layer_properties;
std::vector<swap_chain_buffer> scBuffer;

//TODO MAKE MULTIPLATFORM (Surface Creation)
static HINSTANCE connection;
static HWND window;
long info;
int width;
int height;

depth depths;
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

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	struct sample_info* info = reinterpret_cast<struct sample_info*>(
		GetWindowLongPtr(hWnd, GWLP_USERDATA));
	switch (uMsg) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		//Todo Add a run function for dynamic stuff
		//run(info);
		return 0;
	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

static void createWindowContext(int w, int h) {
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
	createInfo.connection = info.connection;
	createInfo.window = info.window;
	res = vkCreateXcbSurfaceKHR(info.inst, &createInfo,
		NULL, &info.surface);
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

template <class T>
VkResult setUniformValue(T uniformVal, Uniform_Data& uni_data) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(uniformVal);
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;
	res = vkCreateBuffer(device, &buf_info, NULL, &uni_data.buf);
	assert(res == VK_SUCCESS);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, uni_data.buf,&mem_reqs);

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
	
	res = vkAllocateMemory(device, &alloc_info, NULL,&(uni_data.mem));
	assert(res == VK_SUCCESS);
	
	uint8_t* pData;
	res = vkMapMemory(device, uni_data.mem, 0, mem_reqs.size, 0,(void**)&pData);
	assert(res == VK_SUCCESS);
	
	memcpy(pData, &uniformVal, sizeof(uniformVal));
	
	vkUnmapMemory(device, uni_data.mem);
	
	res = vkBindBufferMemory(device, uni_data.buf, uni_data.mem, 0);
	assert(res == VK_SUCCESS);
	
	//state.uniform_data.buffer_info.buffer = state.uniform_data.buf;
	//state.uniform_data.buffer_info.offset = 0;
	//state.uniform_data.buffer_info.range = sizeof(state.MVP);
	
	return res;
}

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
	/* DEPENDS on info.cmd and info.queue initialized */

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