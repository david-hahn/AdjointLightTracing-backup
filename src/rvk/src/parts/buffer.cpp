#include <rvk/parts/buffer.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

#define RVK_BUFFER_CHECK_ALLOC_SIZE(error_msg)	if (mAllocCapacity == 0) {\
													Logger::error(error_msg);\
													return;\
												}
#define RVK_BUFFER_CHECK_SIZE(error_msg)		if (aSize == 0) {\
													Logger::error(error_msg);\
													return;\
												}\
												/* if size is UINT64_MAX use total buffer size and use offset 0 */\
												if (aSize == UINT64_MAX) {\
													aSize = mAllocCapacity;\
													aOffset = 0;\
												}\
												if (aOffset + aSize > mAllocCapacity) {\
													Logger::error("Vulkan: buffer -> offset + size > alloc_size!");\
													return;\
												}

Buffer::Buffer(LogicalDevice* aDevice) : mDevice(aDevice), mAllocCapacity(0), mAllocSize(0), mUsageFlags(0),
                                         mMemPropertyFlags(0), mAllocateFlags(0), mExtMemHandleTypeFlags(0),
                                         mP(nullptr), mBuffer(VK_NULL_HANDLE), mMemory(VK_NULL_HANDLE)
{
}

Buffer::Buffer(LogicalDevice* aDevice, const VkBufferUsageFlags aUsageFlags, const VkDeviceSize aAllocSize,
               const VkMemoryPropertyFlags aMemPropertyFlags, const VkExternalMemoryHandleTypeFlags aExtMemHandleTypeFlags) : Buffer(aDevice)
{
	create(aUsageFlags, aAllocSize, aMemPropertyFlags, aExtMemHandleTypeFlags);
}

Buffer::~Buffer() {
	destroy();
}

void Buffer::create(const VkBufferUsageFlags aUsageFlags, const VkDeviceSize aAllocSize, const VkMemoryPropertyFlags aMemPropertyFlags, const VkExternalMemoryHandleTypeFlags aExtMemHandleTypeFlags) {
	if(aAllocSize <= 0) Logger::error("Vulkan: Can not create buffer with size 0!");
	destroy();

	
	
	mAllocSize = mAllocCapacity = aAllocSize;
	mUsageFlags = aUsageFlags;
	mMemPropertyFlags = aMemPropertyFlags;
	mExtMemHandleTypeFlags = aExtMemHandleTypeFlags;

	
	if (isBitSet<VkBufferUsageFlags>(aUsageFlags, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)) {
		mAllocateFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
	}

	if (isBitSet<VkMemoryPropertyFlags>(aMemPropertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ||
		isBitSet<VkMemoryPropertyFlags>(aMemPropertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		mAllocCapacity = rountUpToMultipleOf<VkDeviceSize>(aAllocSize, mDevice->getPhysicalDevice()->getProperties().limits.nonCoherentAtomSize);
	}
	
	if (isBitSet<VkMemoryPropertyFlags>(aMemPropertyFlags, Location::DEVICE)) {
		if (isAnyBitSet<VkBufferUsageFlags>(aUsageFlags, Use::INDEX | Use::VERTEX | Use::UNIFORM | Use::STORAGE | Use::SBT) ) {
			mUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
	}
	createBufferMemory();
}
void Buffer::destroy()
{
	if (mP != nullptr) {
		mDevice->vk.UnmapMemory(mDevice->getHandle(), mMemory);
		mP = nullptr;
	}
	if (mBuffer != VK_NULL_HANDLE) {
		mDevice->vk.DestroyBuffer(mDevice->getHandle(), mBuffer, nullptr);
		mBuffer = VK_NULL_HANDLE;
	}
	if (mMemory != VK_NULL_HANDLE) {
		mDevice->vk.FreeMemory(mDevice->getHandle(), mMemory, nullptr);
		mMemory = VK_NULL_HANDLE;
	}
	mAllocCapacity = 0;
	mAllocSize = 0;
	mUsageFlags = 0;
	mAllocateFlags = 0;
	mMemPropertyFlags = 0;
}

void Buffer::mapBuffer(uint64_t aSize, uint64_t aOffset) {
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not map buffer with size 0!")
	RVK_BUFFER_CHECK_SIZE("Vulkan: Can not map buffer range of size 0!")
	if (mMemPropertyFlags == Location::DEVICE) {
		Logger::error("Vulkan: Can not map buffer which is located on device!");
		return;
	}
	if (!mP) {
		VK_CHECK(mDevice->vk.MapMemory(mDevice->getHandle(), mMemory, aOffset, aSize, 0, (void**)(&mP)), "failed to Map Memory!");
	}
}

uint8_t* Buffer::getMemoryPointer() const
{ return mP; }

VkDeviceSize Buffer::getSize() const
{ return mAllocSize; }

VkBuffer Buffer::getRawHandle() const
{ return mBuffer; }

BufferHandle Buffer::getHandle(const VkDeviceSize aByteOffset) const
{ return { mDevice,this, aByteOffset }; }

VkDeviceAddress Buffer::getBufferDeviceAddress() const
{
	VkBufferDeviceAddressInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	ai.buffer = mBuffer;
	return mDevice->vk.GetBufferDeviceAddress(mDevice->getHandle(), &ai);
}

VkDeviceMemory Buffer::getMemory() const
{
	return mMemory;
}

#if defined(WIN32)
HANDLE Buffer::getExternalMemoryHandle(const VkExternalMemoryHandleTypeFlagBits aFlags) const
{
	VkMemoryGetWin32HandleInfoKHR winHandleInfo = {};
	winHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
	winHandleInfo.memory = mMemory;
	winHandleInfo.handleType = aFlags;
	HANDLE handle;
	mDevice->vk.GetMemoryWin32HandleKHR(mDevice->getHandle(), &winHandleInfo, &handle);
	return handle;
}
#else
int Buffer::getExternalMemoryHandle(const VkExternalMemoryHandleTypeFlagBits aFlags) const
{
	VkMemoryGetFdInfoKHR fdInfo = {};
	fdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	fdInfo.memory = mMemory;
	fdInfo.handleType = aFlags;
	int fd;
	mDevice->vk.GetMemoryFdKHR(mDevice->getHandle(), &fdInfo, &fd);
	return fd;
}
#endif

void Buffer::unmapBuffer() {
	if (mMemPropertyFlags == Location::DEVICE) {
		Logger::error("Vulkan: Can not unmap buffer which is located on device!");
		return;
	}
	if (mP) {
		mDevice->vk.UnmapMemory(mDevice->getHandle(), mMemory);
		mP = nullptr;
	}
}

void Buffer::STC_UploadData(SingleTimeCommand* aStc, const void* aDataUp, uint64_t aSize, uint64_t aOffset) const
{
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not upload to buffer with size 0!")
	RVK_BUFFER_CHECK_SIZE("Vulkan: Can not upload data with size 0 to buffer!")
	if (aOffset + aSize > mAllocCapacity) {
		Logger::error("Vulkan: Buffer to small!");
		return;
	}
	if (mMemPropertyFlags == Location::HOST_COHERENT) {
		if (mP == nullptr) {
			Logger::error("Vulkan: Buffer not mapped!");
		}
		else {
			uint8_t* p = mP + aOffset;
			std::memcpy(p, aDataUp, (size_t)(aSize));
		}
	}
	else if (mMemPropertyFlags == Location::DEVICE) {
		if (!isBitSet<VkBufferUsageFlags>(mUsageFlags, Use::UPLOAD)) {
			Logger::error("Buffer: VK_BUFFER_USAGE_TRANSFER_DST_BIT not set");
			return;
		}

		Buffer stagingBuffer(mDevice);
		stagingBuffer.create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, aSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		stagingBuffer.mapBuffer();
		uint8_t* p = stagingBuffer.getMemoryPointer();
		memcpy(p, aDataUp, (size_t)aSize);
		stagingBuffer.unmapBuffer();

		aStc->begin();
		VkBufferCopy copyRegion = {};
		copyRegion.size = aSize;
		copyRegion.dstOffset = aOffset;
		mDevice->vkCmd.CopyBuffer(aStc->buffer()->getHandle(), stagingBuffer.getRawHandle(), mBuffer, 1, &copyRegion);

		VkMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		VkBufferMemoryBarrier bufferbarrier = {};
		bufferbarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferbarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bufferbarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferbarrier.buffer = mBuffer;
		bufferbarrier.size = aSize;
		bufferbarrier.offset = aOffset;

		mDevice->vkCmd.PipelineBarrier(aStc->buffer()->getHandle(),
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			0, 1, &barrier, 0, &bufferbarrier, 0, VK_NULL_HANDLE);
		aStc->end();
	}
}

void Buffer::STC_DownloadData(SingleTimeCommand* aStc, void* aDataDown, uint64_t aSize, uint64_t aOffset) const
{
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not download from buffer with size 0!")
	RVK_BUFFER_CHECK_SIZE("Vulkan: Can not download data with size 0 from buffer!")
	if (mMemPropertyFlags == Location::HOST_COHERENT) {
		if (mP == nullptr) {
			Logger::error("Vulkan: Buffer not mapped!");
		}
		else {
			const uint8_t* p = mP + aOffset;
			std::memcpy(aDataDown, p, (size_t)(aSize));
		}
	}
	else if (mMemPropertyFlags == Location::DEVICE) {
		if (!isBitSet<VkBufferUsageFlags>(mUsageFlags, Use::DOWNLOAD)) {
			Logger::error("Buffer: VK_BUFFER_USAGE_TRANSFER_SRC_BIT not set");
			return;
		}

		Buffer stagingBuffer(mDevice);
		stagingBuffer.create(VK_BUFFER_USAGE_TRANSFER_DST_BIT, aSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		aStc->begin();

		VkBufferCopy copyRegion = {};
		copyRegion.size = aSize;
		copyRegion.srcOffset = aOffset;
		mDevice->vkCmd.CopyBuffer(aStc->buffer()->getHandle(), mBuffer, stagingBuffer.getRawHandle(), 1, &copyRegion);

		VkMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		VkBufferMemoryBarrier bufferbarrier = {};
		bufferbarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferbarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		bufferbarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		bufferbarrier.buffer = mBuffer;
		bufferbarrier.size = aSize;
		bufferbarrier.offset = aOffset;

		mDevice->vkCmd.PipelineBarrier(aStc->buffer()->getHandle(),
			VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 1, &barrier, 0, &bufferbarrier, 0, VK_NULL_HANDLE);
		aStc->end();

		stagingBuffer.mapBuffer();
		const uint8_t* p = stagingBuffer.getMemoryPointer();
		memcpy(aDataDown, p, (size_t)aSize);
		stagingBuffer.unmapBuffer();
	}
}

void Buffer::CMD_BindIndexBuffer(const CommandBuffer* aCmdBuffer, const VkIndexType aType, const VkDeviceSize aOffset) const
{
	mDevice->vkCmd.BindIndexBuffer(aCmdBuffer->getHandle(), mBuffer, aOffset, aType);
}

void Buffer::CMD_BindVertexBuffer(const CommandBuffer* aCmdBuffer, const uint32_t aBinding, const VkDeviceSize aOffset) const
{
	mDevice->vkCmd.BindVertexBuffers(aCmdBuffer->getHandle(), aBinding, 1, &mBuffer, &aOffset);
}

void Buffer::CMD_CopyBuffer(const CommandBuffer* aCmdBuffer, const Buffer* const aBufferDst, const uint64_t aOffsetDst,
	uint64_t aSize, uint64_t aOffset) const
{
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not copy from buffer with size 0!")
		if (aBufferDst->mAllocCapacity == 0) { Logger::error("Vulkan: Can not copy to buffer with size 0!"); return; }
	RVK_BUFFER_CHECK_SIZE("Vulkan: Can not copy data of size 0!")
		if (aOffsetDst + aSize > aBufferDst->mAllocCapacity) { Logger::error("Vulkan: buffer -> offset_dst + size > alloc_size_dst!"); return; }

	VkBufferCopy copyRegion = {};
	copyRegion.size = aSize;
	copyRegion.srcOffset = aOffset;
	copyRegion.dstOffset = aOffsetDst;
	mDevice->vkCmd.CopyBuffer(aCmdBuffer->getHandle(), mBuffer, aBufferDst->mBuffer, 1, &copyRegion);
}

void Buffer::CMD_BufferMemoryBarrier(const CommandBuffer* aCmdBuffer, const VkPipelineStageFlags aSrcStageMask,
                                     const VkPipelineStageFlags aDstStageMask, const VkAccessFlags aSrcAccessMask, const VkAccessFlags aDstAccessMask, uint64_t aSize,
                                     uint64_t aOffset) const
{
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not set memory barrier for buffer with size 0!")
		RVK_BUFFER_CHECK_SIZE("Vulkan: Can not set memory barrier for memory range of size 0!")
		VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = mBuffer;
	barrier.srcAccessMask = aSrcAccessMask;
	barrier.dstAccessMask = aDstAccessMask;
	barrier.size = aSize;
	barrier.offset = aOffset;
	mDevice->vkCmd.PipelineBarrier(aCmdBuffer->getHandle(),
		aSrcStageMask,
		aDstStageMask,
		0,
		0, VK_NULL_HANDLE,
		1, &barrier,
		0, VK_NULL_HANDLE);
}

void Buffer::CMD_FillBuffer(const CommandBuffer* aCmdBuffer, const uint32_t aData, uint64_t aSize, uint64_t aOffset) const
{
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not update buffer with size 0!")
	RVK_BUFFER_CHECK_SIZE("Vulkan: Update size is 0!")
	mDevice->vkCmd.FillBuffer(aCmdBuffer->getHandle(), mBuffer, aOffset, aSize, aData);
}

void Buffer::CMD_UpdateBuffer(const CommandBuffer* aCmdBuffer, const void* aData, uint64_t aSize, uint64_t aOffset) const
{
	RVK_BUFFER_CHECK_ALLOC_SIZE("Vulkan: Can not update buffer with size 0!")
	RVK_BUFFER_CHECK_SIZE("Vulkan: Update size is 0!")
	if (aOffset % 4 != 0 || aSize % 4 != 0) {
		Logger::error("Vulkan: CMD_UpdateBuffer -> size/offset must be a multiple of 4!");
	}
	if (aSize > 65536) {
		Logger::error("Vulkan: CMD_UpdateBuffer -> size must be less than or equal to 65536!");
	}
	mDevice->vkCmd.UpdateBuffer(aCmdBuffer->getHandle(), mBuffer, aOffset, aSize, aData);
}

void Buffer::createBufferMemory()
{
	VkBufferCreateInfo bufInfo = {};
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.size = mAllocCapacity;
	bufInfo.usage = mUsageFlags;

	VK_CHECK(mDevice->vk.CreateBuffer(mDevice->getHandle(), &bufInfo, NULL, &mBuffer), "failed to create Buffer!");

	VkMemoryRequirements memReq = {};
	mDevice->vk.GetBufferMemoryRequirements(mDevice->getHandle(), mBuffer, &memReq);

	
	VkExportMemoryAllocateInfo emai = {};
	emai.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
	emai.handleTypes = mExtMemHandleTypeFlags;

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.pNext = mExtMemHandleTypeFlags ? &emai : VK_NULL_HANDLE;
	memoryAllocateFlagsInfo.flags = mAllocateFlags;

	const VkMemoryAllocateInfo memAllocInfo = {
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		(mAllocateFlags == 0) ? VK_NULL_HANDLE : &memoryAllocateFlagsInfo,
		memReq.size,
		static_cast<uint32_t>(mDevice->findMemoryTypeIndex(mMemPropertyFlags, memReq.memoryTypeBits))
	};
	ASSERT(memAllocInfo.memoryTypeIndex != -1, "could not find suitable Memory");

	VK_CHECK(mDevice->vk.AllocateMemory(mDevice->getHandle(), &memAllocInfo, NULL, &mMemory), "failed to allocate Memory!");
	VK_CHECK(mDevice->vk.BindBufferMemory(mDevice->getHandle(), mBuffer, mMemory, 0), "failed to bind Buffer Memory!");
}

#undef RVK_BUFFER_CHECK_ALLOC_SIZE
#undef RVK_BUFFER_CHECK_SIZE
