#pragma once
#include <rvk/parts/rvk_public.hpp>
#include <rvk/parts/acceleration_structure.hpp>

RVK_BEGIN_NAMESPACE
class Buffer;
class CommandBuffer;


class ASTriangleGeometry {
public:
													ASTriangleGeometry();
													~ASTriangleGeometry();
													
													
	void											setVerticesFromDevice(VkFormat aFormat, uint32_t aStride, uint32_t aVertexCount, const rvk::Buffer* aVertexBuffer, uint32_t aOffset = 0);
													
	void											setIndicesFromDevice(VkIndexType aType, uint32_t aIndexCount, const rvk::Buffer* aIndexBuffer, uint32_t aOffset = 0);
													
	void											setTransformFromDevice(BufferHandle aBufferHandle);

private:
	friend class									BottomLevelAS;
	VkAccelerationStructureGeometryTrianglesDataKHR mTriangleData;

	bool											mHasIndices;
	uint32_t										mTriangleCount;
	uint32_t										mIndexBufferOffset;
	uint32_t										mVertexBufferOffset;
	uint32_t										mTransformOffset;
};
class ASAABBGeometry {
public:
	typedef VkAabbPositionsKHR AABB_s;
	ASAABBGeometry();

	void											setAABBsFromDevice(const rvk::Buffer* aAabbBuffer, uint32_t aAabbCount = 1, uint32_t aStride = 24, uint32_t aOffset = 0);
private:
	friend class									BottomLevelAS;
	VkAccelerationStructureGeometryAabbsDataKHR		mAabbData;
	uint32_t										mAabbCount;
	uint32_t										mAabbOffset;
};

class BottomLevelAS final : public AccelerationStructure {
public:
													BottomLevelAS(LogicalDevice* aDevice);
													~BottomLevelAS();

													
	void											reserve(uint32_t aReserve);
													
	void											addGeometry(ASTriangleGeometry const& aTriangleGeometry, VkGeometryFlagsKHR aFlags);
	void											addGeometry(ASAABBGeometry const& aAabbGeometry, VkGeometryFlagsKHR aFlags);
													
	void											preprepare(VkBuildAccelerationStructureFlagsKHR aBuildFlags);
													
	void											prepare(VkBuildAccelerationStructureFlagsKHR aBuildFlags);
													
	int												size() const;
													
	void											clear();
													
													
	void											destroyTempBuffers() override;
													
	void											destroy();

													
	void											CMD_Build(const CommandBuffer* aCmdBuffer, BottomLevelAS* aSrcAs = nullptr);
private:
	friend class									TopLevelAS;
	friend class									ASInstance;


	std::vector<VkAccelerationStructureGeometryKHR>			mGeometry;		
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>	mRange;			
	std::vector<uint32_t>									mPrimitives;		
};
RVK_END_NAMESPACE