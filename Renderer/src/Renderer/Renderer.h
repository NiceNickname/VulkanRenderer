#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <vector>
#include <optional>
#include <string>

class Renderer
{
public:
	static void init(GLFWwindow* windowPointer);
	static void draw();
	static void shutdown();

private:

	struct SwapChainCapabilities
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

	static SwapChainCapabilities getSwapChainCapabilities();
	static VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	static VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	static VkExtent2D chooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	static bool checkDeviceRequirements(VkPhysicalDevice device);
	static void setupDebugOutput();
	static void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, 
		const VkAllocationCallbacks* pAllocator);
	static VkResult CreateDebugutilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);


	static void createSwapChainImageViews();
	static void createGraphicsPipeline();
	static VkShaderModule createShaderModule(const std::vector<char>& code);
	static std::vector<char> readFile(const std::string& path);
	static void createRenderPass();
	static void createFramebuffers();
	static void createCommandPool();
	static void createCommandBuffers();
	static void createSyncObjects();

private:
	static GLFWwindow* window;

	static VkInstance instance;
	static VkPhysicalDevice physicalDevice;
	static VkDevice device;
	static VkDebugUtilsMessengerEXT debugMessenger;
	static VkSurfaceKHR surface;
	static VkQueue graphicsQueue;
	static VkQueue presentQueue;

	static VkSwapchainKHR swapChain;
	static VkFormat swapChainImageFormat;
	static VkExtent2D swapChainExtent;
	static VkPipelineLayout pipelineLayout;
	static VkRenderPass renderPass;
	static VkPipeline graphicsPipeline;
	static VkCommandPool commandPool;
	
	struct  QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
	};

	static std::vector<VkImage> swapChainImages;
	static std::vector<VkImageView> swapChainImageViews;
	static std::vector<VkFramebuffer> swapChainFramebuffers;
	static std::vector<VkCommandBuffer> commandBuffers;

	static std::vector<VkSemaphore> imageAvailableSemaphores;
	static std::vector<VkSemaphore> renderingFinishedSemaphores;
	static std::vector<VkFence> inFlightFences;
	static std::vector<VkFence> imagesInFlight;

	static uint32_t currentFrame;
	static uint32_t maxFramesInFlight;
	static QueueFamilyIndices queueFamilies;

#ifdef NDEBUG
	static const bool validationLayersEnabled = false;
#else
	static const bool validationLayersEnabled = true;
#endif

	static const std::vector<const char*> validationLayers;
	static const std::vector<const char*> requiredExtensions;

};