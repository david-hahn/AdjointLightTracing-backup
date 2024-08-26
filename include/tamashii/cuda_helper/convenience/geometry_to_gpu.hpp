#pragma once
#include <tamashii/engine/scene/render_scene.hpp>

#include <unordered_map>

T_BEGIN_NAMESPACE
class GeometryDataCuda {
public:
	GeometryDataCuda();
	~GeometryDataCuda();

	void													prepare(uint32_t aMaxIndices = 2097152, uint32_t aMaxVertices = 524288);
	void													destroy();

	void													loadScene(tamashii::scene_s aScene);
	void													update(tamashii::scene_s aScene);
	void													unloadScene();

	struct primitveBufferOffset_s {
		uint32_t											mIndexOffset = 0;
		uint32_t											mIndexByteOffset = 0;
		uint32_t											mVertexOffset = 0;
		uint32_t											mVertexByteOffset = 0;
	};

	uint32_t*												getIndexBuffer() const;
	vertex_s*												getVertexBuffer() const;
	
	primitveBufferOffset_s									getOffset() const;
	primitveBufferOffset_s									getOffset(Mesh* aMesh);
	primitveBufferOffset_s									getOffset(Model* aModel);

	struct SceneInfo_s {
		uint32_t											mIndexCount;
		uint32_t											mVertexCount;
		uint32_t											mMeshCount;
		uint32_t											mInstanceCount;
		uint32_t											mGeometryCount;
	};
	static SceneInfo_s										getSceneGeometryInfo(tamashii::scene_s aScene);

protected:
	uint32_t												mMaxIndexCount;
	uint32_t												mMaxVertexCount;

	uint32_t*												mIndexBuffer;
	vertex_s*												mVertexBuffer;
	primitveBufferOffset_s									mBufferOffset;			
	std::unordered_map<Model*, primitveBufferOffset_s>		mModelToBOffset;		
	std::unordered_map<Mesh*, primitveBufferOffset_s>		mMeshToBOffset;			
};
T_END_NAMESPACE
