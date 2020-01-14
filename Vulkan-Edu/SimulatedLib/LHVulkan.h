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


#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif


#define LAYER_COUNT 0
#define LAYER_NAME NULL


//TODO Change this such that it can handle Multiplatform code
#define EXTENSION_COUNT 2
#define EXTENSION_NAMES VK_KHR_SURFACE_EXTENSION_NAME | VK_KHR_WIN32_SURFACE_EXTENSION_NAME

//Global Variable
VkInstance instanceCreate;
std::string name;
uint32_t queue_family_count;
uint32_t graphics_queue_family_index;
uint32_t present_queue_family_index;
VkPhysicalDeviceMemoryProperties memory_properties;
VkPhysicalDeviceProperties gpu_props;
VkSurfaceKHR surface;
VkFormat format;

std::vector<VkPhysicalDevice> gpus;
std::vector<VkQueueFamilyProperties> queue_props;

//TODO MAKE MULTIPLATFORM (Surface Creation)
HINSTANCE connection;
HWND window;

VkResult createInstance(std::string appName = "Sample App", std::string engineName = "Sample Engine") {
	name = appName;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = appName.c_str();
	app_info.applicationVersion = 1;
	app_info.pEngineName = engineName.c_str();
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> extention_types;

	extention_types.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(_WIN32)
	extention_types.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
	info.instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledLayerCount = LAYER_COUNT;
	inst_info.ppEnabledLayerNames = LAYER_NAME;
	inst_info.enabledExtensionCount = extention_types.size();
	inst_info.ppEnabledExtensionNames = extention_types.data();

	VkResult res = vkCreateInstance(&inst_info, NULL, &instanceCreate);
	assert(res == VK_SUCCESS);

	return res;
}


//TODO Make this function select anyother GPU for the user if the user chooses to do so
VkResult createDeviceInfo() {
	std::vector<const char*> device_extention_types;
	device_extention_types.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	uint32_t gpu_count = 1U;
	uint32_t const U_ASSERT_ONLY req_count = gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(instanceCreate, &gpu_count, NULL);
	assert(gpu_count);

	gpus.resize(gpu_count);

	res = vkEnumeratePhysicalDevices(instanceCreate, &gpu_count, gpus.data());
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

static VkResult createWindowContext(int width = 1280, int height = 720) {
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
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name.c_str() , -1, wname, 256);
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
	//Setting the last value might give an error
	SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)NULL);
}


void init_swapchain() {
	VkResult U_ASSERT_ONLY res;

	// Construct the surface description:
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.hinstance = connection;
	createInfo.hwnd = window;
	res = vkCreateWin32SurfaceKHR(instanceCreate, &createInfo,
		NULL, &surface);
#else  // !__ANDROID__ && !_WIN32
	VkXcbSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.connection = connection;
	createInfo.window = window;
	res = vkCreateXcbSurfaceKHR(instanceCreate, &createInfo,
		NULL, surface);
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

}

