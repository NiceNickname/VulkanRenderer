#include "Renderer.h"
#include <stdexcept>
#include <iostream>
#include <set>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessage(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cout << "message from validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}



void Renderer::init(GLFWwindow* window)
{
	createInstance();
	setupDebugOutput();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
}

void Renderer::draw()
{

}

void Renderer::shutdown()
{
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
		if (requiredFeaturesSupported(device))
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
	VkSwapchainCreateInfoKHR swapChainInfo{};
	

}

bool Renderer::requiredFeaturesSupported(VkPhysicalDevice device)
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

Renderer::QueueFamilyIndices Renderer::queueFamilies;

const std::vector<const char*> Renderer::validationLayers = {
		"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> Renderer::requiredExtensions = {};



