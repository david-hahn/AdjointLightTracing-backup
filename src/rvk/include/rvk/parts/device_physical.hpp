#pragma once
#include <rvk/parts/rvk_public.hpp>

#include <map>

RVK_BEGIN_NAMESPACE
class Instance;
class LogicalDevice;
class CommandPool;
class Fence;
class Semaphore;
class Queue;
class QueueFamily;
class PhysicalDevice {
public:

	enum Queue {
		GRAPHICS = VK_QUEUE_GRAPHICS_BIT,
		COMPUTE = VK_QUEUE_COMPUTE_BIT,
		TRANSFER = VK_QUEUE_TRANSFER_BIT
	};
	struct QueueFamily_s {
		uint32_t									family_index;
		std::map<void*, bool>						presentation_support;
		bool										graphics_support;
		bool										compute_support;
		bool										transfer_support;
		uint32_t									available_queues;
	};

	Instance*										getInstance() const;
	uint32_t										getIndex() const;
	bool											isExtensionAvailable(const char* aExtension) const;
	bool											areExtensionsAvailable(const std::vector <const char*>& aExtensions) const;

	int												getQueueFamilyCount() const;
	bool											checkQueueFamily(int aFamilyIndex, uint32_t aQueueFlags, const std::vector<void*>& aPresentationSupport = {}) const;
													
	int												getQueueFamilyIndex(uint32_t aQueueFlags, const std::vector<void*>& aPresentationSupport = {}) const;
	const QueueFamily_s&							getQueueFamily(uint32_t aQueueFamilyIndex) const;
	uint32_t										getAvailableQueueCount(uint32_t aQueueFamilyIndex) const;

	VkFormatProperties								getFormatProperties(VkFormat aFormat) const;
	bool											isFormatSupported(VkFormat aFormat, VkImageTiling aTiling, VkFormatFeatureFlags aFeatures) const;

	const VkPhysicalDeviceFeatures&					getFeatures() const;

	int												findMemoryTypeIndex(VkMemoryPropertyFlags aMemoryProperties, uint32_t aMemoryTypeBits) const;
#ifdef VK_KHR_acceleration_structure
	const VkPhysicalDeviceAccelerationStructureFeaturesKHR& getAccelerationStructureFeatures() const;
	const VkPhysicalDeviceAccelerationStructurePropertiesKHR& getAccelerationStructureProperties() const;
#endif
	const VkPhysicalDeviceProperties&				getProperties() const;
	const VkQueueFamilyProperties&					getQueueFamilyProperties(uint32_t aFamilyIdx) const;
	const VkPhysicalDeviceMemoryProperties&			getMemoryProperties() const;
#ifdef VK_KHR_ray_tracing_pipeline
	const VkPhysicalDeviceRayTracingPipelinePropertiesKHR&	getRayTracingPipelineProperties() const;
#endif
#ifdef VK_KHR_surface
	const VkSurfaceCapabilitiesKHR&					getSurfaceCapabilities(void* aWindowHandle);
	const std::vector<VkSurfaceFormatKHR>&			getSurfaceFormats(void* aWindowHandle);
	const std::vector<VkPresentModeKHR>&			getSurfacePresentModes(void* aWindowHandle);
#endif

	
	
	
	
	
	
	
	LogicalDevice*									createLogicalDevice(const std::vector<const char*>& aExtensions, const std::vector<std::pair<uint32_t, std::vector<float>>>& aQueues, void* pNext = nullptr);
	std::deque<LogicalDevice*>&						getLogicalDevices();
	VkPhysicalDevice								getHandle() const;
private:
	friend class Instance;
	friend class LogicalDevice;

													PhysicalDevice(Instance* aInstance, VkPhysicalDevice aPhysicalDevice, uint32_t aPhysicalDeviceIndex, VkPhysicalDeviceGroupProperties aGroupProperties);

	Instance*										mInstance;
	uint32_t										mPhysicalDeviceIndex;
	VkPhysicalDevice								mPhysicalDevice;
	VkPhysicalDeviceGroupProperties					mPhysicalDeviceGroupProperties;
	std::vector<VkExtensionProperties> 				mExtensionProperties;
	std::vector<VkLayerProperties> 					mLayerProperties;

	VkPhysicalDeviceProperties                      mProperties;
	VkPhysicalDeviceFeatures						mFeatures;
	VkPhysicalDeviceMemoryProperties				mMemoryProperties;
	std::vector<VkQueueFamilyProperties> 			mQueueFamilyProperties;

	VkPhysicalDeviceFeatures2						mFeatures2;
#ifdef VK_KHR_acceleration_structure
	VkPhysicalDeviceAccelerationStructureFeaturesKHR mAccelerationStructureFeatures;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR mAccelerationStructureProperties;
#endif
	VkPhysicalDeviceProperties2 					mProperties2;
	std::vector<VkQueueFamilyProperties2> 			mQueueFamilyProperties2;
	VkPhysicalDeviceMemoryProperties2				mMemoryProperties2;
#ifdef VK_KHR_ray_tracing_pipeline
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR	mRayTracingPipelineProperties;
#endif

#ifdef VK_KHR_surface
	struct SurfaceInfos_s {
		VkSurfaceKHR								surface;
		VkSurfaceCapabilitiesKHR					surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> 			surfaceFormats;
		std::vector<VkPresentModeKHR> 				surfacePresentModes;
	};
	std::map<void*, SurfaceInfos_s>					mSurfaces;
#endif
	std::vector<QueueFamily_s>						mQueueFamilies;

	std::deque<LogicalDevice*>						mLogicalDevices;
};
RVK_END_NAMESPACE
