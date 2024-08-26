#include <rvk/parts/instance.hpp>
#include "rvk_private.hpp"
#ifdef WIN32
#include <Shlwapi.h>
#elif defined( __linux__ ) || defined(__APPLE__)
#include <dlfcn.h>
#endif


RVK_USE_NAMESPACE

void* InstanceManager::loadLibrary()
{
#ifdef WIN32
	const std::string libName = "vulkan-1.dll";
	Logger::info("...calling LoadLibrary( '" + libName + "' )");
	return LoadLibrary(libName.c_str());
#elif defined(__APPLE__)
	const std::string libName = "libvulkan.dylib";
	Logger::info("...calling dlopen( '" + libName + "' )");
	return dlopen(libName.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#elif defined(__linux__ )
	const std::string libName = "libvulkan.so";
	Logger::info("...calling dlopen( '" + libName + "' )");
	return dlopen(libName.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
}

bool InstanceManager::closeLibrary(void* aLib)
{
#ifdef WIN32
	return FreeLibrary(static_cast<HMODULE>(aLib)) > 0;
#elif defined( __linux__ ) || defined(__APPLE__)
	return dlclose(aLib);
#endif
}

void* InstanceManager::loadFunction(void* aLib, const std::string& aName)
{
#ifdef WIN32
	return (void*) GetProcAddress(static_cast<HMODULE>(aLib), aName.c_str());
#elif defined( __linux__ ) || defined(__APPLE__)
	return dlsym(aLib, aName.c_str());
#endif
}

PFN_vkGetInstanceProcAddr InstanceManager::loadEntryFunction(void* aLib)
{
    return (PFN_vkGetInstanceProcAddr)loadFunction(aLib, "vkGetInstanceProcAddr");
}

void InstanceManager::init(PFN_vkGetInstanceProcAddr f)
{
	vk.GetInstanceProcAddr = f;
#define VK_GLOBAL_LEVEL_FUNCTION( fn ) vk.fn = (PFN_vk##fn) vk.GetInstanceProcAddr( NULL, "vk"#fn ); if(!vk.fn) {Logger::error("Vulkan: Could not load global function: "#fn); return;}
#include <rvk/vk_function_list.inl>
#undef VK_GLOBAL_LEVEL_FUNCTION

	uint32_t count = 0;
	vk.EnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
	mAvailableInstanceExtensions.resize(count);
	vk.EnumerateInstanceExtensionProperties(nullptr, &count, mAvailableInstanceExtensions.data());

	vk.EnumerateInstanceLayerProperties(&count, nullptr);
	mAvailableInstanceLayers.resize(count);
	vk.EnumerateInstanceLayerProperties(&count, mAvailableInstanceLayers.data());
}

const std::vector<VkExtensionProperties>& InstanceManager::getAvailableExtensions() const
{
	return mAvailableInstanceExtensions;
}

bool InstanceManager::isExtensionAvailable(const char* aExtension) const
{
	for (uint32_t i = 0; i < mAvailableInstanceExtensions.size(); i++) {
		if (!strcmp(aExtension, mAvailableInstanceExtensions[i].extensionName)) return true;
	}
	return false;
}

bool InstanceManager::areExtensionsAvailable(const std::vector<const char*>& aExtensions) const
{
	bool available = true;
	for (const auto extension : aExtensions)
	{
		if (!isExtensionAvailable(extension)) {
			Logger::error("requested instance extension is not available: " + std::string(extension));
			available = false;
		}
	}
	return available;
}

const std::vector<VkLayerProperties>& InstanceManager::getAvailableLayers() const
{
	return mAvailableInstanceLayers;
}

bool InstanceManager::isLayerAvailable(const char* aLayer) const
{
	for (uint32_t i = 0; i < mAvailableInstanceLayers.size(); i++) {
		if (!strcmp(aLayer, mAvailableInstanceLayers[i].layerName)) return true;
	}
	return false;
}

bool InstanceManager::areLayersAvailable(const std::vector<const char*>& aLayers) const
{
	bool available = true;
	for (const auto layer : aLayers)
	{
		if (!isLayerAvailable(layer)) {
			Logger::warning("Requested instance layer \"" + std::string(layer) + "\" not available");
			available = false;
		}
	}
	return available;
}

Instance* InstanceManager::createInstance(const std::vector<const char*>& aExtensions,
                                          const std::vector<const char*>& aLayers, 
											const void* pNext)
{
	{
		bool available = areExtensionsAvailable(aExtensions);
		available &= areLayersAvailable(aLayers);
		if (!available) return nullptr;
	}

	VkInstance instance;
	{
		VkApplicationInfo vk_app_info = {};
        vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        vk_app_info.pApplicationName = "";
        vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        vk_app_info.pEngineName = "";
        vk_app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        vk_app_info.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pNext = pNext;
		createInfo.flags = 0;
		createInfo.pApplicationInfo = &vk_app_info;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(aExtensions.size());
		createInfo.ppEnabledExtensionNames = aExtensions.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(aLayers.size());
		createInfo.ppEnabledLayerNames = aLayers.data();
		const VkResult res = InstanceManager::getInstance().vk.CreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
		VK_CHECK(res, "failed to create Instance!");
		if (res != VK_SUCCESS) return nullptr;
	}

	mInstances.push_back(new Instance(instance, vk.GetInstanceProcAddr));
	mInstances.back()->mActiveInstanceExtensions = aExtensions;
	mInstances.back()->mActiveInstanceLayers = aLayers;
	if (!mDefaultInstance) mDefaultInstance = mInstances.back();
	return mInstances.back();
}

std::deque<Instance*>& InstanceManager::getInstances()
{
	return mInstances;
}

void InstanceManager::destroyInstance(Instance*& aInstance)
{
	if (mDefaultInstance == aInstance) aInstance = nullptr;
	const auto it = std::find(mInstances.begin(), mInstances.end(), aInstance);
	if (it != mInstances.end()) {
		for(const auto pd : aInstance->getPhysicalDevices())
		{
			delete pd;
		}
		aInstance->getPhysicalDevices().clear();
		delete aInstance;
		mInstances.erase(it);
		aInstance = nullptr;
	}
}

void InstanceManager::shutdown()
{
	for(const Instance *i : mInstances)
	{
		delete i;
	}
	mInstances.clear();
}

InstanceManager::InstanceManager() : mDefaultInstance(nullptr)
{
}

Instance::Instance(const VkInstance aInstance, const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr) : mInstance(aInstance), mDefaultDevice(nullptr)
{

#define VK_INSTANCE_LEVEL_FUNCTION(fn) vk.fn = (PFN_vk##fn) vkGetInstanceProcAddr( aInstance , "vk"#fn );
#include <rvk/vk_function_list.inl>
#undef VK_INSTANCE_LEVEL_FUNCTION

	uint32_t count = 0;
	vk.EnumeratePhysicalDevices(aInstance, &count, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices;
	physicalDevices.resize(count);
	vk.EnumeratePhysicalDevices(aInstance, &count, physicalDevices.data());

	vk.EnumeratePhysicalDeviceGroups(aInstance, &count, VK_NULL_HANDLE);
	std::vector<VkPhysicalDeviceGroupProperties> physicalDeviceGroupProperties(count, { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES });
	vk.EnumeratePhysicalDeviceGroups(aInstance, &count, physicalDeviceGroupProperties.data());

	mPhysicalDevices.resize(physicalDevices.size());
	for (count = 0; count < physicalDevices.size(); count++) {
		mPhysicalDevices[count] = new PhysicalDevice(this, physicalDevices[count], count, physicalDeviceGroupProperties[count]);
	}
}

bool Instance::isExtensionActive(const char* aExtension) const
{
	for (const auto activeInstanceExtension : mActiveInstanceExtensions)
	{
		if (!strcmp(aExtension, activeInstanceExtension)) return true;
	}
	return false;
}

bool Instance::isLayerActive(const char* aLayer) const
{
	for (const auto activeInstanceLayer : mActiveInstanceLayers)
	{
		if (!strcmp(aLayer, activeInstanceLayer)) return true;
	}
	return false;
}

void Instance::destroy()
{
	for (const PhysicalDevice* d : mPhysicalDevices) {
		for (const LogicalDevice* ld : d->mLogicalDevices) {
			delete ld;
		}
	}
	for (const auto& pair : mSurfaces) {
		vk.DestroySurfaceKHR(mInstance, pair.second.mHandle, VK_NULL_HANDLE);
	}
	vk.DestroyInstance(mInstance, VK_NULL_HANDLE);
	mActiveInstanceExtensions.clear();
	mActiveInstanceLayers.clear();
#define VK_INSTANCE_LEVEL_FUNCTION(fn) vk.fn = nullptr;
#include <rvk/vk_function_list.inl>
#undef VK_INSTANCE_LEVEL_FUNCTION
}

PhysicalDevice* Instance::findPhysicalDevice(const std::vector<const char*>& aDeviceExtensions) const
{
	for (PhysicalDevice* d : mPhysicalDevices) {
		if (d->areExtensionsAvailable(aDeviceExtensions)) return d;
	}
	return nullptr;
}

std::vector<PhysicalDevice*>& Instance::getPhysicalDevices()
{
	return mPhysicalDevices;
}

VkSurfaceKHR Instance::registerSurface(void* aWindowHandle, void* aOtherHandle)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (!aWindowHandle || !isExtensionActive(VK_KHR_SURFACE_EXTENSION_NAME)) return surface;
#ifdef WIN32
	VkWin32SurfaceCreateInfoKHR desc {};
	desc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	desc.pNext = nullptr;
	desc.flags = 0;
	desc.hwnd = static_cast<HWND>(aWindowHandle);
	desc.hinstance = static_cast<HINSTANCE>(aOtherHandle);
	VK_CHECK(vk.CreateWin32SurfaceKHR(mInstance, &desc, nullptr, &surface), "failed to create Win32 Surface!");
#elif defined(__APPLE__)
	VkMacOSSurfaceCreateInfoMVK desc{};
	desc.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
	desc.pNext = nullptr;
	desc.flags = 0;
	desc.pView = aWindowHandle;
    VK_CHECK(vk.CreateMacOSSurfaceMVK(mInstance, &desc, nullptr, &surface), "failed to create MacOS Surface!");
#elif defined( __linux__ )
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
	VkWaylandSurfaceCreateInfoKHR desc{};
	desc.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	desc.pNext = nullptr;
	desc.flags = 0;
	desc.surface = (wl_surface*)aWindowHandle;
	desc.display = (wl_display*)aOtherHandle;
#elif defined( VK_USE_PLATFORM_XLIB_KHR )
	VkXlibSurfaceCreateInfoKHR desc{};
	desc.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	desc.window = *((Window*)aWindowHandle);
	desc.dpy = (Display*)aOtherHandle;
    VK_CHECK(vk.CreateXlibSurfaceKHR(mInstance, &desc, nullptr, &surface), "failed to create xlib Surface!");
#elif defined( VK_USE_PLATFORM_XCB_KHR )
	VkXcbSurfaceCreateInfoKHR desc{};
	desc.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	desc.window = *((xcb_window_t*)aWindowHandle);
	desc.connection = (xcb_connection_t*)aOtherHandle;
    VK_CHECK(vk.CreateXcbSurfaceKHR(mInstance, &desc, nullptr, &surface), "failed to create xcb Surface!");
#endif
#endif
	if (surface) {
		Surface s { surface, std::pair(aWindowHandle, aOtherHandle) };
		mSurfaces.insert(std::pair(aWindowHandle, s));

		for (PhysicalDevice* d : mPhysicalDevices) {
			PhysicalDevice::SurfaceInfos_s infos {};
			infos.surface = surface;
			vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(d->mPhysicalDevice, surface, &infos.surfaceCapabilities);

			uint32_t count = 0;
			vk.GetPhysicalDeviceSurfaceFormatsKHR(d->mPhysicalDevice, surface, &count, nullptr);
			infos.surfaceFormats.resize(count);
			vk.GetPhysicalDeviceSurfaceFormatsKHR(d->mPhysicalDevice, surface, &count, infos.surfaceFormats.data());

			vk.GetPhysicalDeviceSurfacePresentModesKHR(d->mPhysicalDevice, surface, &count, nullptr);
			infos.surfacePresentModes.resize(count);
			vk.GetPhysicalDeviceSurfacePresentModesKHR(d->mPhysicalDevice, surface, &count, infos.surfacePresentModes.data());
			d->mSurfaces.insert(std::pair(aWindowHandle, infos));

			for (PhysicalDevice::QueueFamily_s& f : d->mQueueFamilies) {
				VkBool32 support = VK_FALSE;
				vk.GetPhysicalDeviceSurfaceSupportKHR(d->mPhysicalDevice, f.family_index, surface, &support);
				f.presentation_support.insert(std::pair<void*, bool>(aWindowHandle, support));
			}
		}
	}
	return surface;
}

void Instance::unregisterSurface(VkSurfaceKHR aSurface)
{
	for(auto [fst, snd] : mSurfaces) {
		if(snd.mHandle == aSurface) {
			vk.DestroySurfaceKHR(mInstance, aSurface, nullptr);
			mSurfaces.erase(fst);

			for (const auto d : mPhysicalDevices) {
				d->mSurfaces.erase(fst);
			}
			return;
		}
	}
}

Instance::Surface Instance::getSurface(void* aWindowHandle)
{
	if (!aWindowHandle || (mSurfaces.find(aWindowHandle) == mSurfaces.end())) return {};
	return mSurfaces[aWindowHandle];
}

VkInstance Instance::getHandle() const
{
	return mInstance;
}

void Instance::defaultDevice(LogicalDevice* aDefaultDevice)
{
	mDefaultDevice = aDefaultDevice;
}

LogicalDevice* Instance::defaultDevice() const
{
	return mDefaultDevice;
}
