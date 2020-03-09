#include "LHVulkan.h"

void vulkanInit(struct LHContext &context) {
	if (!glfwInit()) {
		std::cerr << "Error INIT FAILED\n";
		exit(-1);
	}
	if (!glfwVulkanSupported()) {
		std::cerr << "Vulkan is not supported\n";
	}

	uint32_t ext_count;
	const char** extName = glfwGetRequiredInstanceExtensions(&ext_count);

	for (int i = 0; i < ext_count; i++) {
		context.instance_extension_names.push_back(extName[i]);
		std::cout << context.instance_extension_names[i] << std::endl;
	}
}

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

		layer_props.instance_extensions.resize(instance_extension_count);
		instance_extensions = layer_props.instance_extensions.data();
		res = vkEnumerateInstanceExtensionProperties(
			layer_name, &instance_extension_count, instance_extensions);
	} while (res == VK_INCOMPLETE);

	return res;
}

VkResult globalLayerProperties(struct LHContext &context) {
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

		res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
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
		context.instance_layer_properties.push_back(layer_props);
	}
	free(vk_props);

	return res;
}

VkResult init_device_extension_properties(struct LHContext &context, layer_properties& layer_props) {
	VkExtensionProperties* device_extensions;
	uint32_t device_extension_count;
	VkResult res;
	char* layer_name = NULL;

	layer_name = layer_props.properties.layerName;

	do {
		res = vkEnumerateDeviceExtensionProperties(context.gpus[context.selectedGPU], layer_name, &device_extension_count, NULL);
		if (res) return res;

		if (device_extension_count == 0) {
			return VK_SUCCESS;
		}

		layer_props.device_extensions.resize(device_extension_count);
		device_extensions = layer_props.device_extensions.data();
		res = vkEnumerateDeviceExtensionProperties(context.gpus[context.selectedGPU], layer_name, &device_extension_count, device_extensions);
	} while (res == VK_INCOMPLETE);

	return res;
}

void init_device_extension_names(struct LHContext& context) {
	context.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

VkResult createInstance(struct LHContext& context, std::string appName, std::string engineName, bool enableValidation) {
	
	vulkanInit(context);
	globalLayerProperties(context);
	init_device_extension_names(context);

	VkResult U_ASSERT_ONLY res;
	context.name = appName;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = appName.c_str();
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = engineName.c_str();
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext = NULL;
	inst_info.flags = 0;
	inst_info.pApplicationInfo = &app_info;
	if (enableValidation) {
		//TODO: Debugging Validation Layer
	}
	inst_info.enabledLayerCount = context.instance_layer_names.size();
	inst_info.ppEnabledLayerNames = context.instance_layer_names.size() ? context.instance_layer_names.data() : NULL;
	inst_info.enabledExtensionCount = context.instance_extension_names.size();
	inst_info.ppEnabledExtensionNames = context.instance_extension_names.data();

	res = vkCreateInstance(&inst_info, NULL, &context.instance);
	assert(res == VK_SUCCESS);

	return res;
}

VkResult createDeviceInfo(struct LHContext& context, bool selectGPU) {
	uint32_t gpu_count = 0;
	uint32_t const U_ASSERT_ONLY req_count = gpu_count;
	VkResult res = vkEnumeratePhysicalDevices(context.instance, &gpu_count, NULL);
	assert(gpu_count);
	
	context.gpus.resize(gpu_count);
	
	res = vkEnumeratePhysicalDevices(context.instance, &gpu_count, context.gpus.data());
	assert(!res && gpu_count >= req_count);

	uint32_t selectGPUs = 0;
	context.selectedGPU = selectGPUs;

	if (selectGPU) {
		std::string input;
		std::cout << "Available Vulkan Devices" << std::endl;

		for (uint32_t i = 0; i < gpu_count; i++) {
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(context.gpus[i], &deviceProperties);
			std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
			std::cout << " Type: " << physicalDeviceTypeString(deviceProperties.deviceType) << std::endl;
			std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << std::endl;
		}

		std::cout << "Please pick the GPU" << std::endl;
		std::cin >> input;
		std::cout << "Selected GPU: " << input << std::endl;
		selectGPUs = std::stoi(input);
		context.selectedGPU = selectGPUs;
	}

	context.physicalDevice = context.gpus[context.selectedGPU];

	vkGetPhysicalDeviceProperties(context.physicalDevice, &context.deviceProperties);
	vkGetPhysicalDeviceFeatures(context.physicalDevice, &context.deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(context.physicalDevice, &context.deviceMemoryProperties);
	
	vkGetPhysicalDeviceQueueFamilyProperties(context.gpus[context.selectedGPU], &context.queue_family_count, NULL);
	assert(context.queue_family_count >= 1);
	
	context.queue_props.resize(context.queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(context.gpus[context.selectedGPU], &context.queue_family_count, context.queue_props.data());
	assert(context.queue_family_count >= 1);
	
	
	/* This is as good a place as any to do this */
	vkGetPhysicalDeviceMemoryProperties(context.gpus[context.selectedGPU], &context.memory_properties);
	vkGetPhysicalDeviceProperties(context.gpus[context.selectedGPU], &context.gpu_props);
	
	for (auto& layer_props : context.instance_layer_properties) {
		init_device_extension_properties(context, layer_props);
	}


	return res;

}

void createWindowContext(struct LHContext &context,int w, int h) {

	context.width = w;
	context.height = h;

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	context.window = glfwCreateWindow(w, h, context.name.c_str(), NULL, NULL);

	if (glfwCreateWindowSurface(context.instance, context.window, nullptr, &context.surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

VkResult createSwapChainExtention(struct LHContext &context) {
	VkResult U_ASSERT_ONLY res;

	// Create the os-specific surface
	// TODO: Test on other OS
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
	surfaceCreateInfo.hwnd = glfwGetWin32Window(context.window);
	res = vkCreateWin32SurfaceKHR(context.instance, &surfaceCreateInfo, nullptr, &context.surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pView = view;
	err = vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.display = display;
	surfaceCreateInfo.surface = window;
	err = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = connection;
	surfaceCreateInfo.window = window;
	err = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#endif
	assert(res == VK_SUCCESS);

	// Iterate over each queue to learn whether it supports presenting:
	VkBool32* pSupportsPresent = (VkBool32*)malloc(context.queue_family_count * sizeof(VkBool32));
	for (uint32_t i = 0; i < context.queue_family_count; i++) {
		vkGetPhysicalDeviceSurfaceSupportKHR(context.gpus[context.selectedGPU], i, context.surface, &pSupportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	context.graphics_queue_family_index = UINT32_MAX;
	context.present_queue_family_index = UINT32_MAX;
	for (uint32_t i = 0; i < context.queue_family_count; ++i) {
		if ((context.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			if (context.graphics_queue_family_index == UINT32_MAX)
				context.graphics_queue_family_index = i;

			if (pSupportsPresent[i] == VK_TRUE) {
				context.graphics_queue_family_index = i;
				context.present_queue_family_index = i;
				break;
			}
		}
	}
//
	if (context.present_queue_family_index == UINT32_MAX) {
		// If didn't find a queue that supports both graphics and present, then
		// find a separate present queue.
		for (size_t i = 0; i < context.queue_family_count; ++i)
			if (pSupportsPresent[i] == VK_TRUE) {
				context.present_queue_family_index = i;
				break;
			}
	}
	free(pSupportsPresent);

	// Generate error if could not find queues that support graphics
	// and present
	if (context.graphics_queue_family_index == UINT32_MAX ||
		context.present_queue_family_index == UINT32_MAX) {
		std::cout << "Could not find a queues for both graphics and present";
		exit(-1);
	}

	// Get the list of VkFormats that are supported:
	uint32_t formatCount;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(context.gpus[context.selectedGPU], context.surface, &formatCount, NULL);
	assert(res == VK_SUCCESS);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(context.gpus[context.selectedGPU], context.surface, &formatCount, surfaceFormats.data());
	assert(res == VK_SUCCESS);
	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		context.format = VK_FORMAT_B8G8R8A8_UNORM;
		context.colorSpace = surfaceFormats[0].colorSpace;
	}
	else {
		assert(formatCount >= 1);
		context.format = surfaceFormats[0].format;
		context.colorSpace = surfaceFormats[0].colorSpace;
	}
	return res;
}


VkResult createDevice(struct LHContext &context) {
	VkResult res;
	VkDeviceQueueCreateInfo queue_info = {};

	float queue_priorities[1] = { 0.0 };
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = queue_priorities;
	queue_info.queueFamilyIndex = context.graphics_queue_family_index;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = NULL;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = context.device_extension_names.size();
	device_info.ppEnabledExtensionNames =
		device_info.enabledExtensionCount ? context.device_extension_names.data()
		: NULL;
	device_info.pEnabledFeatures = NULL;

	res = vkCreateDevice(context.gpus[context.selectedGPU], &device_info, NULL, &context.device);
	assert(res == VK_SUCCESS);
	return res;
}

void createDeviceQueue(struct LHContext& context) {
	vkGetDeviceQueue(context.device, context.graphics_queue_family_index, 0, &context.queue);
	if (context.graphics_queue_family_index == context.present_queue_family_index) {
		context.present_queue = context.queue;
	}
	else {
		vkGetDeviceQueue(context.device, context.present_queue_family_index, 0, &context.present_queue);
	}
}

VkResult createSynchObject(struct LHContext& context) {
	VkResult res;


	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queu
	res =vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.semaphores.presentComplete);
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	res =vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &context.semaphores.renderComplete);

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	context.submitInfo = {};
	context.submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	context.submitInfo.pWaitDstStageMask = &context.submitPipelineStages;
	context.submitInfo.waitSemaphoreCount = 1;
	context.submitInfo.pWaitSemaphores = &context.semaphores.presentComplete;
	context.submitInfo.signalSemaphoreCount = 1;
	context.submitInfo.pSignalSemaphores = &context.semaphores.renderComplete;

	return res;
}

VkResult createCommandPool(struct LHContext &context) {
	VkResult U_ASSERT_ONLY res;

	VkCommandPoolCreateInfo cmd_pool_info = {};
	cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.pNext = NULL;
	cmd_pool_info.queueFamilyIndex = context.graphics_queue_family_index;
	cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	res = vkCreateCommandPool(context.device, &cmd_pool_info, NULL, &context.cmd_pool);
	assert(res == VK_SUCCESS);

	return res;
}

VkResult createSwapChain(struct LHContext &context) {
	VkResult U_ASSERT_ONLY res;

	VkSwapchainKHR oldSwapChain = context.swapChain;
	VkSurfaceCapabilitiesKHR surfCapabilities;

	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.gpus[context.selectedGPU], context.surface, &surfCapabilities);
	assert(res == VK_SUCCESS);

	uint32_t presentModeCount;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(context.gpus[context.selectedGPU], context.surface, &presentModeCount, NULL);
	assert(res == VK_SUCCESS);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(context.gpus[context.selectedGPU], context.surface, &presentModeCount, presentModes.data());
	assert(res == VK_SUCCESS);

	VkExtent2D swapchainExtent = {};
	if (surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
		swapchainExtent.width = context.width;
		swapchainExtent.height = context.height;
	}
	else {
		swapchainExtent = surfCapabilities.currentExtent;
	}

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (int i = 0; i < presentModeCount; i++) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
			swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	uint32_t desiredNumberOfSwapChainImages = surfCapabilities.minImageCount + 1;
	if ((surfCapabilities.maxImageCount > 0) && (desiredNumberOfSwapChainImages > surfCapabilities.maxImageCount)){
		desiredNumberOfSwapChainImages = surfCapabilities.maxImageCount;
	}

	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR){
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else{
		preTransform = surfCapabilities.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainCreateInfoKHR swapchain_ci = {};
	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.pNext = NULL;
	swapchain_ci.surface = context.surface;
	swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
	swapchain_ci.imageFormat = context.format;
	swapchain_ci.imageColorSpace = context.colorSpace;
	swapchain_ci.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = 0;
	swapchain_ci.pQueueFamilyIndices = NULL;
	swapchain_ci.presentMode = swapchainPresentMode;
	swapchain_ci.oldSwapchain = oldSwapChain;
	// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
	swapchain_ci.clipped = VK_TRUE;
	swapchain_ci.compositeAlpha = compositeAlpha;

	// Enable transfer source on swap chain images if supported
	if (surfCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (surfCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchain_ci.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	res = vkCreateSwapchainKHR(context.device, &swapchain_ci, NULL, &context.swapChain);
	assert(res == VK_SUCCESS);

	// If an existing swap chain is re-created, destroy the old swap chain
// This also cleans up all the presentable images
	if (oldSwapChain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < context.swapchainImageCount; i++)
		{
			vkDestroyImageView(context.device, context.buffers[i].view, nullptr);
		}
		vkDestroySwapchainKHR(context.device, oldSwapChain, nullptr);
	}
	res = vkGetSwapchainImagesKHR(context.device, context.swapChain, &context.swapchainImageCount, NULL);
	assert(res == VK_SUCCESS);

	context.images.resize(context.swapchainImageCount);
	res = vkGetSwapchainImagesKHR(context.device, context.swapChain, &context.swapchainImageCount, context.images.data());
	assert(res == VK_SUCCESS);

	context.buffers.resize(context.swapchainImageCount);
	for (uint32_t i = 0; i < context.swapchainImageCount; i++){
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = context.format;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;

		context.buffers[i].image = context.images[i];
		colorAttachmentView.image = context.buffers[i].image;

		res = vkCreateImageView(context.device, &colorAttachmentView, nullptr, &context.buffers[i].view);
		assert(res == VK_SUCCESS);
	}


	return res;
}


VkResult createCommandBuffer(struct LHContext &context) {
	/* DEPENDS on init_swapchain_extension() and init_command_pool() */
	VkResult U_ASSERT_ONLY res;
	context.cmdBuffer.resize(context.swapchainImageCount);
	VkCommandBufferAllocateInfo cmd = {};
	cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd.pNext = NULL;
	cmd.commandPool = context.cmd_pool;
	cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd.commandBufferCount = context.cmdBuffer.size();

	res = vkAllocateCommandBuffers(context.device, &cmd, context.cmdBuffer.data());
	assert(res == VK_SUCCESS);
	return res;
}

VkResult createSynchPrimitive(struct LHContext& context) {
	VkResult U_ASSERT_ONLY res;

	// Wait fences to sync command buffer access
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	context.waitFences.resize(context.cmdBuffer.size());

	for (auto& fence : context.waitFences) {
		res = vkCreateFence(context.device, &fenceCreateInfo, nullptr, &fence);
		assert(res == VK_SUCCESS);
	}
	return res;
}

VkResult createDepthBuffers(struct LHContext &context) {
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	VkImageCreateInfo image_info = {};
	VkFormatProperties props;

#if defined(VK_USE_PLATFORM_IOS_MVK)
if (info.depth.format == VK_FORMAT_UNDEFINED) info.depth.format = VK_FORMAT_D32_SFLOAT;
#else
if (context.depth.format == VK_FORMAT_UNDEFINED) context.depth.format = VK_FORMAT_D16_UNORM;
#endif

	const VkFormat depth_format = context.depth.format;
	vkGetPhysicalDeviceFormatProperties(context.gpus[context.selectedGPU], depth_format, &props);
	if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		image_info.tiling = VK_IMAGE_TILING_LINEAR;
	}
	else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
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
	image_info.extent.width = context.width;
	image_info.extent.height = context.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices = NULL;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.flags = 0;

	res = vkCreateImage(context.device, &image_info, nullptr, &context.depth.image);
	assert(res == VK_SUCCESS);
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(context.device, context.depth.image, &memReqs);

	VkMemoryAllocateInfo memAllloc{};

	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
	
	pass =memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllloc.memoryTypeIndex);
	assert(pass);

	res = vkAllocateMemory(context.device, &memAllloc, NULL, &context.depth.mem);
	assert(res == VK_SUCCESS);
	res = vkBindImageMemory(context.device, context.depth.image, context.depth.mem, 0);
	assert(res == VK_SUCCESS);

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

	if (depth_format == VK_FORMAT_D16_UNORM_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT ||
		depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
		view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	view_info.image = context.depth.image;
	res = vkCreateImageView(context.device, &view_info, NULL, &context.depth.view);
	assert(res == VK_SUCCESS);

}

VkResult createRenderPass(struct LHContext& context,bool includeDepth) {
	VkResult U_ASSERT_ONLY res;
	std::array<VkAttachmentDescription, 2> attachments = {};
	// Color attachment
	attachments[0].format = context.format;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth attachment
	if (includeDepth) {
		attachments[1].format = context.depth.format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = includeDepth ? &depthReference : NULL;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;
	subpassDescription.pResolveAttachments = NULL;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	res = vkCreateRenderPass(context.device, &renderPassInfo, NULL, &context.render_pass);
	assert(res == VK_SUCCESS);
	return res;

}

void createPipeLineCache(struct LHContext &context) {
	VkResult U_ASSERT_ONLY res;

	VkPipelineCacheCreateInfo pipelineCache;
	pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCache.pNext = NULL;
	pipelineCache.initialDataSize = 0;
	pipelineCache.pInitialData = NULL;
	pipelineCache.flags = 0;
	res = vkCreatePipelineCache(context.device, &pipelineCache, NULL,&context.pipelineCache);
	assert(res == VK_SUCCESS);
}

VkResult createFrameBuffer(struct LHContext &context,bool includeDepth) {
	
	VkResult U_ASSERT_ONLY res;
	VkImageView attachments[2];
	attachments[1] = context.depth.view;
	
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = NULL;
	fb_info.renderPass = context.render_pass;
	fb_info.attachmentCount = includeDepth ? 2 : 1;
	fb_info.pAttachments = attachments;
	fb_info.width = context.width;
	fb_info.height = context.height;
	fb_info.layers = 1;

	context.frameBuffers.resize(context.swapchainImageCount);

	for (int i = 0; i < context.swapchainImageCount; i++) {
		attachments[0] = context.buffers[i].view;
		res = vkCreateFramebuffer(context.device, &fb_info, NULL, &context.frameBuffers[i]);
		assert(res == VK_SUCCESS);
	}
	
	return res;
}

VkResult mapVerticiesToGPU(struct LHContext& context,struct vertices &v,struct indices &i, bool useStaging) {
	
	VkResult U_ASSERT_ONLY res;
	bool U_ASSERT_ONLY pass;

	uint8_t* pData;

	VkMemoryAllocateInfo memAlloc = {};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	
	/*

		void *data;

		if (useStagingBuffers)
		{
			// Static data like vertex and index buffer should be stored on the device memory 
			// for optimal (and fastest) access by the GPU
			//
			// To achieve this we use so-called "staging buffers" :
			// - Create a buffer that's visible to the host (and can be mapped)
			// - Copy the data to this buffer
			// - Create another buffer that's local on the device (VRAM) with the same size
			// - Copy the data from the host to the device using a command buffer
			// - Delete the host visible (staging) buffer
			// - Use the device local buffers for rendering

			struct StagingBuffer {
				VkDeviceMemory memory;
				VkBuffer buffer;
			};

			struct {
				StagingBuffer vertices;
				StagingBuffer indices;
			} stagingBuffers;

			// Vertex buffer
			VkBufferCreateInfo vertexBufferInfo = {};
			vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexBufferInfo.size = vertexBufferSize;
			// Buffer is used as the copy source
			vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			// Create a host-visible buffer to copy the vertex data to (staging buffer)
			VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer));
			vkGetBufferMemoryRequirements(device, stagingBuffers.vertices.buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Request a host visible memory type that can be used to copy our data do
			// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
			memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.vertices.memory));
			// Map and copy
			VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.vertices.memory, 0, memAlloc.allocationSize, 0, &data));
			memcpy(data, vertexBuffer.data(), vertexBufferSize);
			vkUnmapMemory(device, stagingBuffers.vertices.memory);
			VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0));

			// Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
			vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			VK_CHECK_RESULT(vkCreateBuffer(device, &vertexBufferInfo, nullptr, &vertices.buffer));
			vkGetBufferMemoryRequirements(device, vertices.buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertices.memory));
			VK_CHECK_RESULT(vkBindBufferMemory(device, vertices.buffer, vertices.memory, 0));

			// Index buffer
			VkBufferCreateInfo indexbufferInfo = {};
			indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			indexbufferInfo.size = indexBufferSize;
			indexbufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			// Copy index data to a buffer visible to the host (staging buffer)
			VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferInfo, nullptr, &stagingBuffers.indices.buffer));
			vkGetBufferMemoryRequirements(device, stagingBuffers.indices.buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffers.indices.memory));
			VK_CHECK_RESULT(vkMapMemory(device, stagingBuffers.indices.memory, 0, indexBufferSize, 0, &data));
			memcpy(data, indexBuffer.data(), indexBufferSize);
			vkUnmapMemory(device, stagingBuffers.indices.memory);
			VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0));

			// Create destination buffer with device only visibility
			indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferInfo, nullptr, &indices.buffer));
			vkGetBufferMemoryRequirements(device, indices.buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indices.memory));
			VK_CHECK_RESULT(vkBindBufferMemory(device, indices.buffer, indices.memory, 0));

			// Buffer copies have to be submitted to a queue, so we need a command buffer for them
			// Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
			VkCommandBuffer copyCmd = getCommandBuffer(true);

			// Put buffer region copies into command buffer
			VkBufferCopy copyRegion = {};

			// Vertex buffer
			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(copyCmd, stagingBuffers.vertices.buffer, vertices.buffer, 1, &copyRegion);
			// Index buffer
			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(copyCmd, stagingBuffers.indices.buffer, indices.buffer,	1, &copyRegion);

			// Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
			flushCommandBuffer(copyCmd);

			// Destroy staging buffers
			// Note: Staging buffer must not be deleted before the copies have been submitted and executed
			vkDestroyBuffer(device, stagingBuffers.vertices.buffer, nullptr);
			vkFreeMemory(device, stagingBuffers.vertices.memory, nullptr);
			vkDestroyBuffer(device, stagingBuffers.indices.buffer, nullptr);
			vkFreeMemory(device, stagingBuffers.indices.memory, nullptr);
		}	
	*/


	if (useStaging) {

	}
	else {
		VkBufferCreateInfo vertextBufInfo = {};
		vertextBufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertextBufInfo.pNext = NULL;
		vertextBufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vertextBufInfo.size = v.dataSize;
		vertextBufInfo.queueFamilyIndexCount = 0;
		vertextBufInfo.pQueueFamilyIndices = NULL;
		vertextBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vertextBufInfo.flags = 0;

		res = vkCreateBuffer(context.device, &vertextBufInfo, NULL, &v.buffer);
		assert(res == VK_SUCCESS);

		vkGetBufferMemoryRequirements(context.device, v.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		pass = memory_type_from_properties(context, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memAlloc.memoryTypeIndex);
		assert(pass && "No mappable coherent memory");

		res = vkAllocateMemory(context.device, &memAlloc, nullptr, &v.memory);
		assert(res == VK_SUCCESS);

		v.buffer_info.range = memReqs.size;
		v.buffer_info.offset = 0;

		res = vkMapMemory(context.device, v.memory, 0, memAlloc.allocationSize, 0, (void**)&pData);
		assert(res == VK_SUCCESS);
		
		memcpy(pData, (const void*)v.vBuffer, v.dataSize);
		vkUnmapMemory(context.device, v.memory);
		res = vkBindBufferMemory(context.device, v.buffer, v.memory, 0);
		assert(res == VK_SUCCESS);

		// Index buffer
		VkBufferCreateInfo indexbufferInfo = {};

		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.pNext = NULL;
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		indexbufferInfo.size = sizeof(i.indices) * i.count;
		indexbufferInfo.queueFamilyIndexCount = 0;
		indexbufferInfo.pQueueFamilyIndices = NULL;
		indexbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		indexbufferInfo.flags = 0;

		// Copy index data to a buffer visible to the host
		res = (vkCreateBuffer(context.device, &indexbufferInfo, nullptr, &i.buffer));
		vkGetBufferMemoryRequirements(context.device, i.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;

		pass = memory_type_from_properties(context, memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&memAlloc.memoryTypeIndex);
		res = (vkAllocateMemory(context.device, &memAlloc, nullptr, &i.memory));
		assert(res == VK_SUCCESS);
		res = (vkMapMemory(context.device, i.memory, 0, memReqs.size, 0, (void**)&pData));
		assert(res == VK_SUCCESS);
		memcpy(pData, i.indices, sizeof(indices) * i.count);
		vkUnmapMemory(context.device, i.memory);
		res = (vkBindBufferMemory(context.device, i.buffer, i.memory, 0));
		assert(res == VK_SUCCESS);
	}
	return res;;
}
//--------------IGNORE FROM HERE---------------------------------------------------------------------->

//
///*------------------------------------------------------------------------------------------------->*/
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

//TODO: Move all of this to a helper file
bool memory_type_from_properties(struct LHContext &context, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex) {
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < context.memory_properties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((context.memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}


std::string physicalDeviceTypeString(VkPhysicalDeviceType type){
	switch (type){
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
		STR(OTHER);
		STR(INTEGRATED_GPU);
		STR(DISCRETE_GPU);
		STR(VIRTUAL_GPU);
#undef STR
	default: return "UNKNOWN_DEVICE_TYPE";
	}
}