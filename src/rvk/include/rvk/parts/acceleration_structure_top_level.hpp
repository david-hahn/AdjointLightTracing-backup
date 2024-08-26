#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/acceleration_structure.hpp>

RVK_BEGIN_NAMESPACE
class BottomLevelAS;
class LogicalDevice;
class Buffer;
class CommandBuffer;


class ASInstance {
public:
													ASInstance(const BottomLevelAS* aBottom);
													~ASInstance() = default;
													
	void											setCustomIndex(uint32_t aIndex);
	void											setMask(uint32_t aMask);
	void											setSBTRecordOffset(uint32_t aOffset);
	void											setFlags(VkGeometryInstanceFlagsKHR aFlags);
	void											setTransform(const float* aMat3X4);

												private:
	friend class									TopLevelAS;
	VkAccelerationStructureInstanceKHR				mInstance;
};


class TopLevelAS final : public AccelerationStructure {
public:
													TopLevelAS(LogicalDevice* aDevice);
													~TopLevelAS();
													
	void											reserve(uint32_t aReserve);
													
	void											setInstanceBuffer(rvk::Buffer* aBuffer, uint32_t aOffset = 0);

													
	void											addInstance(ASInstance const& aBottomInstance);
	void											replaceInstance(uint32_t aIndex, ASInstance const& aBottomInstance);
													
	void											preprepare(VkBuildAccelerationStructureFlagsKHR aBuildFlags);
													
	void											prepare(VkBuildAccelerationStructureFlagsKHR aBuildFlags, VkGeometryFlagsKHR aInstanceFlags = 0);
													
	uint32_t										size() const;
													
	uint32_t										getInstanceBufferSize() const;
													
	void											clear();
													
	void											destroyTempBuffers() override;
													
	void											destroy();

													
	void											CMD_Build(const CommandBuffer* aCmdBuffer, TopLevelAS* aSrcAs = nullptr);
private:
	friend class									Descriptor;


	VkGeometryFlagsKHR								mInstanceFlags;
	std::vector<VkAccelerationStructureInstanceKHR> mInstanceList;

	buffer_s										mInstanceBuffer;
};
RVK_END_NAMESPACE