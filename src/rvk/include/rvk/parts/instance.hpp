#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
/*
** --SurfaceHandles--
** win:		(p1: HWND,			p2: HINSTANCE/HMODULE from vk dll)
** macOS:	(p1: NSView,        p2: NULL)
** wayland: (p1: wl_surface,    p2: wl_display)
** xcb:     (p1: xcb_window_t,  p2: xcb_connection_t)
** X11:     (p1: Window,        p2: Display)
*/
typedef std::pair<void*, void*> SurfaceHandles;

class LogicalDevice;
class PhysicalDevice;
class Instance;
class InstanceManager {
public:
	static InstanceManager&							getInstance()
													{
														static InstanceManager instance;
														return instance;
													}
	static Instance*								getDefaultInstance()
													{
														return getInstance().mDefaultInstance;
													}

													
	static void*									loadLibrary();
	static bool										closeLibrary(void *aLib);
	static void*									loadFunction(void* aLib, const std::string& aName );
	static PFN_vkGetInstanceProcAddr				loadEntryFunction(void* aLib);

	void											init(PFN_vkGetInstanceProcAddr f);

	const std::vector<VkExtensionProperties>&		getAvailableExtensions() const;
	bool											isExtensionAvailable(const char* aExtension) const;
	bool											areExtensionsAvailable(const std::vector <const char*>& aExtensions) const;

	const std::vector<VkLayerProperties>&			getAvailableLayers() const;
	bool											isLayerAvailable(const char* aLayer) const;
	bool											areLayersAvailable(const std::vector <const char*>& aLayers) const;

													
													
	Instance*										createInstance(const std::vector<const char*>& aExtensions, const std::vector<const char*>& aLayers, const void* pNext = nullptr);
	std::deque<Instance*>&							getInstances();
	void											destroyInstance(Instance*& aInstance);

	void											shutdown();

	struct vk_functions {
#define VK_EXPORT_FUNCTION(fn) PFN_vk##fn fn = nullptr;
#define VK_GLOBAL_LEVEL_FUNCTION(fn) PFN_vk##fn fn = nullptr;
#include <rvk/vk_function_list.inl>
#undef VK_EXPORT_FUNCTION
#undef VK_GLOBAL_LEVEL_FUNCTION
	} vk;
private:
													InstanceManager();

	std::vector<VkExtensionProperties>				mAvailableInstanceExtensions;
	std::vector<VkLayerProperties>					mAvailableInstanceLayers;
	std::deque<Instance*>							mInstances;
	Instance*										mDefaultInstance; 
};

class Instance {
public:
	struct Surface
	{
		VkSurfaceKHR								mHandle;
		SurfaceHandles								mSurfaceHandles;
	};

													Instance(VkInstance aInstance, PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr);
													~Instance() = default;

	bool											isExtensionActive(const char* aExtension) const;
	bool											isLayerActive(const char* aLayer) const;
													
	void											destroy();

	
	PhysicalDevice*									findPhysicalDevice(const std::vector <const char*>& aDeviceExtensions) const;
	std::vector<PhysicalDevice*>&					getPhysicalDevices();

	VkSurfaceKHR									registerSurface(void* aWindowHandle, void* aOtherHandle);
	void											unregisterSurface(VkSurfaceKHR aSurface);
	Surface											getSurface(void* aWindowHandle);
	VkInstance										getHandle() const;

	void											defaultDevice(LogicalDevice* aDefaultDevice);
	LogicalDevice*									defaultDevice() const;

	struct vk_functions {

#define VK_INSTANCE_LEVEL_FUNCTION(fn) PFN_vk##fn fn = nullptr;
#include <rvk/vk_function_list.inl>
#undef VK_INSTANCE_LEVEL_FUNCTION
	} vk;

private:
	friend class InstanceManager;

	std::vector<const char*>						mActiveInstanceExtensions;
	std::vector<const char*>						mActiveInstanceLayers;
	VkInstance										mInstance;

	std::unordered_map<void*, Surface>				mSurfaces;
	std::vector<PhysicalDevice*> 					mPhysicalDevices;
	LogicalDevice*									mDefaultDevice;	
};

RVK_END_NAMESPACE
