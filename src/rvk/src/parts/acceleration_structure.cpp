#include <rvk/parts/acceleration_structure.hpp>
#include <rvk/rvk.hpp>
RVK_USE_NAMESPACE

void AccelerationStructure::setScratchBuffer(Buffer* aBuffer, const uint32_t aOffset)
{
	deleteInternalBuffer(mScratchBuffer);
	mScratchBuffer.buffer = aBuffer;
	mScratchBuffer.offset = aOffset;
	mScratchBuffer.external = true;
}

void AccelerationStructure::setASBuffer(Buffer* aBuffer, const uint32_t aOffset)
{
	deleteInternalBuffer(mAsBuffer);
	mAsBuffer.buffer = aBuffer;
	mAsBuffer.offset = aOffset;
	mAsBuffer.external = true;
}

uint32_t AccelerationStructure::getASSize() const
{ return mAsSize; }

uint32_t AccelerationStructure::getBuildScratchSize() const
{ return mBuildSize; }

uint32_t AccelerationStructure::getUpdateScratchSize() const
{ return mUpdateSize; }

bool AccelerationStructure::isBuild() const
{ return mBuild; }

VkDeviceAddress AccelerationStructure::getAccelerationStructureDeviceAddress() const
{
	VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
	addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addressInfo.accelerationStructure = mAs;
	return mDevice->vk.GetAccelerationStructureDeviceAddressKHR(mDevice->getHandle(), &addressInfo);
}

AccelerationStructure::AccelerationStructure(LogicalDevice* aDevice) : mDevice(aDevice), mValidSize(false),
mAsSize(0), mBuildSize(0), mUpdateSize(0),
mScratchBuffer({}), mAsBuffer({}),
mBuild(false), mBuildInfo({}),
mAs(VK_NULL_HANDLE)
{
}

bool rvk::AccelerationStructure::checkBuffer(const buffer_s& aBuffer, const uint32_t aReqSize)
{
	
	if (aBuffer.buffer == nullptr) return false;
	
	else if (!aBuffer.external) {
		if (aBuffer.buffer->getSize() < aReqSize) {
			delete aBuffer.buffer;
			return false;
		}
	}
	
	else if (aBuffer.external) {
		if (aBuffer.buffer->getSize() < (aReqSize + aBuffer.offset)) Logger::error("AS buffer to small");
	}
	return true;
}
void rvk::AccelerationStructure::deleteInternalBuffer(buffer_s& aBuffer)
{
	if (aBuffer.buffer != nullptr && aBuffer.external == false) {
		delete aBuffer.buffer;
		aBuffer = {};
	}
}
void rvk::AccelerationStructure::destroy()
{
	mValidSize = false;
	mAsSize = 0;
	mBuildSize = 0;
	mUpdateSize = 0;
	mBuildInfo = {};
	if (mAs != VK_NULL_HANDLE) {
		mDevice->vk.DestroyAccelerationStructureKHR(mDevice->getHandle(), mAs, VK_NULL_HANDLE);
		mAs = VK_NULL_HANDLE;
	}
	deleteInternalBuffer(mScratchBuffer);
	deleteInternalBuffer(mAsBuffer);
	mBuild = false;
}

