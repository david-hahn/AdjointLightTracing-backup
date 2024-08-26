#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class LogicalDevice;
class CommandBuffer;
class SingleTimeCommand;
class Buffer {
public:
	
	enum Location {
		
		HOST_COHERENT = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		
		DEVICE = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	
	enum Use {
		
		INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		
		VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		
		UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		
		STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		
		UPLOAD = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		DOWNLOAD = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		
		
		AS_STORE = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		
		AS_INPUT = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		
		AS_SCRATCH = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		
		SBT = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};
													Buffer(LogicalDevice* aDevice);
													Buffer(LogicalDevice* aDevice, VkBufferUsageFlags aUsageFlags, VkDeviceSize aAllocSize, VkMemoryPropertyFlags aMemPropertyFlags, VkExternalMemoryHandleTypeFlags aExtMemHandleTypeFlags = 0);
													~Buffer();

	void											create(VkBufferUsageFlags aUsageFlags, VkDeviceSize aAllocSize, VkMemoryPropertyFlags aMemPropertyFlags, VkExternalMemoryHandleTypeFlags aExtMemHandleTypeFlags = 0);
	void											destroy();

													
	void											mapBuffer(uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0);
	uint8_t*										getMemoryPointer() const;
	void											unmapBuffer();
	VkDeviceSize									getSize() const;
	VkBuffer										getRawHandle() const;
	BufferHandle									getHandle(VkDeviceSize aByteOffset = 0) const;
	VkDeviceAddress									getBufferDeviceAddress() const;
	VkDeviceMemory									getMemory() const;

#if defined(WIN32)
	[[nodiscard]] HANDLE							getExternalMemoryHandle(VkExternalMemoryHandleTypeFlagBits aFlags) const;
#else
	[[nodiscard]] int								getExternalMemoryHandle(VkExternalMemoryHandleTypeFlagBits aFlags) const;
#endif

													
													
													
	void											STC_UploadData(SingleTimeCommand* aStc, const void* aDataUp, uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0) const;
													
	void											STC_DownloadData(SingleTimeCommand* aStc, void* aDataDown, uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0) const;
													

													
	void											CMD_BindIndexBuffer(const CommandBuffer* aCmdBuffer, VkIndexType aType, VkDeviceSize aOffset = 0) const;
	void											CMD_BindVertexBuffer(const CommandBuffer* aCmdBuffer, uint32_t aBinding, VkDeviceSize aOffset = 0) const;
	void											CMD_CopyBuffer(const CommandBuffer* aCmdBuffer, const Buffer* aBufferDst, uint64_t aOffsetDst = 0, uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0) const;
	void											CMD_BufferMemoryBarrier(const CommandBuffer* aCmdBuffer, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask, VkAccessFlags aSrcAccessMask = 0, VkAccessFlags aDstAccessMask = 0, uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0) const;
	void											CMD_FillBuffer(const CommandBuffer* aCmdBuffer, uint32_t aData, uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0) const;
	void											CMD_UpdateBuffer(const CommandBuffer* aCmdBuffer, const void* aData, uint64_t aSize = UINT64_MAX, uint64_t aOffset = 0) const;
private:
	friend class									Descriptor;
	friend class									ASTriangleGeometry;
	friend class									ASAABBGeometry;
	friend class									BottomLevelAS;
	friend class									TopLevelAS;
	friend class									RTPipeline;
	friend class									CommandBuffer;

	void											createBufferMemory();

	LogicalDevice*									mDevice;
	VkDeviceSize									mAllocCapacity;
	VkDeviceSize									mAllocSize;
	VkBufferUsageFlags								mUsageFlags;
	VkMemoryPropertyFlags							mMemPropertyFlags;
	VkMemoryAllocateFlags							mAllocateFlags;
	VkExternalMemoryHandleTypeFlags					mExtMemHandleTypeFlags;
	
	uint8_t*										mP;
	VkBuffer										mBuffer;
	VkDeviceMemory									mMemory;
};
RVK_END_NAMESPACE