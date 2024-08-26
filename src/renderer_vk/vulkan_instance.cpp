#include <tamashii/renderer_vk/vulkan_instance.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/platform/window.hpp>
#include <fmt/color.h>


#include <imgui.h>
#if defined( _WIN32 )
#include <imgui_impl_win32.h>
#elif defined( __APPLE__ )
#include <imgui_impl_osx.h>

#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )

#elif defined( VK_USE_PLATFORM_XCB_KHR )
#include <tamashii/core/gui/x11/imgui_impl_x11.h>
#endif
#endif

RVK_USE_NAMESPACE
T_USE_NAMESPACE

namespace {
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == messageSeverity) spdlog::error("{}", pCallbackData->pMessage);
		else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT == messageSeverity) spdlog::warn("{}", pCallbackData->pMessage);
		else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT == messageSeverity) spdlog::info("{}", pCallbackData->pMessage);
		else spdlog::debug("{}", pCallbackData->pMessage);
		if (pCallbackData->cmdBufLabelCount)
		{
			for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
			{
				const VkDebugUtilsLabelEXT* label = &pCallbackData->pCmdBufLabels[i];
				spdlog::warn("{} ~", label->pLabelName);
			}
		}

		if (pCallbackData->objectCount)
		{
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const VkDebugUtilsObjectNameInfoEXT* obj = &pCallbackData->pObjects[i];
				spdlog::warn("--- {} {}", obj->pObjectName, static_cast<int32_t>(obj->objectType));
			}
		}
#ifdef VALIDATION_LAYER_THROW_RUNTIME_ERROR
		throw std::runtime_error("Vulkan Validation Layer");
#endif 
		return VK_FALSE;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackPrintf(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		
		if (!pCallbackData->pMessageIdName) return VK_FALSE;
		if (std::string(pCallbackData->pMessageIdName) != "UNASSIGNED-DEBUG-PRINTF") return VK_FALSE;
		std::string str = pCallbackData->pMessage;
		const std::string findStr = "0x92394c89 | ";
		str.replace(0, str.find(findStr) + findStr.size(),
			fmt::format(fmt::fg(fmt::terminal_color::cyan), "shader "));
		spdlog::debug("{}", str);
		return VK_FALSE;
	}
}

VulkanInstance::VulkanInstance() : mVendor{}, mLib{ nullptr }, mInstance{ nullptr }, mDevice{ nullptr }, mSwapchainData{},
mDebugCallback{ nullptr }, mDebugCallbackPrintf{ nullptr }
{
	rvk::Logger::setInfoCallback([](const std::string& aString) { spdlog::info(aString); });
	rvk::Logger::setDebugCallback([](const std::string& aString) { spdlog::debug(aString); });
	rvk::Logger::setWarningCallback([](const std::string& aString) { spdlog::warn(aString); });
	rvk::Logger::setCriticalCallback([](const std::string& aString) { spdlog::critical(aString); });
	rvk::Logger::setErrorCallback([](const std::string& aString) { spdlog::error(aString); });

	mInstanceExtensions = {
#ifndef NDEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined( _WIN32 )
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__APPLE__)
		VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined( VK_USE_PLATFORM_XCB_KHR )
		VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif
#endif
	};

	mValidationLayers = {
#ifdef TAMASHII_VULKAN_VALIDATION_LAYERS
#ifndef NDEBUG
		"VK_LAYER_KHRONOS_validation"
#endif
#endif
	};
}

void VulkanInstance::init(tamashii::Window* aWindow)
{
	const std::vector defaultDeviceExtensions{
#if defined(__APPLE__)
		
#endif
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
#ifdef VK_KHR_swapchain
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#endif
		
		
		
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
		VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
#if defined(WIN32)
		VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME,
#else
		VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
		VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
		VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME,
#endif
		
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
		#ifndef NDEBUG
		
#endif
	};

	const std::vector rtDeviceExtensions{
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
		
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		
		VK_KHR_SPIRV_1_4_EXTENSION_NAME,
		
		VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
	};

	mMainThreadId = std::this_thread::get_id();
	
	if (!((mLib = InstanceManager::loadLibrary()))) throw std::runtime_error("...could not initialize Vulkan");
	spdlog::info("...loading entry function");
	const PFN_vkGetInstanceProcAddr func = InstanceManager::loadEntryFunction(mLib);
	if (!func) throw std::runtime_error("...could not load entry function");

	InstanceManager& instanceManager = InstanceManager::getInstance();
	instanceManager.init(func);

	
	const bool validationLayerAvailable = instanceManager.areLayersAvailable(mValidationLayers);
	if (!validationLayerAvailable) mValidationLayers.clear();
	void* pNext = nullptr;
#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = &debugCallback;
	std::vector enabledValidationFeatures{ VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
	VkValidationFeaturesEXT validationFeatures{};
	validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validationFeatures.enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size());
	validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures.data();
	debugCreateInfo.pNext = &validationFeatures;
	if (validationLayerAvailable) pNext = &validationFeatures;
#endif

	mInstance = instanceManager.createInstance(mInstanceExtensions, mValidationLayers, pNext);
#ifndef NDEBUG
	if (validationLayerAvailable) setupDebugCallback();
#endif

	
	PhysicalDevice* d = mInstance->findPhysicalDevice(defaultDeviceExtensions);
	if (!d) {
		spdlog::error("No suitable Device found!");
		Common::getInstance().shutdown();
	}

	
	mDeviceExtensions.reserve(defaultDeviceExtensions.size() + rtDeviceExtensions.size());
	mDeviceExtensions.insert(mDeviceExtensions.end(), defaultDeviceExtensions.begin(), defaultDeviceExtensions.end());
	bool rtAvailable = true;
	for (const char* ext : rtDeviceExtensions) if (!d->isExtensionAvailable(ext)) rtAvailable = false;
	if (rtAvailable) mDeviceExtensions.insert(mDeviceExtensions.end(), rtDeviceExtensions.begin(), rtDeviceExtensions.end());
	else spdlog::warn("Vulkan: Ray Tracing not supported by device");

	
	constexpr uint32_t queueFlags = PhysicalDevice::Queue::COMPUTE | PhysicalDevice::Queue::TRANSFER;
	const int allQueueIndex = d->getQueueFamilyIndex(PhysicalDevice::Queue::GRAPHICS | queueFlags);
	const int computeTransferQueueIndex = d->getQueueFamilyIndex(queueFlags);

	
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT };
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR pipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	VkPhysicalDeviceFeatures2 device_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
	rvkChain(device_features, dynamicRenderingFeatures, bufferDeviceAddressFeature, accelerationStructureFeatures, pipelineFeatures, atomicFloatFeatures, indexingFeatures);
	mInstance->vk.GetPhysicalDeviceFeatures2(d->getHandle(), &device_features);

#define CHECK_FEATURE(struct, feat) if(!struct.feat) spdlog::warn("Vulkan: Feature '" #feat "' not supported by device")
	CHECK_FEATURE(device_features.features, fillModeNonSolid);
	CHECK_FEATURE(device_features.features, multiDrawIndirect);
	CHECK_FEATURE(device_features.features, drawIndirectFirstInstance);
	CHECK_FEATURE(device_features.features, shaderClipDistance);
	CHECK_FEATURE(device_features.features, geometryShader);
	CHECK_FEATURE(device_features.features, shaderFloat64);
	CHECK_FEATURE(device_features.features, shaderInt64);
	CHECK_FEATURE(indexingFeatures, runtimeDescriptorArray);
	CHECK_FEATURE(indexingFeatures, descriptorBindingVariableDescriptorCount);
	CHECK_FEATURE(indexingFeatures, descriptorBindingPartiallyBound);
	CHECK_FEATURE(atomicFloatFeatures, shaderBufferFloat32AtomicAdd);
	CHECK_FEATURE(atomicFloatFeatures, shaderBufferFloat64AtomicAdd);
	CHECK_FEATURE(dynamicRenderingFeatures, dynamicRendering);
	if (rtAvailable) {
		CHECK_FEATURE(pipelineFeatures, rayTracingPipeline);
		CHECK_FEATURE(pipelineFeatures, rayTracingPipelineTraceRaysIndirect);
		CHECK_FEATURE(pipelineFeatures, rayTraversalPrimitiveCulling);
		CHECK_FEATURE(accelerationStructureFeatures, accelerationStructure);
		CHECK_FEATURE(bufferDeviceAddressFeature, bufferDeviceAddress);
	}
#undef CHECK_FEATURE

	mDevice = d->createLogicalDevice(mDeviceExtensions, { { 0, {1.0f, 1.0f, 1.0f} } }, &device_features);
	const VkPhysicalDeviceProperties deviceProperties = mDevice->getPhysicalDevice()->getProperties();
	if (deviceProperties.vendorID == 0x1002) mVendor = Vendor::AMD;
	else if (deviceProperties.vendorID == 0x10DE) mVendor = Vendor::NVIDIA;
	else if (deviceProperties.vendorID == 0x8086) mVendor = Vendor::INTEL;

	
	mCommandPools.reserve(2);
	mCommandPools.push_back(mDevice->createCommandPool(0));
	mCommandPools.push_back(mDevice->createCommandPool(0));

	mSwapchainData.mCommandPool = mDevice->createCommandPool(0, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	if (aWindow) {
		registerWindow(aWindow);
	}
	else {
		mSwapchainData.mFrames.resize(FRAMES);
		createFakeSwapchain();
	}

	setDefaultState();
}

void VulkanInstance::registerWindow(tamashii::Window* aWindow)
{
	mDevice->waitIdle();
	destroyFakeSwapchain();
	
	mInstance->registerSurface(aWindow->getWindowHandle(), aWindow->getInstanceHandle());
	mSwapchainData.mWindow = aWindow;
	
	{
		mSwapchainData.mCurrentImageIndex = 0;
		mSwapchainData.mPreviousImageIndex = 0;
		mSwapchainData.mSwapchain.emplace(mDevice, aWindow->getWindowHandle());

		auto& sc = mSwapchainData.mSwapchain.value();
		sc.setImageCount(FRAMES);
		if (var::vsync.value() && sc.checkPresentMode(VK_PRESENT_MODE_FIFO_KHR)) sc.setPresentMode(VK_PRESENT_MODE_FIFO_KHR);
		else if (sc.checkPresentMode(VK_PRESENT_MODE_MAILBOX_KHR)) sc.setPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
		else if (sc.checkPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR)) sc.setPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR);
		sc.setSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
		
		const std::vector depthFormats{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT };
		for (const VkFormat& f : depthFormats) {
			if (sc.checkDepthFormat(f)) {
				sc.useDepth(f);
				break;
			}
		}
		sc.finish();
		mSwapchainData.mFrames.resize(sc.getImageCount());
		spdlog::info("...swapchain created ({}x{})", sc.getExtent().width, sc.getExtent().height);
	}

	for (SwapchainFrameData& imgData : mSwapchainData.mFrames)
	{
		if (mSwapchainData.mSwapchain) {
			imgData.mNextImageAvailableSemaphores = mDevice->createSemaphore();
			imgData.mRenderFinishedSemaphores = mDevice->createSemaphore();
		}
		if (!imgData.mInFlightFences) imgData.mInFlightFences = mDevice->createFence(VK_FENCE_CREATE_SIGNALED_BIT);
		if (!imgData.mCommandBuffer) imgData.mCommandBuffer = mSwapchainData.mCommandPool->allocCommandBuffers(1).front();
	}
	
	SingleTimeCommand stc(mSwapchainData.mCommandPool, mDevice->getQueue(0, 0));
	stc.begin();
	for (uint32_t i = 0; i < mSwapchainData.mSwapchain->getImageCount(); i++) {
		mSwapchainData.mFrames[i].mColor = &mSwapchainData.mSwapchain->getImages()[i];
		mSwapchainData.mFrames[i].mDepth = &mSwapchainData.mSwapchain->getDepthImages()[i];
		mSwapchainData.mFrames[i].mColor->CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		mSwapchainData.mFrames[i].mDepth->CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	stc.end();

	setDefaultState();

	mGuiBackend = std::make_unique<GuiBackend>(*this, aWindow->getWindowHandle(), static_cast<uint32_t>(mSwapchainData.mFrames.size()), tamashii::var::default_font.value());
}

void VulkanInstance::unregisterWindow(tamashii::Window* aWindow)
{
	mDevice->waitIdle();
	mGuiBackend.reset();
	for (SwapchainFrameData& imgData : mSwapchainData.mFrames) {
		if (mSwapchainData.mSwapchain) {
			mDevice->destroySemaphore(imgData.mNextImageAvailableSemaphores);
			mDevice->destroySemaphore(imgData.mRenderFinishedSemaphores);
		}
	}
	mSwapchainData.mSwapchain.reset();
	mInstance->unregisterSurface(mInstance->getSurface(aWindow->getWindowHandle()).mHandle);
	mSwapchainData.mCurrentImageIndex = 0;
	mSwapchainData.mPreviousImageIndex = 0;
	mSwapchainData.mWindow = nullptr;
	createFakeSwapchain();
}

void VulkanInstance::createFakeSwapchain()
{
	mDevice->waitIdle();

	for (SwapchainFrameData& imgData : mSwapchainData.mFrames) {
		if (!mSwapchainData.mSwapchain) {
			if (!imgData.mInFlightFences) imgData.mInFlightFences = mDevice->createFence(VK_FENCE_CREATE_SIGNALED_BIT);
			if (!imgData.mCommandBuffer) imgData.mCommandBuffer = mSwapchainData.mCommandPool->allocCommandBuffers(1).front();

			imgData.mColor = new rvk::Image(mDevice);
			imgData.mColor->createImage2D(var::render_size.value().at(0), var::render_size.value().at(1), COLOR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
			imgData.mDepth = new rvk::Image(mDevice);
			imgData.mDepth->createImage2D(var::render_size.value().at(0), var::render_size.value().at(1), DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		}
	}

	SingleTimeCommand stc(mSwapchainData.mCommandPool, mDevice->getQueue(0, 0));
	stc.begin();
	for (const SwapchainFrameData& imgData : mSwapchainData.mFrames) {
		imgData.mColor->CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		imgData.mDepth->CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
	stc.end();
}

void VulkanInstance::destroyFakeSwapchain()
{
	mDevice->waitIdle();
	for (SwapchainFrameData& imgData : mSwapchainData.mFrames) {
		if (!mSwapchainData.mSwapchain) {
			if (imgData.mColor) {
				delete imgData.mColor;
				imgData.mColor = nullptr;
			}
			if (imgData.mDepth) {
				delete imgData.mDepth;
				imgData.mDepth = nullptr;
			}
		}
	}
	mSwapchainData.mCurrentImageIndex = 0;
	mSwapchainData.mPreviousImageIndex = 0;
}

void VulkanInstance::printDeviceInfos() const
{
	uint32_t count = 0;
	const char* vendorName = "unknown";
	const VkPhysicalDeviceProperties deviceProperties = mDevice->getPhysicalDevice()->getProperties();
	if (deviceProperties.vendorID == 0x1002) vendorName = "Advanced Micro Devices, Inc.";
	else if (deviceProperties.vendorID == 0x10DE) vendorName = "NVIDIA Corporation";
	else if (deviceProperties.vendorID == 0x8086) vendorName = "Intel Corporation";
	const uint32_t major = VK_VERSION_MAJOR(deviceProperties.apiVersion);
	const uint32_t minor = VK_VERSION_MINOR(deviceProperties.apiVersion);
	const uint32_t patch = VK_VERSION_PATCH(deviceProperties.apiVersion);

	spdlog::info("Device {}", count);
	spdlog::info("\tVendor: {}", vendorName);
	spdlog::info("\tGPU: {}", deviceProperties.deviceName);
	spdlog::info("\tVulkan Version: {}.{}.{}", major, minor, patch);
}

void VulkanInstance::destroy()
{
	mGuiBackend.reset();
	mSwapchainData.mSwapchain.reset();
	mSwapchainData.mFrames.clear();
	mSwapchainData = {};
	mCommandPools.clear();
	InstanceManager& instanceManager = InstanceManager::getInstance();
	instanceManager.destroyInstance(mInstance);
	mDevice = nullptr;
	InstanceManager::closeLibrary(mLib);
}

void VulkanInstance::setResizeCallback(const std::function<void(uint32_t, uint32_t, rvk::Swapchain&)>& aCallback)
{
	mCallbackSwapchainResize = aCallback;
}

void VulkanInstance::recreateSwapchain(const uint32_t aWidth, const uint32_t aHeight)
{
	if (!mSwapchainData.mSwapchain) return;
	mDevice->waitIdle();
	mSwapchainData.mSwapchain->recreateSwapchain();
	SingleTimeCommand stc(mSwapchainData.mCommandPool, mDevice->getQueue(0, 0));
	stc.begin();
	for (rvk::Image& img : mSwapchainData.mSwapchain->getImages()) img.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	for (rvk::Image& img : mSwapchainData.mSwapchain->getDepthImages()) img.CMD_TransitionImage(stc.buffer(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	stc.end();
}

void VulkanInstance::beginFrame()
{
	SwapchainData& sd = mSwapchainData;
	const SwapchainFrameData& oldFrame = sd.mFrames[sd.mCurrentImageIndex];
	while (VK_TIMEOUT == oldFrame.mInFlightFences->wait()) {}
	
	if (sd.mSwapchain) {
		sd.mSwapchain->acquireNextImage(sd.mFrames[sd.mCurrentImageIndex].mNextImageAvailableSemaphores);
		sd.mCurrentImageIndex = sd.mSwapchain->getCurrentIndex();
		sd.mPreviousImageIndex = sd.mSwapchain->getPreviousIndex();
	}
	else {
		const auto imageCount = static_cast<uint32_t>(sd.mFrames.size());
		sd.mPreviousImageIndex = sd.mCurrentImageIndex;
		sd.mCurrentImageIndex = (sd.mCurrentImageIndex + 1) % imageCount;
	}

	
	const SwapchainFrameData& newFrame = sd.mFrames[sd.mCurrentImageIndex];
	newFrame.mInFlightFences->reset();
	newFrame.mCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void VulkanInstance::endFrame()
{
	SwapchainData& sd = mSwapchainData;
	const SwapchainFrameData& oldFrame = sd.mFrames[sd.mPreviousImageIndex];
	const SwapchainFrameData& curFrame = sd.mFrames[sd.mCurrentImageIndex];

	curFrame.mCommandBuffer->end();
	if (sd.mSwapchain) {
		
		const std::vector<std::pair<Semaphore*, VkPipelineStageFlags>> waitSemaphore{
			{ oldFrame.mNextImageAvailableSemaphores, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } };
		const std::vector signalSemaphores{ curFrame.mRenderFinishedSemaphores };
		mDevice->getQueue(0, 0)->submitCommandBuffers({ curFrame.mCommandBuffer },
			curFrame.mInFlightFences, waitSemaphore, signalSemaphores);
		
		const VkResult r = mDevice->getQueue(0, 0)->present({ &sd.mSwapchain.value() }, signalSemaphores);
		if (r == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain(0, 0);
			const VkExtent2D extent = sd.mSwapchain->getExtent();
			mCallbackSwapchainResize(extent.width, extent.height, sd.mSwapchain.value());
		}
		else VK_CHECK(r, "failed to Queue Present!");
	}
	else {
		mDevice->getQueue(0, 0)->submitCommandBuffers({ curFrame.mCommandBuffer }, curFrame.mInFlightFences);
	}
}

GuiBackend* VulkanInstance::gui() const
{
	return mGuiBackend.get();
}

void VulkanInstance::setupDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = &debugCallback;
	
	VkDebugUtilsMessengerCreateInfoEXT debugPrintfCreateInfo{};
	debugPrintfCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugPrintfCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; 
	debugPrintfCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debugPrintfCreateInfo.pfnUserCallback = &debugCallbackPrintf;
#ifndef NDEBUG
	VK_CHECK(mInstance->vk.CreateDebugUtilsMessengerEXT(mInstance->getHandle(), &debugCreateInfo, VK_NULL_HANDLE, &mDebugCallback), "failed to set up debug callback!");
	VK_CHECK(mInstance->vk.CreateDebugUtilsMessengerEXT(mInstance->getHandle(), &debugPrintfCreateInfo, VK_NULL_HANDLE, &mDebugCallbackPrintf), "failed to set up debug callback!");
#endif
}

void VulkanInstance::setDefaultState()
{
	
	RPipeline::global_render_state = {};
	RPipeline::global_render_state.dynamicStates.viewport = true;
	RPipeline::global_render_state.dynamicStates.scissor = true;

	if (mSwapchainData.mSwapchain) {
		const Swapchain& sc = mSwapchainData.mSwapchain.value();
		const VkExtent2D extent = sc.getExtent();
		RPipeline::global_render_state.scissor.extent.width = extent.width;
		RPipeline::global_render_state.scissor.extent.height = extent.height;
		RPipeline::global_render_state.viewport.height = static_cast<float>(extent.height);
		RPipeline::global_render_state.viewport.width = static_cast<float>(extent.width);
		RPipeline::global_render_state.renderpass = sc.getRenderpassClear()->getHandle();
	}
	else
	{
		const auto extent = var::varToVec(var::render_size);
		RPipeline::global_render_state.scissor.extent.width = extent.x;
		RPipeline::global_render_state.scissor.extent.height = extent.y;
		RPipeline::global_render_state.viewport.height = static_cast<float>(extent.x);
		RPipeline::global_render_state.viewport.width = static_cast<float>(extent.y);
	}
	
	RPipeline::global_render_state.colorBlend[0].blendEnable = VK_TRUE;
	RPipeline::global_render_state.colorBlend[0].colorBlendOp = VK_BLEND_OP_ADD;
	RPipeline::global_render_state.colorBlend[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	RPipeline::global_render_state.colorBlend[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	RPipeline::global_render_state.colorBlend[0].alphaBlendOp = VK_BLEND_OP_ADD;
	RPipeline::global_render_state.colorBlend[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	RPipeline::global_render_state.colorBlend[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	RPipeline::global_render_state.colorBlend[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	
	RPipeline::global_render_state.dsBlend.depthTestEnable = VK_TRUE;
	RPipeline::global_render_state.dsBlend.depthWriteEnable = VK_TRUE;
#ifdef RVK_USE_INVERSE_Z
	
	RPipeline::global_render_state.dsBlend.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
	RPipeline::global_render_state.viewport.minDepth = 1;
	RPipeline::global_render_state.viewport.maxDepth = 0;
#else
	
	RPipeline::global_render_state.dsBlend.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
#endif
}
