#pragma once
#include <tamashii/core/scene/render_scene.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE
class MaterialDataVulkan;
class LightDataVulkan;
class GeometryDataVulkan {
public:
															GeometryDataVulkan(rvk::LogicalDevice* aDevice);
															~GeometryDataVulkan();

	void													prepare(uint32_t aMaxIndices = 2097152,
	                                                                uint32_t aMaxVertices = 524288,
	                                                                uint32_t aIndexBufferUsageFlags = rvk::Buffer::Use::INDEX,
	                                                                uint32_t aVertexBufferUsageFlags = rvk::Buffer::Use::VERTEX);
	void													destroy();

	void													loadScene(rvk::SingleTimeCommand* aStc, tamashii::SceneBackendData aScene);
	void													update(rvk::SingleTimeCommand* aStc, tamashii::SceneBackendData aScene);
	void													unloadScene();

	struct primitveBufferOffset_s {
		uint32_t											mIndexOffset = 0;
		uint32_t											mIndexByteOffset = 0;
		uint32_t											mVertexOffset = 0;
		uint32_t											mVertexByteOffset = 0;
	};

	rvk::Buffer*											getIndexBuffer();
	rvk::Buffer*											getVertexBuffer();
															
	primitveBufferOffset_s									getOffset() const;
	primitveBufferOffset_s									getOffset(Mesh *aMesh);
	primitveBufferOffset_s									getOffset(Model *aModel);

	struct SceneInfo_s {
		uint32_t											mIndexCount;
		uint32_t											mVertexCount;
		uint32_t											mMeshCount;
		uint32_t											mInstanceCount;
		uint32_t											mGeometryCount;
	};
	static SceneInfo_s										getSceneGeometryInfo(tamashii::SceneBackendData aScene);

protected:
	rvk::LogicalDevice*										mDevice;

	uint32_t												mMaxIndexCount;
	uint32_t												mMaxVertexCount;

	rvk::Buffer												mIndexBuffer;
	rvk::Buffer												mVertexBuffer;
	primitveBufferOffset_s									mBufferOffset;			
	std::unordered_map<Model*, primitveBufferOffset_s>		mModelToBOffset;		
	std::unordered_map<Mesh*, primitveBufferOffset_s>		mMeshToBOffset;			
};

class GeometryDataBlasVulkan : public GeometryDataVulkan {
public:
															GeometryDataBlasVulkan(rvk::LogicalDevice* aDevice);
															~GeometryDataBlasVulkan();

															
	void													prepare(uint32_t aMaxIndices = 2097152,
																	uint32_t aMaxVertices = 524288,
																	uint32_t aIndexBufferUsageFlags = rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::AS_INPUT,
																	uint32_t aVertexBufferUsageFlags = rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::AS_INPUT);
	void													destroy();

	void													loadScene(rvk::SingleTimeCommand* aStc, tamashii::SceneBackendData aScene);
	void													unloadScene();

	int														getGeometryIndex(Mesh *aMesh);

	rvk::BottomLevelAS*										getBlas(Model *aModel);
	rvk::BottomLevelAS*										getUnitAabbBlas();
	rvk::BottomLevelAS*										getUnitPlaneBlas();

private:
	rvk::BottomLevelAS*										mUnitAabbBlas;
	rvk::BottomLevelAS*										mUnitPlaneBlas;

	rvk::Buffer												mAsBuffer;
	std::vector<rvk::BottomLevelAS*>						mBottomAs;
	std::unordered_map<Model*, rvk::BottomLevelAS*>			mModelToBlas;
	std::unordered_map<Mesh*, uint32_t>						mMeshToGeometryIndex; 
};

class GeometryDataTlasVulkan {
public:
															GeometryDataTlasVulkan(rvk::LogicalDevice* aDevice);
															~GeometryDataTlasVulkan();

	void													prepare(uint32_t aInstanceCount, uint32_t aGeometryDataBufferFlags = 0, uint32_t aGeometryDataCount = 0);
	void													destroy();

	void													loadScene(rvk::SingleTimeCommand* aStc, tamashii::SceneBackendData aScene, GeometryDataBlasVulkan* blas_gpu, MaterialDataVulkan* md_gpu = nullptr, LightDataVulkan* ld_gpu = nullptr);
	void													unloadScene();
	void													update(const rvk::CommandBuffer* aCmdBuffer, rvk::SingleTimeCommand* aStc, tamashii::SceneBackendData aScene, GeometryDataBlasVulkan* blas_gpu, MaterialDataVulkan* md_gpu = nullptr, LightDataVulkan* ld_gpu = nullptr);

															
	void													add(RefModel *aRefModel, const rvk::BottomLevelAS *aBlas);
	void													add(RefLight *aRefLight, const rvk::BottomLevelAS* aBlas);
	void													build(const rvk::CommandBuffer* aCmdBuffer);
	uint32_t												size() const;
	void													clear();

	rvk::TopLevelAS*										getTlas();
	rvk::Buffer*											getGeometryDataBuffer();
	int														getInstanceIndex(RefModel* aRefModel);
	int														getCustomIndex(RefModel* aRefModel);
	int														getGeometryOffset(RefModel* aRefModel);

	std::deque<RefModel*>&								getModels();
	std::deque<RefLight*>&								getLights();
private:
	rvk::LogicalDevice*										mDevice;
	/*!!! change this struct only in combination with the one in shader/convenience/as_data.glsl/hlsl !!!*/
	static constexpr uint32_t eGeoDataTriangleBit			= 0x00000001u;	
    static constexpr uint32_t eGeoDataIndexedTriangleBit	= 0x00000003u;	
    static constexpr uint32_t eGeoDataIntrinsicBit			= 0x00000004u;	
    static constexpr uint32_t eGeoDataMaterialBit			= 0x00000008u;	
    static constexpr uint32_t eGeoDataLightBit				= 0x00000010u;	
	struct GeometryData_s {
		glm::mat4 mModelMatrix;
		uint32_t mIndexBufferOffset;
		uint32_t mVertexBufferOffset;
		uint32_t mDataIndex;
		uint32_t mFlags;
	};

	uint32_t												mMaxInstanceCount;
	uint32_t												mGeometryOffset;
	std::unordered_map<Ref*, uint32_t>					mRefToInstanceIndex;
	std::unordered_map<Ref*, uint32_t>					mRefToCustomIndex;
	std::unordered_map<RefModel*, uint32_t>				mRefModelToGeometryOffset;
	rvk::TopLevelAS											mTlas;
	std::deque<RefModel*>									mModels;
	std::deque<RefLight*>									mLights;

	uint32_t												mMaxGeometryDataCount;
	std::vector<GeometryData_s>								mGeometryData;
	rvk::Buffer												mGeometryDataBuffer;
};
T_END_NAMESPACE