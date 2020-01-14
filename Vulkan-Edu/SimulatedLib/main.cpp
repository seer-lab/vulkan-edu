#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "LHVulkan.h"

int main() {
	VkResult res;

	res = createInstance();
	res = createDeviceInfo();
	res = createWindowContext(1920, 1080);
}