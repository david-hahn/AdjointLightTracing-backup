#pragma once
#if defined( _WIN32 )


#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif




#define RVK_USE_INVERSE_Z

#include <deque>
#include <thread>
#include <iostream>
#include <vector>
#include <functional>
#include <cassert>
#include <unordered_map>
#include <cstring>

#if defined( _WIN32 )
#pragma warning (disable: 26812) 
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#elif defined(__APPLE__)
#ifndef VK_USE_PLATFORM_MACOS_MVK
#define VK_USE_PLATFORM_MACOS_MVK
#endif
#elif defined( __linux__)
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define RVK_INLINE inline
#define RVK_BEGIN_NAMESPACE namespace rvk {
#define RVK_END_NAMESPACE }
#define RVK_USE_NAMESPACE using namespace rvk;

RVK_BEGIN_NAMESPACE
class Buffer;
class LogicalDevice;
struct BufferHandle
{
	const LogicalDevice* mDevice;
	const Buffer* mBuffer;
	VkDeviceSize mBufferOffset;
};
RVK_END_NAMESPACE
