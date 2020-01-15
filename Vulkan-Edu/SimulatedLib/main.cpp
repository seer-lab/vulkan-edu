#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "LHVulkan.h"

int main() {
	VkResult res;
	createInstance();
	createDeviceInfo();
	createWindowContext(512, 512);
	createSwapChain();

	//res = createInstance();
	//res = createDeviceInfo();
	//createWindowContext(1280, 720);
	//res = createSwapChain();
}