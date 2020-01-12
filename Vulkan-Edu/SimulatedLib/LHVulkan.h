#include <vulkan/vulkan.h>

#if _WIN32
#include <vulkan/vulkan_win32.h>
#endif

#include <string>
#include <assert.h>
#include <vector>


#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define LAYER_COUNT 0
#define LAYER_NAME NULL


//TODO Change this such that it can handle Multiplatform code
#define EXTENSION_COUNT 2
#define EXTENSION_NAMES VK_KHR_SURFACE_EXTENSION_NAME | VK_KHR_WIN32_SURFACE_EXTENSION_NAME


//Global Variable
VkInstance instanceCreate;

VkResult createInstance(std::string appName = "Sample App", std::string engineName = "Sample Engine") {
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
	extention_types.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
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

VkResult createDeviceInfo() {
	std::vector<const char*> device_extention_types;
	device_extention_types.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	uint32_t const U_ASSERT_ONLY req_count = gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, NULL);
	assert(gpu_count);
	info.gpus.resize(gpu_count);

	res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, info.gpus.data());
	assert(!res && gpu_count >= req_count);

	vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0],
		&info.queue_family_count, NULL);
	assert(info.queue_family_count >= 1);

	info.queue_props.resize(info.queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(
		info.gpus[0], &info.queue_family_count, info.queue_props.data());
	assert(info.queue_family_count >= 1);

	/* This is as good a place as any to do this */
	vkGetPhysicalDeviceMemoryProperties(info.gpus[0], &info.memory_properties);
	vkGetPhysicalDeviceProperties(info.gpus[0], &info.gpu_props);

	return res;
}

