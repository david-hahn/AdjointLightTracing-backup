#include <tamashii/cuda_helper/convenience/geometry_to_gpu.hpp>
#include <tamashii/engine/scene/ref_entities.hpp>
#include <cuda_runtime.h>
T_USE_NAMESPACE

GeometryDataCuda::GeometryDataCuda() : mMaxIndexCount(0), mMaxVertexCount(0), mIndexBuffer(nullptr), mVertexBuffer(nullptr)
{}

GeometryDataCuda::~GeometryDataCuda()
{
	destroy();
}

void GeometryDataCuda::prepare(const uint32_t aMaxIndices, const uint32_t aMaxVertices)
{
	unloadScene();

	cudaMalloc(&mIndexBuffer, sizeof(uint32_t) * aMaxIndices);
	cudaMalloc(&mVertexBuffer, sizeof(vertex_s) * aMaxVertices);

	mMaxIndexCount = aMaxIndices;
	mMaxVertexCount = aMaxVertices;
}
void GeometryDataCuda::destroy()
{
	cudaFree(mIndexBuffer);
	cudaFree(mVertexBuffer);
	mMaxIndexCount = 0;
	mMaxVertexCount = 0;
	unloadScene();
}
void GeometryDataCuda::loadScene(const tamashii::scene_s aScene)
{
	unloadScene();
	
	{
		
		uint32_t mcount = 0;
		uint32_t icount = 0;
		uint32_t vcount = 0;
		for (Model* model : aScene.models) {
			for (const Mesh* mesh : *model) {
				mcount++;
				if (mesh->hasIndices()) icount += mesh->getIndexCount();
				vcount += mesh->getVertexCount();
			}
		}
		mModelToBOffset.reserve(aScene.models.size());
		mMeshToBOffset.reserve(mcount);
		if (vcount) {
			
			primitveBufferOffset_s offsets = {};
			for (Model* model : aScene.models) {
				mModelToBOffset.insert(std::pair(model, offsets));
				for (Mesh* mesh : *model) {
					mMeshToBOffset.insert(std::pair(mesh, offsets));
					
					if (mesh->hasIndices()) {
						cudaMemcpy(mIndexBuffer, offsets.mIndexByteOffset + mesh->getIndicesArray(), mesh->getIndexCount() * sizeof(uint32_t), cudaMemcpyHostToDevice);
						offsets.mIndexOffset += mesh->getIndexCount();
						offsets.mIndexByteOffset += mesh->getIndexCount() * sizeof(uint32_t);
						if (offsets.mIndexOffset > mMaxIndexCount) spdlog::error("Indices count > buffer size");
					}
					
					cudaMemcpy(mVertexBuffer, offsets.mVertexByteOffset + mesh->getVerticesArray(), mesh->getVertexCount() * sizeof(vertex_s), cudaMemcpyHostToDevice);
					offsets.mVertexOffset += mesh->getVertexCount();
					offsets.mVertexByteOffset += mesh->getVertexCount() * sizeof(vertex_s);
					if (offsets.mVertexOffset > mMaxVertexCount) spdlog::error("Vertices count > buffer size");
				}
			}
			mBufferOffset = offsets;
		}
	}
}

void GeometryDataCuda::update(const scene_s aScene)
{
	unloadScene();
	loadScene(aScene);
}

void GeometryDataCuda::unloadScene()
{
	mModelToBOffset.clear();
	mMeshToBOffset.clear();
	mBufferOffset = {};
}

uint32_t* GeometryDataCuda::getIndexBuffer() const
{
	return mIndexBuffer;
}

vertex_s* GeometryDataCuda::getVertexBuffer() const
{
	return mVertexBuffer;
}

GeometryDataCuda::primitveBufferOffset_s GeometryDataCuda::getOffset() const
{
	return mBufferOffset;
}

GeometryDataCuda::primitveBufferOffset_s GeometryDataCuda::getOffset(Mesh* aMesh)
{
	return mMeshToBOffset[aMesh];
}

GeometryDataCuda::primitveBufferOffset_s GeometryDataCuda::getOffset(Model* aModel)
{
	return mModelToBOffset[aModel];
}

GeometryDataCuda::SceneInfo_s GeometryDataCuda::getSceneGeometryInfo(const tamashii::scene_s aScene)
{
	SceneInfo_s info{};
	for (Model* model : aScene.models) {
		for (const Mesh* mesh : *model) {
			info.mMeshCount++;
			if (mesh->hasIndices()) info.mIndexCount += mesh->getIndexCount();
			info.mVertexCount += mesh->getVertexCount();
		}
	}
	info.mInstanceCount = aScene.refModels.size();

	for (const RefModel_s* refModel : aScene.refModels) {
		for (RefMesh_s* refMesh : refModel->refMeshes) {
			info.mGeometryCount++;
		}
	}
	return info;
}
