#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class Buffer;
class LogicalDevice;





class AccelerationStructure {
public:

	
	void											setScratchBuffer(Buffer* aBuffer, uint32_t aOffset = 0);
	void											setASBuffer(Buffer* aBuffer, uint32_t aOffset = 0);

	uint32_t										getASSize() const;
	uint32_t										getBuildScratchSize() const;
	uint32_t										getUpdateScratchSize() const;
	bool											isBuild() const;

	VkDeviceAddress									getAccelerationStructureDeviceAddress() const;

	
	
	
	
	virtual void									destroyTempBuffers() = 0;
protected:
													AccelerationStructure(LogicalDevice* aDevice);
													~AccelerationStructure() = default;
	struct buffer_s {
		rvk::Buffer* buffer = nullptr;
		uint32_t									offset = 0;
		bool										external = false;	
	};
	
	
	
	static bool										checkBuffer(const buffer_s& aBuffer, uint32_t aReqSize);
	
	static void										deleteInternalBuffer(buffer_s& aBuffer);
	virtual void											destroy();

	LogicalDevice*									mDevice;
	bool											mValidSize;
	uint32_t										mAsSize;		
	uint32_t										mBuildSize;		
	uint32_t										mUpdateSize;	

	buffer_s										mScratchBuffer;
	buffer_s										mAsBuffer;

	bool											mBuild;
	VkAccelerationStructureBuildGeometryInfoKHR		mBuildInfo;
	VkAccelerationStructureKHR						mAs;
};
RVK_END_NAMESPACE