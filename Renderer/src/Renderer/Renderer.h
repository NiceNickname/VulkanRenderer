#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <vector>
#include <optional>

class Renderer
{
public:
	static void init(GLFWwindow* window);
	static void draw();
	static void shutdown();

private:

	struct swapChainCapabilities
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};


	static void createInstance();
	static void createSurface(GLFWwindow* window);
	static bool allLayersSupported();
	static void pickPhysicalDevice();
	static bool checkQueueFamilies(VkPhysicalDevice device);
	static void createLogicalDevice();
	static void createSwapChain();
	static swapChainCapabilities getSwapChainCapabilities();
	static VkPresentModeKHR chooseSwapChainPresentMode();
	static VkSurfaceFormatKHR chooseSwapChainformat();
	static VkExtent2D chooseSwapChainExtent();
	static bool requiredFeaturesSupported(VkPhysicalDevice device);
	static void setupDebugOutput();
	static void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, 
		const VkAllocationCallbacks* pAllocator);
	static VkResult CreateDebugutilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);


private:
	static VkInstance instance;
	static VkPhysicalDevice physicalDevice;
	static VkDevice device;
	static VkDebugUtilsMessengerEXT debugMessenger;
	static VkSurfaceKHR surface;
	static VkQueue graphicsQueue;
	static VkQueue presentQueue;
	static VkSwapchainKHR swapChain;
	
	struct  QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
	};

	

	static QueueFamilyIndices queueFamilies;

#ifdef NDEBUG
	static const bool validationLayersEnabled = false;
#else
	static const bool validationLayersEnabled = true;
#endif

	static const std::vector<const char*> validationLayers;
	static const std::vector<const char*> requiredExtensions;

};