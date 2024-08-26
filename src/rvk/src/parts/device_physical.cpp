#include <rvk/parts/device_physical.hpp>
#include "rvk_private.hpp"

RVK_USE_NAMESPACE

Instance* PhysicalDevice::getInstance() const
{
	return mInstance;
}

uint32_t PhysicalDevice::getIndex() const
{
	return mPhysicalDeviceIndex;
}

bool PhysicalDevice::isExtensionAvailable(const char* aExtension) const
{
	for (uint32_t i = 0; i < mExtensionProperties.size(); i++) {
		if (!strcmp(aExtension, mExtensionProperties[i].extensionName)) return true;
	}
	return false;
}

bool PhysicalDevice::areExtensionsAvailable(const std::vector<const char*>& aExtensions) const
{
	bool compatible = true;
	for (auto extension : aExtensions)
	{
		if (!isExtensionAvailable(extension)) {
			compatible = false;
			Logger::error("Vulkan: required device extension is not available: " + std::string(extension));
			break;
		}
	}
		
	if (compatible) {
		return true;
	}
	return false;
}

int PhysicalDevice::getQueueFamilyCount() const
{
	return static_cast<int>(mQueueFamilies.size());
}

bool PhysicalDevice::checkQueueFamily(const int aFamilyIndex, const uint32_t aQueueFlags,
	const std::vector<void*>& aPresentationSupport) const
{
	const bool reqGraphics = aQueueFlags & PhysicalDevice::Queue::GRAPHICS;
	const bool reqCompute = aQueueFlags & PhysicalDevice::Queue::COMPUTE;
	const bool reqTransfer = aQueueFlags & PhysicalDevice::Queue::TRANSFER;

	bool compatible = true;
	for (const auto& surface_id : aPresentationSupport)
	{
		auto pos = mQueueFamilies[aFamilyIndex].presentation_support.find(surface_id);
		if (pos == mQueueFamilies[aFamilyIndex].presentation_support.end()) return false;
		compatible &= pos->second;
	}
	if (reqGraphics) compatible &= mQueueFamilies[aFamilyIndex].graphics_support;
	if (reqCompute) compatible &= mQueueFamilies[aFamilyIndex].compute_support;
	if (reqTransfer) compatible &= mQueueFamilies[aFamilyIndex].transfer_support;
	return compatible;
}

int PhysicalDevice::getQueueFamilyIndex(const uint32_t aQueueFlags, const std::vector<void*>& aPresentationSupport) const
{
	const bool reqGraphics = aQueueFlags & PhysicalDevice::Queue::GRAPHICS;
	const bool reqCompute = aQueueFlags & PhysicalDevice::Queue::COMPUTE;
	const bool reqTransfer = aQueueFlags & PhysicalDevice::Queue::TRANSFER;

	int bestFamily = -1;
	int bestScore = 0;
	for (uint32_t i = 0; i < mQueueFamilies.size(); i++) {
		
		uint8_t score = static_cast<uint8_t>(mQueueFamilies[i].graphics_support) + static_cast<uint8_t>(mQueueFamilies[i].compute_support) + static_cast<uint8_t>(mQueueFamilies[i].transfer_support);
		bool compatible = true;
		for (const auto& surfaceId : aPresentationSupport)
		{
			auto pos = mQueueFamilies[i].presentation_support.find(surfaceId);
			if (pos == mQueueFamilies[i].presentation_support.end()) return -1;
			score += static_cast<uint8_t>(pos->second);
			compatible &= pos->second;
		}
		if (reqGraphics) compatible &= mQueueFamilies[i].graphics_support;
		if (reqCompute) compatible &= mQueueFamilies[i].compute_support;
		if (reqTransfer) compatible &= mQueueFamilies[i].transfer_support;

		
		if(compatible)
		{
			
			if (bestFamily == -1) {
				bestFamily = static_cast<int>(i);
				bestScore = score;
			}
			
			else if(score < bestScore)
			{
				bestFamily = static_cast<int>(i);
				bestScore = score;
			}
		}
	}
	return bestFamily;
}

const PhysicalDevice::QueueFamily_s& PhysicalDevice::getQueueFamily(const uint32_t aQueueFamilyIndex) const
{
	return mQueueFamilies[aQueueFamilyIndex];
}

uint32_t PhysicalDevice::getAvailableQueueCount(const uint32_t aQueueFamilyIndex) const
{
	return static_cast<int>(mQueueFamilyProperties[aQueueFamilyIndex].queueCount);
}

VkFormatProperties PhysicalDevice::getFormatProperties(const VkFormat aFormat) const
{
	VkFormatProperties props;
	mInstance->vk.GetPhysicalDeviceFormatProperties(mPhysicalDevice, aFormat, &props);
	return props;
}

bool PhysicalDevice::isFormatSupported(const VkFormat aFormat, const VkImageTiling aTiling, const VkFormatFeatureFlags aFeatures) const
{
	VkFormatProperties props;
	mInstance->vk.GetPhysicalDeviceFormatProperties(mPhysicalDevice, aFormat, &props);
	if (aTiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & aFeatures) == aFeatures) {
		return true;
	}
	if (aTiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & aFeatures) == aFeatures) {
		return true;
	}
	return false;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::getFeatures() const
{
	return mFeatures;
}


int PhysicalDevice::findMemoryTypeIndex(const VkMemoryPropertyFlags aMemoryProperties, const uint32_t aMemoryTypeBits) const
{
	const uint32_t memoryCount = mMemoryProperties.memoryTypeCount;
	for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex) {
		const uint32_t memoryTypeBits = (1 << memoryIndex);
		const bool isRequiredMemoryType = aMemoryTypeBits & memoryTypeBits;

		const VkMemoryPropertyFlags properties =
			mMemoryProperties.memoryTypes[memoryIndex].propertyFlags;
		const bool hasRequiredProperties =
			(properties & aMemoryProperties) == aMemoryProperties;

		if (isRequiredMemoryType && hasRequiredProperties)
			return static_cast<int>(memoryIndex);
	}

	
	return -1;
}

#ifdef VK_KHR_acceleration_structure
const VkPhysicalDeviceAccelerationStructureFeaturesKHR& PhysicalDevice::getAccelerationStructureFeatures() const
{
	return mAccelerationStructureFeatures;
}

const VkPhysicalDeviceAccelerationStructurePropertiesKHR& PhysicalDevice::getAccelerationStructureProperties() const
{
	return mAccelerationStructureProperties;
}
#endif

const VkPhysicalDeviceProperties& PhysicalDevice::getProperties() const
{
	return mProperties;
}

const VkQueueFamilyProperties& PhysicalDevice::getQueueFamilyProperties(const uint32_t aFamilyIdx) const
{
	return mQueueFamilyProperties[aFamilyIdx];
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::getMemoryProperties() const
{
	return mMemoryProperties;
}

#ifdef VK_KHR_ray_tracing_pipeline
const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& PhysicalDevice::getRayTracingPipelineProperties() const
{
	return mRayTracingPipelineProperties;
}
#endif

#ifdef VK_KHR_surface
const VkSurfaceCapabilitiesKHR& PhysicalDevice::getSurfaceCapabilities(void* aWindowHandle)
{
	SurfaceInfos_s& si = mSurfaces[aWindowHandle];
	mInstance->vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, si.surface, &si.surfaceCapabilities);
	return si.surfaceCapabilities;
}

const std::vector<VkSurfaceFormatKHR>& PhysicalDevice::getSurfaceFormats(void* aWindowHandle)
{
	return mSurfaces[aWindowHandle].surfaceFormats;
}

const std::vector<VkPresentModeKHR>& PhysicalDevice::getSurfacePresentModes(void* aWindowHandle)
{
	return mSurfaces[aWindowHandle].surfacePresentModes;
}
#endif

LogicalDevice* PhysicalDevice::createLogicalDevice(const std::vector<const char*>& aExtensions, const std::vector<std::pair<uint32_t, std::vector<float>>>& aQueues, void* pNext)
{
	for (const auto& queue_family : aQueues) {
		if (queue_family.first >= mQueueFamilyProperties.size()) {
			Logger::error("Not queue family '" + std::to_string(queue_family.first) + "'");
			return nullptr;
		}
		if (queue_family.second.size() > mQueueFamilyProperties[queue_family.first].queueCount) {
			Logger::error("Not enough queues for queue family '" + std::to_string(queue_family.first) + "'");
			return nullptr;
		}
	}
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(mQueueFamilies.size());
	
	std::vector<std::vector<float>> queuePriorities;
	queuePriorities.resize(mQueueFamilies.size());
	for (const auto& qf : mQueueFamilies) queuePriorities[qf.family_index].reserve(qf.available_queues);
	
	for (const auto& qf : aQueues) {
		const uint32_t& fidx = qf.first;
		for (const float& qp : qf.second) {
			queuePriorities[fidx].push_back(qp);
		}

		VkDeviceQueueCreateInfo qci = {};
		qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qci.queueFamilyIndex = fidx;
		qci.queueCount = static_cast<uint32_t>(queuePriorities[fidx].size());
		qci.pQueuePriorities = queuePriorities[fidx].data();
		queueCreateInfos.push_back(qci);
	}
	
	VkDevice device = VK_NULL_HANDLE;
	VkDeviceCreateInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	desc.pNext = pNext;
	desc.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	desc.pQueueCreateInfos = toArrayPointer(queueCreateInfos);
    desc.enabledExtensionCount = static_cast<uint32_t>(aExtensions.size());
    desc.ppEnabledExtensionNames = aExtensions.empty() ? nullptr : aExtensions.data();
	const VkResult res = mInstance->vk.CreateDevice(mPhysicalDevice, &desc, nullptr, &device);
	VK_CHECK(res, "failed to create logical device!");
	if (res != VK_SUCCESS) return nullptr;

	const auto ld = new LogicalDevice(this, device);
	ld->mActiveDeviceExtensions = aExtensions;
	ld->mQueueFamilies.reserve(mQueueFamilies.size());
	
	for(uint32_t i = 0; i < queuePriorities.size(); i++)
	{
		ld->mQueueFamilies.emplace_back(ld, i, queuePriorities[i]);
	}

	mLogicalDevices.push_back(ld);
	if (!mInstance->defaultDevice()) mInstance->defaultDevice(ld);
	return ld;
}

std::deque<LogicalDevice*>& PhysicalDevice::getLogicalDevices()
{
	return mLogicalDevices;
}

VkPhysicalDevice PhysicalDevice::getHandle() const
{
	return mPhysicalDevice;
}

PhysicalDevice::PhysicalDevice(Instance* aInstance, VkPhysicalDevice aPhysicalDevice, uint32_t aPhysicalDeviceIndex, VkPhysicalDeviceGroupProperties aGroupProperties) :
	mInstance{ aInstance }, mPhysicalDeviceIndex{ aPhysicalDeviceIndex }, mPhysicalDevice{ aPhysicalDevice }, mPhysicalDeviceGroupProperties{},
	mProperties{}, mFeatures{}, mMemoryProperties{}, mFeatures2{}, mAccelerationStructureFeatures{},
	mAccelerationStructureProperties{}, mProperties2{}, mMemoryProperties2{}, mRayTracingPipelineProperties{}
{
	aInstance->vk.GetPhysicalDeviceProperties(mPhysicalDevice, &mProperties);
	const uint32_t major = VK_VERSION_MAJOR(mProperties.apiVersion);
	const uint32_t minor = VK_VERSION_MINOR(mProperties.apiVersion);
	const uint32_t patch = VK_VERSION_PATCH(mProperties.apiVersion);
	aInstance->vk.GetPhysicalDeviceFeatures(mPhysicalDevice, &mFeatures);

	uint32_t count = 0;
	aInstance->vk.EnumerateDeviceExtensionProperties(mPhysicalDevice, VK_NULL_HANDLE, &count, VK_NULL_HANDLE);
	mExtensionProperties.resize(count);
	aInstance->vk.EnumerateDeviceExtensionProperties(mPhysicalDevice, VK_NULL_HANDLE, &count, mExtensionProperties.data());

	aInstance->vk.EnumerateDeviceLayerProperties(mPhysicalDevice, &count, VK_NULL_HANDLE);
	mLayerProperties.resize(count);
	aInstance->vk.EnumerateDeviceLayerProperties(mPhysicalDevice, &count, mLayerProperties.data());

	aInstance->vk.GetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);

	aInstance->vk.GetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &count, VK_NULL_HANDLE);
	mQueueFamilyProperties.resize(count);
	aInstance->vk.GetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &count, mQueueFamilyProperties.data());

	if (minor >= 2) {
		VkPhysicalDeviceFeatures2 features2;
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		mAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		features2.pNext = &mAccelerationStructureFeatures;
		VkPhysicalDeviceProperties2 properties2;
		properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		mRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		properties2.pNext = &mRayTracingPipelineProperties;
		mAccelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
		mRayTracingPipelineProperties.pNext = &mAccelerationStructureProperties;

		aInstance->vk.GetPhysicalDeviceFeatures2(mPhysicalDevice, &features2);
		aInstance->vk.GetPhysicalDeviceProperties2(mPhysicalDevice, &properties2);

	
	
	

	
	
	
	}

	mQueueFamilies.resize(mQueueFamilyProperties.size());
	for (uint32_t i = 0; i < mQueueFamilies.size(); i++) {
		mQueueFamilies[i].family_index = i;
		mQueueFamilies[i].graphics_support = isBitSet<VkQueueFlags>(mQueueFamilyProperties[i].queueFlags, VK_QUEUE_GRAPHICS_BIT);
		mQueueFamilies[i].compute_support = isBitSet<VkQueueFlags>(mQueueFamilyProperties[i].queueFlags, VK_QUEUE_COMPUTE_BIT);
		mQueueFamilies[i].transfer_support = isBitSet<VkQueueFlags>(mQueueFamilyProperties[i].queueFlags, VK_QUEUE_TRANSFER_BIT);
		mQueueFamilies[i].available_queues = mQueueFamilyProperties[i].queueCount;
	}
}

