#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "LHVulkan.h"

int main() {
	VkResult res;
	res = createInstance();
	res = createDeviceInfo();
	createWindowContext(512, 512);
	res = createSwapChainExtention(); //Needs a bit more to add
	res = createDevice();

	VkCommandBuffer cmdbuffer;
	res = createCommandPool();
	res = createCommandBuffer(cmdbuffer);
	res = execute_begin_command_buffer(cmdbuffer);
	createDeviceQueue();
	createSwapChain(17);
	createDepthBuffer(cmdbuffer);
	glm::mat4 model = glm::mat4(1.0f);
	Uniform_Data uData;
	setUniformValue<glm::mat4>(model, uData);

}