#include <rvk/parts/acceleration_structure_top_level.hpp>
#include <rvk/parts/command_buffer.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE


rvk::ASInstance::ASInstance(const BottomLevelAS* aBottom) : mInstance({ {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f }, 0, 0xFF, 0, 0, aBottom->getAccelerationStructureDeviceAddress() }) {}

void ASInstance::setCustomIndex(const uint32_t aIndex)
{ mInstance.instanceCustomIndex = aIndex; }

void ASInstance::setMask(const uint32_t aMask)
{ mInstance.mask = aMask; }

void ASInstance::setSBTRecordOffset(const uint32_t aOffset)
{ mInstance.instanceShaderBindingTableRecordOffset = aOffset; }

void ASInstance::setFlags(const VkGeometryInstanceFlagsKHR aFlags)
{ mInstance.flags = aFlags; }

void ASInstance::setTransform(const float* const aMat3X4)
{ std::memcpy(&mInstance.transform, aMat3X4, sizeof(VkTransformMatrixKHR)); }

TopLevelAS::TopLevelAS(LogicalDevice* aDevice) : AccelerationStructure(aDevice), mInstanceFlags(0), mInstanceList({}), mInstanceBuffer({})
{
}

rvk::TopLevelAS::~TopLevelAS()
{
	destroy();
}

void TopLevelAS::reserve(const uint32_t aReserve)
{ mInstanceList.reserve(aReserve); }

void TopLevelAS::setInstanceBuffer(rvk::Buffer* aBuffer, const uint32_t aOffset)
{
	deleteInternalBuffer(mInstanceBuffer);
	mInstanceBuffer.buffer = aBuffer;
	mInstanceBuffer.offset = aOffset;
	mInstanceBuffer.external = true;
}

void rvk::TopLevelAS::addInstance(ASInstance const& aBottomInstance)
{
	mValidSize = false;
	mInstanceList.push_back(aBottomInstance.mInstance);
}

void TopLevelAS::replaceInstance(const uint32_t aIndex, ASInstance const& aBottomInstance)
{
	mValidSize = false;
	mInstanceList[aIndex] = aBottomInstance.mInstance;
}

void rvk::TopLevelAS::preprepare(const VkBuildAccelerationStructureFlagsKHR aBuildFlags)
{
	
	
	VkAccelerationStructureGeometryKHR asInstanceGeometry = {};
	asInstanceGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	asInstanceGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	asInstanceGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

	
	mBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	mBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	mBuildInfo.flags = aBuildFlags;
	mBuildInfo.geometryCount = 1; 
	mBuildInfo.pGeometries = &asInstanceGeometry;

	
	const uint32_t instanceCount = mInstanceList.size();
	VkAccelerationStructureBuildSizesInfoKHR asBuildSizesInfo{};
	asBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	mDevice->vk.GetAccelerationStructureBuildSizesKHR(mDevice->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&mBuildInfo, &instanceCount, &asBuildSizesInfo);
	mAsSize = asBuildSizesInfo.accelerationStructureSize;
	mBuildSize = asBuildSizesInfo.buildScratchSize;
	mUpdateSize = asBuildSizesInfo.updateScratchSize;
	mBuildInfo.pGeometries = nullptr;
}

void rvk::TopLevelAS::prepare(const VkBuildAccelerationStructureFlagsKHR aBuildFlags, const VkGeometryFlagsKHR aInstanceFlags)
{
	mInstanceFlags = aInstanceFlags;

	
	if (!mValidSize) preprepare(aBuildFlags);

	
	if (!checkBuffer(mAsBuffer, mAsSize)) {
		mAsBuffer.buffer = new rvk::Buffer(mDevice, rvk::Buffer::Use::AS_STORE, mAsSize, rvk::Buffer::Location::DEVICE);
		mAsBuffer.external = mAsBuffer.offset = 0;
	}
	if (mAsBuffer.offset % 256 != 0) Logger::error("Vulkan: AS buffer offset must be multiple of 256");

	if (mAs != VK_NULL_HANDLE)  mDevice->vk.DestroyAccelerationStructureKHR(mDevice->getHandle(), mAs, nullptr);
	
	VkDeviceSize offset = 0;
	VkAccelerationStructureCreateInfoKHR asCreateInfo{};
	asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	asCreateInfo.buffer = mAsBuffer.buffer->mBuffer;
	asCreateInfo.offset = mAsBuffer.offset; 
	asCreateInfo.size = mAsSize;
	asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	VK_CHECK(mDevice->vk.CreateAccelerationStructureKHR(mDevice->getHandle(), &asCreateInfo, nullptr, &mAs), "failed to create Acceleration Structure!");
}

uint32_t TopLevelAS::size() const
{ return static_cast<uint32_t>(mInstanceList.size()); }

uint32_t TopLevelAS::getInstanceBufferSize() const
{ return mInstanceList.size() * sizeof(VkAccelerationStructureInstanceKHR); }

void TopLevelAS::clear()
{ mInstanceList.clear(); }

void rvk::TopLevelAS::destroyTempBuffers()
{
	deleteInternalBuffer(mScratchBuffer);
	deleteInternalBuffer(mInstanceBuffer);
}

void rvk::TopLevelAS::destroy()
{
	AccelerationStructure::destroy();
	
	deleteInternalBuffer(mInstanceBuffer);
	mInstanceList.clear();
	mInstanceList = {};
	mInstanceFlags = 0;
	mInstanceBuffer = {};
}

void TopLevelAS::CMD_Build(const CommandBuffer* aCmdBuffer, TopLevelAS* aSrcAs)
{
	if (mAs == nullptr) Logger::error("Can not build tlas when prepare was not called");
	const bool update = aSrcAs == nullptr ? false : true;
	if (update && aSrcAs->mAs == nullptr) Logger::error("Can not update tlas because source is not a completed blas");

	
	
	
	if (!checkBuffer(mInstanceBuffer, mInstanceList.size() * sizeof(VkAccelerationStructureInstanceKHR))) {
		mInstanceBuffer.buffer = new rvk::Buffer(mDevice, rvk::Buffer::Use::AS_INPUT, mInstanceList.size() * sizeof(VkAccelerationStructureInstanceKHR), rvk::Buffer::Location::HOST_COHERENT);
		mInstanceBuffer.external = mInstanceBuffer.offset = 0;
		mInstanceBuffer.buffer->mapBuffer();
	}
	mInstanceBuffer.buffer->STC_UploadData(nullptr, toArrayPointer(mInstanceList), mInstanceList.size() * sizeof(VkAccelerationStructureInstanceKHR));

	mInstanceBuffer.buffer->CMD_BufferMemoryBarrier(aCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);

	VkAccelerationStructureGeometryKHR asInstanceGeometry = {};
	asInstanceGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	asInstanceGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	asInstanceGeometry.flags = mInstanceFlags;
	asInstanceGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	asInstanceGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	asInstanceGeometry.geometry.instances.data.deviceAddress = mInstanceBuffer.buffer->getBufferDeviceAddress() + mInstanceBuffer.offset;
	mBuildInfo.pGeometries = &asInstanceGeometry;

	
	mScratchBuffer.offset = rountUpToMultipleOf(mScratchBuffer.offset, mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
	if (!checkBuffer(mScratchBuffer, update ? mUpdateSize : mBuildSize)) {
		mScratchBuffer.buffer = new rvk::Buffer(mDevice, rvk::Buffer::Use::AS_SCRATCH, update ? mUpdateSize : mBuildSize, rvk::Buffer::Location::DEVICE);
		mScratchBuffer.external = mScratchBuffer.offset = 0;
	}

	mBuildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	mBuildInfo.dstAccelerationStructure = mAs;
	mBuildInfo.srcAccelerationStructure = update ? aSrcAs->mAs : VK_NULL_HANDLE;
	mBuildInfo.scratchData.deviceAddress = mScratchBuffer.buffer->getBufferDeviceAddress() + mScratchBuffer.offset;

	VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo{ static_cast<uint32_t>(mInstanceList.size()), 0, 0, 0 };
	VkAccelerationStructureBuildRangeInfoKHR* p_asBuildRangeInfos = &asBuildRangeInfo;
	mDevice->vkCmd.BuildAccelerationStructuresKHR(aCmdBuffer->getHandle(), 1, &mBuildInfo, &p_asBuildRangeInfos);
	mBuild = true;
}
