#include <rvk/parts/acceleration_structure_bottom_level.hpp>
#include <rvk/parts/command_buffer.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

rvk::ASTriangleGeometry::ASTriangleGeometry() : mTriangleData({}), mHasIndices(false), mTriangleCount(0),
mIndexBufferOffset(0), mVertexBufferOffset(0), mTransformOffset(0) {
	mTriangleData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	mTriangleData.pNext = VK_NULL_HANDLE;
	mTriangleData.indexType = VK_INDEX_TYPE_NONE_KHR;
}

ASTriangleGeometry::~ASTriangleGeometry()
{ mTriangleData = {}; };

void rvk::ASTriangleGeometry::setVerticesFromDevice(const VkFormat aFormat, const uint32_t aStride, const uint32_t aVertexCount, const rvk::Buffer* const aVertexBuffer, const uint32_t aOffset) {
	mTriangleData.vertexFormat = aFormat;
	mTriangleData.vertexData.deviceAddress = aVertexBuffer->getBufferDeviceAddress();
	mTriangleData.maxVertex = aVertexCount;
	mTriangleData.vertexStride = aStride;

	
	if (mTriangleData.indexType == VK_INDEX_TYPE_NONE_KHR) {
		if(aVertexCount % 3 == 0) mTriangleCount = aVertexCount / 3;
	}
	mVertexBufferOffset = aOffset;
}

void ASTriangleGeometry::setIndicesFromDevice(const VkIndexType aType, const uint32_t aIndexCount, const rvk::Buffer* aIndexBuffer,
                                              const uint32_t aOffset) {
	mTriangleData.indexType = aType;
	mTriangleData.indexData.deviceAddress = aIndexBuffer->getBufferDeviceAddress();
	ASSERT(aIndexCount % 3 == 0, "Only geometries with triangle topology are supported for acceleration structure creation");
	mTriangleCount = aIndexCount / 3;
	mIndexBufferOffset = aOffset;
	mHasIndices = true;
}

void rvk::ASTriangleGeometry::setTransformFromDevice(const BufferHandle aBufferHandle) {
	mTriangleData.transformData.deviceAddress = aBufferHandle.mBuffer->getBufferDeviceAddress();
	mTransformOffset = static_cast<uint32_t>(aBufferHandle.mBufferOffset);
}

rvk::ASAABBGeometry::ASAABBGeometry() : mAabbData({}), mAabbCount(0), mAabbOffset(0) {
	mAabbData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
	mAabbData.pNext = VK_NULL_HANDLE;
	mAabbData.stride = 24;
}
void rvk::ASAABBGeometry::setAABBsFromDevice(const rvk::Buffer* const aAabbBuffer, const uint32_t aAabbCount, const uint32_t aStride, const uint32_t aOffset)
{
	mAabbData.data.deviceAddress = aAabbBuffer->getBufferDeviceAddress();
    mAabbData.stride = aStride;
	mAabbCount = aAabbCount;
    mAabbOffset = aOffset;
}


BottomLevelAS::BottomLevelAS(LogicalDevice* aDevice) : AccelerationStructure(aDevice), mGeometry({}), mRange({}), mPrimitives({})
{
}

rvk::BottomLevelAS::~BottomLevelAS()
{
	destroy();
}

void BottomLevelAS::reserve(const uint32_t aReserve)
{ mGeometry.reserve(aReserve); mPrimitives.reserve(aReserve); mRange.reserve(aReserve); }

void rvk::BottomLevelAS::addGeometry(ASTriangleGeometry const& aTriangleGeometry, const VkGeometryFlagsKHR aFlags)
{
	mValidSize = false;
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = aFlags;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles = aTriangleGeometry.mTriangleData;
	mGeometry.push_back(accelerationStructureGeometry);
	mPrimitives.push_back(aTriangleGeometry.mTriangleCount);

	VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo = {};
	asBuildRangeInfo.primitiveCount = aTriangleGeometry.mTriangleCount;
	if (aTriangleGeometry.mHasIndices) {
		asBuildRangeInfo.primitiveOffset = aTriangleGeometry.mIndexBufferOffset;
		asBuildRangeInfo.firstVertex = aTriangleGeometry.mVertexBufferOffset/aTriangleGeometry.mTriangleData.vertexStride;
	} else {
		asBuildRangeInfo.primitiveOffset = aTriangleGeometry.mVertexBufferOffset;
	}

	asBuildRangeInfo.transformOffset = aTriangleGeometry.mTransformOffset;
	mRange.push_back(asBuildRangeInfo);
}

void rvk::BottomLevelAS::addGeometry(ASAABBGeometry const& aAabbGeometry, const VkGeometryFlagsKHR aFlags)
{
	mValidSize = false;
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = aFlags;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
	accelerationStructureGeometry.geometry.aabbs = aAabbGeometry.mAabbData;
	mGeometry.push_back(accelerationStructureGeometry);
	mPrimitives.push_back(aAabbGeometry.mAabbCount);

	VkAccelerationStructureBuildRangeInfoKHR asBuildRangeInfo = {};
	asBuildRangeInfo.primitiveCount = aAabbGeometry.mAabbCount;
	asBuildRangeInfo.primitiveOffset = aAabbGeometry.mAabbOffset;
	asBuildRangeInfo.firstVertex = 0;
	asBuildRangeInfo.transformOffset = 0;
	mRange.push_back(asBuildRangeInfo);
}

void rvk::BottomLevelAS::preprepare(const VkBuildAccelerationStructureFlagsKHR aBuildFlags)
{
	mValidSize = true;
	
	mBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	mBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	mBuildInfo.flags = aBuildFlags;
	mBuildInfo.geometryCount = mGeometry.size();
	mBuildInfo.pGeometries = toArrayPointer(mGeometry);

	
	VkAccelerationStructureBuildSizesInfoKHR asBuildSizesInfo{};
	asBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	mDevice->vk.GetAccelerationStructureBuildSizesKHR(mDevice->getHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&mBuildInfo, toArrayPointer(mPrimitives), &asBuildSizesInfo);
	mAsSize = asBuildSizesInfo.accelerationStructureSize;
	mBuildSize = asBuildSizesInfo.buildScratchSize;
	mUpdateSize = asBuildSizesInfo.updateScratchSize;

}

void rvk::BottomLevelAS::prepare(const VkBuildAccelerationStructureFlagsKHR aBuildFlags)
{
	
	if(!mValidSize) preprepare(aBuildFlags);

	
	if (!checkBuffer(mAsBuffer, mAsSize)) {
		mAsBuffer.buffer = new rvk::Buffer(mDevice, rvk::Buffer::Use::AS_STORE, mAsSize, rvk::Buffer::Location::DEVICE);
		mAsBuffer.external = mAsBuffer.offset = 0;
	}
	if (mAsBuffer.offset % 256 != 0) Logger::error("Vulkan: AS buffer offset must be multiple of 256");

	if (mAs != VK_NULL_HANDLE) mDevice->vk.DestroyAccelerationStructureKHR(mDevice->getHandle(), mAs, nullptr);
	
	VkDeviceSize offset = 0;
	VkAccelerationStructureCreateInfoKHR asCreateInfo = {};
	asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	asCreateInfo.buffer = mAsBuffer.buffer->mBuffer;
	asCreateInfo.offset = mAsBuffer.offset; 
	asCreateInfo.size = mAsSize;
	asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	VK_CHECK(mDevice->vk.CreateAccelerationStructureKHR(mDevice->getHandle(), &asCreateInfo, NULL, &mAs), "failed to create Acceleration Structure!");
}

int BottomLevelAS::size() const
{ return mGeometry.size(); }

void BottomLevelAS::clear()
{ mGeometry.clear(); mRange.clear(); mPrimitives.clear(); mBuildInfo = {}; }


void rvk::BottomLevelAS::destroyTempBuffers()
{
	deleteInternalBuffer(mScratchBuffer);
}

void rvk::BottomLevelAS::destroy()
{
	AccelerationStructure::destroy();
	mGeometry.clear();
	mPrimitives.clear();
	mRange.clear();
	mGeometry = {};
	mPrimitives = {};
	mRange = {};
}

void BottomLevelAS::CMD_Build(const CommandBuffer* aCmdBuffer, BottomLevelAS* aSrcAs)
{
	if (mAs == nullptr) Logger::error("Can not build blas when prepare was not called");
	const bool update = aSrcAs == nullptr ? false : true;
	if (update && aSrcAs->mAs == nullptr) Logger::error("Can not update blas because source is not a completed blas");

	
	mScratchBuffer.offset = rountUpToMultipleOf(mScratchBuffer.offset, mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
	if (!checkBuffer(mScratchBuffer, update ? mUpdateSize : mBuildSize)) {
		mScratchBuffer.buffer = new rvk::Buffer(mDevice, rvk::Buffer::Use::AS_SCRATCH, update ? mUpdateSize : mBuildSize, rvk::Buffer::Location::DEVICE);
		mScratchBuffer.external = mScratchBuffer.offset = 0;
	}

	mBuildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	mBuildInfo.dstAccelerationStructure = mAs;
	mBuildInfo.srcAccelerationStructure = update ? aSrcAs->mAs : VK_NULL_HANDLE;
	mBuildInfo.scratchData.deviceAddress = mScratchBuffer.buffer->getBufferDeviceAddress() + mScratchBuffer.offset;
	const VkAccelerationStructureBuildRangeInfoKHR* pAsBuildRangeInfos = toArrayPointer(mRange);
	mDevice->vkCmd.BuildAccelerationStructuresKHR(aCmdBuffer->getHandle(), 1, &mBuildInfo, &pAsBuildRangeInfos);
	mBuild = true;
}
