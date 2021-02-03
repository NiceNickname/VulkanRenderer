#include "Renderer.h"
#include <stdexcept>
#include <iostream>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessage(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cout << "message from validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}



void Renderer::init(GLFWwindow* windowPointer)
{
	window = windowPointer;
	createInstance();
	setupDebugOutput();
	createSurface(windowPointer);
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createRenderPass();
	createFramebuffers();
	createGraphicsPipeline();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}

void Renderer::draw()
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
		imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { renderingFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("failed to submit draw command buffer!");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % maxFramesInFlight;

}

void Renderer::shutdown()
{
	vkDeviceWaitIdle(device);

	
	
	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		vkDestroySemaphore(device, renderingFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);

	for (auto framebuffer : swapChainFramebuffers)
		vkDestroyFramebuffer(device, framebuffer, nullptr);

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);

	if (validationLayersEnabled)
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void Renderer::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pApplicationName = "Vulkan renderer";
	appInfo.pEngineName = "Vortex Engine";

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	uint32_t extensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);


	
	if (validationLayersEnabled)
	{
		if (allLayersSupported())
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			instanceInfo.enabledLayerCount = validationLayers.size();
			instanceInfo.ppEnabledLayerNames = validationLayers.data();
			VkDebugUtilsMessengerCreateInfoEXT messengerInfo{};
			fillDebugMessengerCreateInfo(messengerInfo);
			instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&messengerInfo;
		}
		else
			throw std::runtime_error("validation layers are not supported");
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
	}

	instanceInfo.enabledExtensionCount = extensions.size();
	instanceInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("cannot create instance");

	std::cout << "INSTANCE CREATED!" << std::endl;
}

void Renderer::createSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		throw std::runtime_error("cannot create surface");
}

bool Renderer::allLayersSupported()
{
	uint32_t layerCount = 0;
	
	std::vector<VkLayerProperties> supportedLayers;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	supportedLayers.resize(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, supportedLayers.data());


	for (const char* layerName : validationLayers)
	{
		bool found = false;

		for (const auto& supportedLayer : supportedLayers)
		{
			if (strcmp(layerName, supportedLayer.layerName) == 0)
				found = true;
		}

		if (!found)
			return false;
	}

	return true;
}

void Renderer::pickPhysicalDevice()
{
	std::vector<VkPhysicalDevice> physicalDevices;

	uint32_t physicalDeviceCount;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

	physicalDevices.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	for (const auto& device : physicalDevices)
	{
		if (checkDeviceRequirements(device))
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);
			std::cout << "picked physical device name: " << properties.deviceName << std::endl;
			physicalDevice = device;
			break;
		}
	}
}

bool Renderer::checkQueueFamilies(VkPhysicalDevice device)
{
	std::vector<VkQueueFamilyProperties> families;
	uint32_t familiesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familiesCount, nullptr);
	
	families.resize(familiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familiesCount, families.data());

	VkBool32 presentSupported = false;
	for (size_t i = 0; i < families.size(); i++)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupported);

		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			queueFamilies.graphicsFamily = i;
		if (presentSupported)
			queueFamilies.presentFamily = i;
	}

	return queueFamilies.graphicsFamily.has_value() && queueFamilies.presentFamily.has_value();
}

void Renderer::createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueInfos{};
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures requiredFeatures{};

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.enabledExtensionCount = requiredExtensions.size();
	deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();
	deviceInfo.queueCreateInfoCount = queueInfos.size();
	deviceInfo.pQueueCreateInfos = queueInfos.data();
	deviceInfo.pEnabledFeatures = &requiredFeatures;
	
	if (validationLayersEnabled)
	{
		deviceInfo.enabledLayerCount = validationLayers.size();
		deviceInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		deviceInfo.enabledLayerCount = 0;

	if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("cannot create logical device");

	vkGetDeviceQueue(device, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilies.presentFamily.value(), 0, &presentQueue);
}

void Renderer::createSwapChain()
{
	SwapChainCapabilities capabilities = getSwapChainCapabilities();

	VkSurfaceFormatKHR format = chooseSwapChainFormat(capabilities.formats);
	VkPresentModeKHR presentMode = chooseSwapChainPresentMode(capabilities.presentModes);
	VkExtent2D extent = chooseSwapChainExtent(capabilities.capabilities);


	VkSwapchainCreateInfoKHR swapChainInfo{};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.minImageCount = capabilities.capabilities.minImageCount + 1;
	swapChainInfo.surface = surface;
	swapChainInfo.imageFormat = format.format;
	swapChainInfo.imageColorSpace = format.colorSpace;
	swapChainInfo.presentMode = presentMode;
	swapChainInfo.imageExtent = extent;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.preTransform = capabilities.capabilities.currentTransform;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	uint32_t queueFamilyIndices[] = { queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value() };
	if (queueFamilies.graphicsFamily.value() != queueFamilies.presentFamily.value())
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain) != VK_SUCCESS)
		throw std::runtime_error("cannot create swapchain!");

	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);

	swapChainImages.resize(swapChainImageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());

	swapChainImageFormat = format.format;
	swapChainExtent = extent;

	createSwapChainImageViews();
}

Renderer::SwapChainCapabilities Renderer::getSwapChainCapabilities()
{
	SwapChainCapabilities swapChainCapabilities;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainCapabilities.capabilities);

	uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
	swapChainCapabilities.formats.resize(surfaceFormatCount);

	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, 
		swapChainCapabilities.formats.data());

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	swapChainCapabilities.presentModes.resize(presentModeCount);

	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, 
		swapChainCapabilities.presentModes.data());;

	return swapChainCapabilities;

}

VkPresentModeKHR Renderer::chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	for (const auto& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR Renderer::chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	for (const auto& format : formats)
	{
		if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	return formats[0];
}

VkExtent2D Renderer::chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{

	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D extent = {
			(uint32_t)width,
			(uint32_t)height
		};

		extent.width = std::max(capabilities.minImageExtent.width, 
			std::min(capabilities.maxImageExtent.width, extent.width));

		extent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, extent.height));
	}

}

bool Renderer::checkDeviceRequirements(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);

	return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && checkQueueFamilies(device);
}

VkResult Renderer::CreateDebugutilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Renderer::createSwapChainImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = swapChainImages[i];
		imageViewInfo.format = swapChainImageFormat;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;

		if (vkCreateImageView(device, &imageViewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("cannot create image views");
	}
}

void Renderer::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("assets/shaders/vert.spv");
	auto fragShaderCode = readFile("assets/shaders/frag.spv");

	VkShaderModule vertexShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.pName = "main";
	
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo stages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo  vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(swapChainExtent.width);
	viewport.height = float(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("cannot create pipeline layout");


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = stages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("cannot create graphics pipeline");

	vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*)code.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("cannot create shader module");

	return shaderModule;
}

std::vector<char> Renderer::readFile(const std::string& path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("cannot open a file");

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> code(fileSize);
	file.seekg(0);
	file.read(code.data(), fileSize);
	file.close();

	return code;
}

void Renderer::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};

	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	
	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("cannot create render pass");


	
}

void Renderer::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(device, &createInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("cannot create framebuffer");
	}
}

void Renderer::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilies.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("cannot create command pool");
}

void Renderer::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = commandBuffers.size();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("cannot create command buffers");

	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("cannot begin command buffer");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("cannot end command buffer");
	}
}

void Renderer::createSyncObjects()
{
	imageAvailableSemaphores.resize(maxFramesInFlight);
	renderingFinishedSemaphores.resize(maxFramesInFlight);
	inFlightFences.resize(maxFramesInFlight);
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < maxFramesInFlight; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderingFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw  std::runtime_error("failed to create synchronization objects for the frame!");
	}
}

void Renderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Renderer::setupDebugOutput()
{
	if (!validationLayersEnabled)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};

	fillDebugMessengerCreateInfo(createInfo);

	if (CreateDebugutilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		throw std::runtime_error("cannot create debug messenger");
	

}

void Renderer::fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugMessage;
}

VkInstance Renderer::instance;

VkPhysicalDevice Renderer::physicalDevice;

VkDevice Renderer::device;

VkDebugUtilsMessengerEXT Renderer::debugMessenger;

VkSurfaceKHR Renderer::surface;

VkQueue Renderer::graphicsQueue;

VkQueue Renderer::presentQueue;

VkSwapchainKHR Renderer::swapChain;

VkFormat Renderer::swapChainImageFormat;

VkExtent2D Renderer::swapChainExtent;

VkPipelineLayout Renderer::pipelineLayout;

VkRenderPass Renderer::renderPass;

VkPipeline Renderer::graphicsPipeline;

VkCommandPool Renderer::commandPool;

Renderer::QueueFamilyIndices Renderer::queueFamilies;

const std::vector<const char*> Renderer::validationLayers = {
		"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> Renderer::requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

GLFWwindow* Renderer::window;

std::vector<VkImage> Renderer::swapChainImages;

std::vector<VkImageView> Renderer::swapChainImageViews;

std::vector<VkFramebuffer> Renderer::swapChainFramebuffers;

std::vector<VkCommandBuffer> Renderer::commandBuffers;

std::vector<VkSemaphore> Renderer::imageAvailableSemaphores;

std::vector<VkSemaphore> Renderer::renderingFinishedSemaphores;

std::vector<VkFence> Renderer::inFlightFences;

std::vector<VkFence> Renderer::imagesInFlight;

uint32_t Renderer::currentFrame = 0;

uint32_t Renderer::maxFramesInFlight = 2;
