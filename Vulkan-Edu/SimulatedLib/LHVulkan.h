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

VkResult init_global_extension_propertiesT(layer_properties& layer_props);
VkResult globalLayerProperties();
void deviceExtentionName();
void instanceExtentionName();
VkResult createInstance(std::string appName = "Sample App", std::string engineName = "Sample Engine");
VkResult createDeviceInfo();
VkResult createSwapChain();
VkResult createDevice();

#define LAYER_COUNT 0
#define LAYER_NAME NULL

//TODO Change this such that it can handle Multiplatform code
#define EXTENSION_COUNT 2
#define EXTENSION_NAMES VK_KHR_SURFACE_EXTENSION_NAME | VK_KHR_WIN32_SURFACE_EXTENSION_NAME

//Global Variable
static VkInstance inst;
std::string name;
uint32_t queue_family_count;
uint32_t graphics_queue_family_index;
uint32_t present_queue_family_index;
VkPhysicalDeviceMemoryProperties memory_properties;
VkPhysicalDeviceProperties gpu_props;
VkDevice device;
VkSurfaceKHR surface;
VkFormat format;

std::vector<const char*> device_extension_names;
std::vector<const char*> instance_layer_names;
std::vector<const char*> instance_extension_names;
std::vector<VkPhysicalDevice> gpus;
std::vector<VkQueueFamilyProperties> queue_props;
std::vector<layer_properties> instance_layer_properties;

//TODO MAKE MULTIPLATFORM (Surface Creation)
static HINSTANCE connection;
static HWND window;
long info;

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

static void createWindowContext(int height, int width) {
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

VkResult createSwapChain() {
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

	//// Iterate over each queue to learn whether it supports presenting:
	//VkBool32* pSupportsPresent =
	//	(VkBool32*)malloc(info.queue_family_count * sizeof(VkBool32));
	//for (uint32_t i = 0; i < info.queue_family_count; i++) {
	//	vkGetPhysicalDeviceSurfaceSupportKHR(info.gpus[0], i, info.surface,
	//		&pSupportsPresent[i]);
	//}

	//// Search for a graphics and a present queue in the array of queue
	//// families, try to find one that supports both
	//info.graphics_queue_family_index = UINT32_MAX;
	//info.present_queue_family_index = UINT32_MAX;
	//for (uint32_t i = 0; i < info.queue_family_count; ++i) {
	//	if ((info.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
	//		if (info.graphics_queue_family_index == UINT32_MAX)
	//			info.graphics_queue_family_index = i;

	//		if (pSupportsPresent[i] == VK_TRUE) {
	//			info.graphics_queue_family_index = i;
	//			info.present_queue_family_index = i;
	//			break;
	//		}
	//	}
	//}

	//if (info.present_queue_family_index == UINT32_MAX) {
	//	// If didn't find a queue that supports both graphics and present, then
	//	// find a separate present queue.
	//	for (size_t i = 0; i < info.queue_family_count; ++i)
	//		if (pSupportsPresent[i] == VK_TRUE) {
	//			info.present_queue_family_index = i;
	//			break;
	//		}
	//}
	//free(pSupportsPresent);

	//// Generate error if could not find queues that support graphics
	//// and present
	//if (info.graphics_queue_family_index == UINT32_MAX ||
	//	info.present_queue_family_index == UINT32_MAX) {
	//	std::cout << "Could not find a queues for both graphics and present";
	//	exit(-1);
	//}

	//// Get the list of VkFormats that are supported:
	//uint32_t formatCount;
	//res = vkGetPhysicalDeviceSurfaceFormatsKHR(info.gpus[0], info.surface,
	//	&formatCount, NULL);
	//assert(res == VK_SUCCESS);
	//VkSurfaceFormatKHR* surfFormats =
	//	(VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
	//res = vkGetPhysicalDeviceSurfaceFormatsKHR(info.gpus[0], info.surface,
	//	&formatCount, surfFormats);
	//assert(res == VK_SUCCESS);
	//// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	//// the surface has no preferred format.  Otherwise, at least one
	//// supported format will be returned.
	//if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
	//	info.format = VK_FORMAT_B8G8R8A8_UNORM;
	//}
	//else {
	//	assert(formatCount >= 1);
	//	info.format = surfFormats[0].format;
	//}
	//free(surfFormats);
	return res;
}

//VkResult createDevice() {
//	VkResult res;
//	VkDeviceQueueCreateInfo queue_info = {};
//
//	float queue_priorities[1] = { 0.0 };
//	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//	queue_info.pNext = NULL;
//	queue_info.queueCount = 1;
//	queue_info.pQueuePriorities = queue_priorities;
//	queue_info.queueFamilyIndex = graphics_queue_family_index;
//
//	VkDeviceCreateInfo device_info = {};
//	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//	device_info.pNext = NULL;
//	device_info.queueCreateInfoCount = 1;
//	device_info.pQueueCreateInfos = &queue_info;
//	device_info.enabledExtensionCount = device_extension_names.size();
//	device_info.ppEnabledExtensionNames =
//		device_info.enabledExtensionCount ? device_extension_names.data()
//		: NULL;
//	device_info.pEnabledFeatures = NULL;
//
//	res = vkCreateDevice(gpus[0], &device_info, NULL, &device);
//	std::cout << res << "\n";
//	assert(res == VK_SUCCESS);
//
//	return res;
//}
