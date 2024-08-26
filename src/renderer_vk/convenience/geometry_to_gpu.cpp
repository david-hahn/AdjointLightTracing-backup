#include <tamashii/renderer_vk/convenience/geometry_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/material_to_gpu.hpp>
#include <tamashii/renderer_vk/convenience/light_to_gpu.hpp>
#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/light.hpp>

T_USE_NAMESPACE

GeometryDataVulkan::GeometryDataVulkan(rvk::LogicalDevice* aDevice) : mDevice(aDevice),
                                                                      mMaxIndexCount(0), mMaxVertexCount(0), mIndexBuffer(aDevice), mVertexBuffer(aDevice)
{}

GeometryDataVulkan::~GeometryDataVulkan()
{ destroy(); }

void GeometryDataVulkan::prepare(const uint32_t aMaxIndices,
                               const uint32_t aMaxVertices,
                               const uint32_t aIndexBufferUsageFlags,
                               const uint32_t aVertexBufferUsageFlags)
{
	unloadScene();
	mMaxIndexCount = std::max(1u, aMaxIndices);
	mMaxVertexCount = std::max(1u, aMaxVertices);
	mIndexBuffer.create(aIndexBufferUsageFlags, mMaxIndexCount * sizeof(uint32_t), rvk::Buffer::Location::DEVICE);
	mVertexBuffer.create(aVertexBufferUsageFlags, mMaxVertexCount * sizeof(vertex_s), rvk::Buffer::Location::DEVICE);
}
void GeometryDataVulkan::destroy()
{
	mIndexBuffer.destroy();
	mVertexBuffer.destroy();
	mMaxIndexCount = 0;
	mMaxVertexCount = 0;
	unloadScene();
}
void GeometryDataVulkan::loadScene(rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData aScene)
{
	unloadScene();
	
	{
		
		uint32_t mcount = 0;
		uint32_t icount = 0;
		uint32_t vcount = 0;
		for (auto& model : aScene.models) {
			for (const auto& mesh : *model) {
				mcount++;
				if (mesh->hasIndices()) icount += mesh->getIndexCount();
				vcount += mesh->getVertexCount();
			}
		}
		mModelToBOffset.reserve(aScene.models.size());
		mMeshToBOffset.reserve(mcount);
		if (vcount) {
			
			primitveBufferOffset_s offsets = {};
			for (auto& model : aScene.models) {
				
				mModelToBOffset.insert(std::pair(model.get(), offsets));
				for (const auto& mesh : *model) {
					mMeshToBOffset.insert(std::pair(mesh.get(), offsets));
					
					if (mesh->hasIndices()) {
						mIndexBuffer.STC_UploadData(aStc, mesh->getIndicesArray(), mesh->getIndexCount() * sizeof(uint32_t), offsets.mIndexByteOffset);
						offsets.mIndexOffset += mesh->getIndexCount();
						offsets.mIndexByteOffset += mesh->getIndexCount() * sizeof(uint32_t);
						if (offsets.mIndexOffset > mMaxIndexCount) spdlog::error("Indices count > buffer size");
					}
					
					mVertexBuffer.STC_UploadData(aStc, mesh->getVerticesArray(), mesh->getVertexCount() * sizeof(vertex_s), offsets.mVertexByteOffset);
					offsets.mVertexOffset += mesh->getVertexCount();
					offsets.mVertexByteOffset += mesh->getVertexCount() * sizeof(vertex_s);
					if (offsets.mVertexOffset > mMaxVertexCount) spdlog::error("Vertices count > buffer size");
				}
			}
			mBufferOffset = offsets;
		}
	}
}

void GeometryDataVulkan::update(rvk::SingleTimeCommand* aStc, const SceneBackendData aScene)
{
	unloadScene();
	loadScene(aStc, aScene);
}

void GeometryDataVulkan::unloadScene()
{
	mModelToBOffset.clear();
	mMeshToBOffset.clear();
	mBufferOffset = {};
}

rvk::Buffer* GeometryDataVulkan::getIndexBuffer()
{ return &mIndexBuffer; }

rvk::Buffer* GeometryDataVulkan::getVertexBuffer()
{ return &mVertexBuffer; }

GeometryDataVulkan::primitveBufferOffset_s GeometryDataVulkan::getOffset() const
{ return mBufferOffset; }

GeometryDataVulkan::primitveBufferOffset_s GeometryDataVulkan::getOffset(Mesh* aMesh)
{ return mMeshToBOffset[aMesh]; }

GeometryDataVulkan::primitveBufferOffset_s GeometryDataVulkan::getOffset(Model* aModel)
{ return mModelToBOffset[aModel]; }

GeometryDataVulkan::SceneInfo_s GeometryDataVulkan::getSceneGeometryInfo(const tamashii::SceneBackendData aScene)
{
	SceneInfo_s info{};
	for (auto& model : aScene.models) {
		for (const auto& mesh : *model) {
			info.mMeshCount++;
			if (mesh->hasIndices()) info.mIndexCount += mesh->getIndexCount();
			info.mVertexCount += mesh->getVertexCount();
		}
	}
	info.mInstanceCount = static_cast<uint32_t>(aScene.refModels.size());
	info.mInstanceCount += static_cast<uint32_t>(aScene.refLights.size());

	for (const auto& refModel : aScene.refModels) {
		info.mGeometryCount += refModel->refMeshes.size();
	}
	info.mGeometryCount += static_cast<uint32_t>(aScene.refLights.size());
	return info;
}

GeometryDataBlasVulkan::GeometryDataBlasVulkan(rvk::LogicalDevice* aDevice) : GeometryDataVulkan(aDevice), mUnitAabbBlas(nullptr), mUnitPlaneBlas(nullptr), mAsBuffer(aDevice)
{}

GeometryDataBlasVulkan::~GeometryDataBlasVulkan()
{ destroy(); }

void GeometryDataBlasVulkan::prepare(const uint32_t aMaxIndices, 
                                   const uint32_t aMaxVertices, 
                                   const uint32_t aIndexBufferUsageFlags, 
                                   const uint32_t aVertexBufferUsageFlags)
{
	GeometryDataVulkan::prepare(aMaxIndices, aMaxVertices, aIndexBufferUsageFlags, aVertexBufferUsageFlags);
}

void GeometryDataBlasVulkan::destroy()
{
	unloadScene();
	GeometryDataVulkan::destroy();
}

void GeometryDataBlasVulkan::loadScene(rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData aScene)
{
	unloadScene();
	GeometryDataVulkan::loadScene(aStc, aScene);

	rvk::Buffer aabbBuffer(mDevice);
	constexpr float surface[24] = { /*minX*/ -1.0f, /*minY*/ -1.0f, /*minZ*/ -1.0f, /*maxX*/ 1.0f, /*maxY*/ 1.0f, /*maxZ*/ 1.0f,
									-0.5f, -0.5f, 0.0f, /**/ -0.5f, 0.5f, 0.0f, /**/ 0.5f, -0.5f, 0.0f,
									 0.5f, 0.5f, 0.0f, /**/ 0.5f, -0.5f, 0.0f, /**/ -0.5f, 0.5f, 0.0f };
	aabbBuffer.create(rvk::Buffer::AS_INPUT, sizeof(surface), rvk::Buffer::Location::DEVICE);
	aabbBuffer.STC_UploadData(aStc, &surface);

	uint32_t geometry_count = 0;
	for (const auto& model : aScene.models) {
		geometry_count += model->getMeshList().size();
	}
    if(!geometry_count) return;

	mBottomAs.reserve(aScene.models.size());
	mModelToBlas.reserve(aScene.models.size());
	mMeshToGeometryIndex.reserve(geometry_count);

	uint32_t scratch_buffer_size = 0;
	uint32_t as_buffer_size = 0;
	uint32_t geometry_index = 0;
	
	for (auto& model : aScene.models) {
		
		auto blas = new rvk::BottomLevelAS(mDevice);
		blas->reserve(static_cast<uint32_t>(model->getMeshList().size()));
		geometry_index = 0;
		for (const auto& mesh : *model) {
			mMeshToGeometryIndex.insert(std::pair(mesh.get(), geometry_index));
			geometry_index++;

			
			rvk::ASTriangleGeometry astri;
			if (mesh->hasIndices()) astri.setIndicesFromDevice(VK_INDEX_TYPE_UINT32, static_cast<uint32_t>(mesh->getIndexCount()), &mIndexBuffer, mMeshToBOffset[mesh.get()].mIndexByteOffset);
			astri.setVerticesFromDevice(VK_FORMAT_R32G32B32_SFLOAT, sizeof(vertex_s), static_cast<uint32_t>(mesh->getVertexCount()), &mVertexBuffer, mMeshToBOffset[mesh.get()].mVertexByteOffset);

			blas->addGeometry(astri, mesh->getMaterial()->getBlendMode() == Material::BlendMode::_OPAQUE ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0);
		}
		
		blas->preprepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
		mBottomAs.push_back(blas);
		
		mModelToBlas.insert(std::pair(model.get(), blas));

		as_buffer_size += rountUpToMultipleOf<uint32_t>(blas->getASSize(), 256);
		scratch_buffer_size += rountUpToMultipleOf<uint32_t>(blas->getBuildScratchSize(), mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
	}

	
	mUnitAabbBlas = new rvk::BottomLevelAS(mDevice);
	rvk::ASAABBGeometry aabbgeo;
	aabbgeo.setAABBsFromDevice(&aabbBuffer);
	mUnitAabbBlas->addGeometry(aabbgeo, VK_GEOMETRY_OPAQUE_BIT_KHR);
	mUnitAabbBlas->preprepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	as_buffer_size += rountUpToMultipleOf<uint32_t>(mUnitAabbBlas->getASSize(), 256);
	scratch_buffer_size += rountUpToMultipleOf<uint32_t>(mUnitAabbBlas->getBuildScratchSize(), mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
	
	mUnitPlaneBlas = new rvk::BottomLevelAS(mDevice);
	rvk::ASTriangleGeometry trianglegeo;
	trianglegeo.setVerticesFromDevice(VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float), 6u, &aabbBuffer, sizeof(VkAabbPositionsKHR));
	mUnitPlaneBlas->addGeometry(trianglegeo, VK_GEOMETRY_OPAQUE_BIT_KHR);
	mUnitPlaneBlas->preprepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	as_buffer_size += rountUpToMultipleOf<uint32_t>(mUnitPlaneBlas->getASSize(), 256);
	scratch_buffer_size += rountUpToMultipleOf<uint32_t>(mUnitPlaneBlas->getBuildScratchSize(), mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);

	mAsBuffer.create(rvk::Buffer::AS_STORE, as_buffer_size, rvk::Buffer::Location::DEVICE);
	rvk::Buffer scratchBuffer(mDevice);
	scratchBuffer.create(rvk::Buffer::AS_SCRATCH, scratch_buffer_size, rvk::Buffer::Location::DEVICE);

	uint32_t scratchBufferOffset = 0;
	uint32_t asBufferOffset = 0;
	aStc->begin();
	for (auto& model : aScene.models) {
		auto* modelPtr = model.get();
		mModelToBlas[modelPtr]->setASBuffer(&mAsBuffer, asBufferOffset);
		mModelToBlas[modelPtr]->setScratchBuffer(&scratchBuffer, scratchBufferOffset);
		mModelToBlas[modelPtr]->prepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
		mModelToBlas[modelPtr]->CMD_Build(aStc->buffer());
		asBufferOffset += rountUpToMultipleOf<uint32_t>(mModelToBlas[modelPtr]->getASSize(), 256);
		scratchBufferOffset += rountUpToMultipleOf<uint32_t>(mModelToBlas[modelPtr]->getBuildScratchSize(), mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
	}
	mUnitAabbBlas->setASBuffer(&mAsBuffer, asBufferOffset);
	mUnitAabbBlas->setScratchBuffer(&scratchBuffer, scratchBufferOffset);
	mUnitAabbBlas->prepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	mUnitAabbBlas->CMD_Build(aStc->buffer());
	asBufferOffset += rountUpToMultipleOf<uint32_t>(mUnitAabbBlas->getASSize(), 256);
	scratchBufferOffset += rountUpToMultipleOf<uint32_t>(mUnitAabbBlas->getBuildScratchSize(), mDevice->getPhysicalDevice()->getAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment);
	mUnitPlaneBlas->setASBuffer(&mAsBuffer, asBufferOffset);
	mUnitPlaneBlas->setScratchBuffer(&scratchBuffer, scratchBufferOffset);
	mUnitPlaneBlas->prepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	mUnitPlaneBlas->CMD_Build(aStc->buffer());

	aStc->end();

	scratchBuffer.destroy();
}

void GeometryDataBlasVulkan::unloadScene()
{
	for (const rvk::BottomLevelAS *blas : mBottomAs) {
		delete blas;
	}
	delete mUnitAabbBlas;
	mUnitAabbBlas = nullptr;
	delete mUnitPlaneBlas;
	mUnitPlaneBlas = nullptr;
	mBottomAs.clear();
	mModelToBlas.clear();
	mMeshToGeometryIndex.clear();
	mAsBuffer.destroy();
}

int GeometryDataBlasVulkan::getGeometryIndex(Mesh *aMesh)
{
	if (aMesh == nullptr || (mMeshToGeometryIndex.find(aMesh) == mMeshToGeometryIndex.end())) return -1;
	return static_cast<int>(mMeshToGeometryIndex[aMesh]);
}

rvk::BottomLevelAS* GeometryDataBlasVulkan::getBlas(Model *aModel)
{
	if (aModel == nullptr || (mModelToBlas.find(aModel) == mModelToBlas.end())) return nullptr;
	return mModelToBlas[aModel];
}

rvk::BottomLevelAS* GeometryDataBlasVulkan::getUnitAabbBlas()
{
	return mUnitAabbBlas->isBuild() ? mUnitAabbBlas : nullptr;
}

rvk::BottomLevelAS* GeometryDataBlasVulkan::getUnitPlaneBlas()
{
	return mUnitPlaneBlas->isBuild() ? mUnitPlaneBlas : nullptr;
}

GeometryDataTlasVulkan::GeometryDataTlasVulkan(rvk::LogicalDevice* aDevice) : mDevice(aDevice),
                                                                              mMaxInstanceCount(0), mGeometryOffset(0), mTlas(aDevice), mMaxGeometryDataCount(0),
                                                                              mGeometryDataBuffer(aDevice)
{
}

GeometryDataTlasVulkan::~GeometryDataTlasVulkan()
{ destroy(); }

void GeometryDataTlasVulkan::prepare(const uint32_t aInstanceCount, const uint32_t aGeometryDataBufferFlags, const uint32_t aGeometryDataCount)
{
	mRefToInstanceIndex.reserve(aGeometryDataCount);
	mRefModelToGeometryOffset.reserve(aGeometryDataCount);
	mRefToCustomIndex.reserve(aGeometryDataCount);
	mTlas.reserve(aInstanceCount);
	mGeometryOffset = 0;
	mMaxInstanceCount = aInstanceCount;

	if (aGeometryDataCount) {
		mGeometryData.reserve(aGeometryDataCount);
		mGeometryDataBuffer.create(aGeometryDataBufferFlags, aGeometryDataCount * sizeof(GeometryData_s), rvk::Buffer::Location::DEVICE);
		mMaxGeometryDataCount = aGeometryDataCount;
	}
}


void GeometryDataTlasVulkan::destroy()
{
	unloadScene();
	mTlas.destroy();
	mMaxInstanceCount = 0;

	mMaxGeometryDataCount = 0;
	mGeometryDataBuffer.destroy();
}


void GeometryDataTlasVulkan::loadScene(rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData aScene, GeometryDataBlasVulkan* blas_gpu, MaterialDataVulkan* md_gpu, LightDataVulkan* ld_gpu)
{
	unloadScene();
	if (aScene.refModels.empty()) return;
	for (auto& refModel : aScene.refModels) {
		
		add(refModel.get(), blas_gpu->getBlas(refModel->model.get()));
		if (mMaxGeometryDataCount) {
			GeometryData_s data{};
			data.mModelMatrix = refModel->model_matrix;
			for (const auto& refMesh : refModel->refMeshes) {
				const GeometryDataVulkan::primitveBufferOffset_s offset = blas_gpu->getOffset(refMesh->mesh.get());
				data.mIndexBufferOffset = offset.mIndexOffset;
				data.mVertexBufferOffset = offset.mVertexOffset;
				data.mFlags = refMesh->mesh->hasIndices() ? eGeoDataIndexedTriangleBit : eGeoDataTriangleBit;
				if (md_gpu) {
					data.mFlags |= eGeoDataMaterialBit;
					data.mDataIndex = md_gpu->getIndex(refMesh->mesh->getMaterial());
				}
				mGeometryData.push_back(data);
			}
		}
	}
	if (ld_gpu) {
		for (auto& refLight : aScene.refLights) {
			if (refLight->light->getType() == Light::Type::POINT) {
				const auto& pl = dynamic_cast<PointLight&>(*refLight->light);
				if (pl.getRadius() == 0.0f) continue;
				
				add(refLight.get(), blas_gpu->getUnitAabbBlas());
			}
			else if (refLight->light->getType() == Light::Type::SPOT) {
				const auto& sl = dynamic_cast<SpotLight&>(*refLight->light);
				if (sl.getRadius() == 0.0f) continue;
				
				add(refLight.get(), blas_gpu->getUnitAabbBlas());
			}
			else if (refLight->light->getType() == Light::Type::IES)
			{
				const auto& ies = dynamic_cast<IESLight&>(*refLight->light);
				if (ies.getRadius() == 0.0f) continue;
				
				add(refLight.get(), blas_gpu->getUnitAabbBlas());
			}
			else if (refLight->light->getType() == Light::Type::SURFACE)
			{
				const auto& sl = dynamic_cast<SurfaceLight&>(*refLight->light);
				if (sl.getShape() != SurfaceLight::Shape::SQUARE && sl.getShape() != SurfaceLight::Shape::RECTANGLE) continue;
				
				add(refLight.get(), blas_gpu->getUnitPlaneBlas());
			}
			else continue;

			if (mMaxGeometryDataCount) {
				GeometryData_s data{};
				data.mModelMatrix = refLight->model_matrix;
				data.mFlags = eGeoDataIntrinsicBit | eGeoDataLightBit;
				
				data.mDataIndex = ld_gpu->getIndex(refLight.get());
				mGeometryData.push_back(data);
			}
		}
	}
	if(!mGeometryData.empty()) mGeometryDataBuffer.STC_UploadData(aStc, mGeometryData.data(), mGeometryData.size() * sizeof(GeometryData_s));

	aStc->begin();
	mTlas.prepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);
	mTlas.CMD_Build(aStc->buffer(), nullptr);
	aStc->end();
}

void GeometryDataTlasVulkan::unloadScene()
{
	clear();
	mTlas.destroy();
}

void GeometryDataTlasVulkan::update(const rvk::CommandBuffer* aCmdBuffer, rvk::SingleTimeCommand* aStc, const tamashii::SceneBackendData aScene, GeometryDataBlasVulkan* blas_gpu, MaterialDataVulkan* md_gpu, LightDataVulkan* ld_gpu)
{
	clear();
	if (aScene.refModels.empty()) return;
	for (auto& refModel : aScene.refModels) {
		
		add(refModel.get(), blas_gpu->getBlas(refModel->model.get()));
		if (mMaxGeometryDataCount) {
			GeometryData_s data{};
			data.mModelMatrix = refModel->model_matrix;
			for (const auto& refMesh : refModel->refMeshes) {
				const GeometryDataVulkan::primitveBufferOffset_s offset = blas_gpu->getOffset(refMesh->mesh.get());
				data.mIndexBufferOffset = offset.mIndexOffset;
				data.mVertexBufferOffset = offset.mVertexOffset;
				data.mFlags = refMesh->mesh->hasIndices() ? eGeoDataIndexedTriangleBit : eGeoDataTriangleBit;
				if (md_gpu) {
					data.mFlags |= eGeoDataMaterialBit;
					data.mDataIndex = md_gpu->getIndex(refMesh->mesh->getMaterial());
				}
				mGeometryData.push_back(data);
				if (mGeometryData.size() > mMaxGeometryDataCount) spdlog::error("Geometry count > buffer size");
			}
		}
	}
	if (ld_gpu) {
		for (auto& refLight : aScene.refLights) {
			if (refLight->light->getType() == Light::Type::POINT) {
				const auto& pl = dynamic_cast<PointLight&>(*refLight->light);
				if (pl.getRadius() == 0.0f) continue;
				
				add(refLight.get(), blas_gpu->getUnitAabbBlas());
			}
			else if (refLight->light->getType() == Light::Type::SPOT) {
				const auto& sl = dynamic_cast<SpotLight&>(*refLight->light);
				if (sl.getRadius() == 0.0f) continue;
				
				add(refLight.get(), blas_gpu->getUnitAabbBlas());
			}
			else if (refLight->light->getType() == Light::Type::IES) {
				const auto& ies = dynamic_cast<IESLight&>(*refLight->light);
				if (ies.getRadius() == 0.0f) continue;
				
				add(refLight.get(), blas_gpu->getUnitAabbBlas());
			}
			else if (refLight->light->getType() == Light::Type::SURFACE) {
				const auto& sl = dynamic_cast<SurfaceLight&>(*refLight->light);
				if (sl.getShape() != SurfaceLight::Shape::SQUARE && sl.getShape() != SurfaceLight::Shape::RECTANGLE) continue;
				
				add(refLight.get(), blas_gpu->getUnitPlaneBlas());
			}
			else continue;

			if (mMaxGeometryDataCount) {
				GeometryData_s data{};
				data.mModelMatrix = refLight->model_matrix;
				data.mFlags = eGeoDataIntrinsicBit | eGeoDataLightBit;
				
				data.mDataIndex = ld_gpu->getIndex(refLight.get());
				mGeometryData.push_back(data);
			}
		}
	}
	if (!mGeometryData.empty()) mGeometryDataBuffer.STC_UploadData(aStc, mGeometryData.data(), mGeometryData.size() * sizeof(GeometryData_s));

	mTlas.prepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);
	mTlas.CMD_Build(aCmdBuffer, nullptr);
}

void GeometryDataTlasVulkan::add(RefModel* aRefModel, const rvk::BottomLevelAS* aBlas)
{
	mModels.push_back(aRefModel);
	
	glm::mat4 modelMatrix = glm::transpose(aRefModel->model_matrix);
	
	rvk::ASInstance asInstance(aBlas);
	asInstance.setTransform(&modelMatrix[0][0]);
	asInstance.setCustomIndex(static_cast<uint32_t>(mGeometryData.size()));
	asInstance.setMask(aRefModel->mask);

	bool cull = true;
	for(const auto& mesh : aRefModel->refMeshes) cull &= mesh->mesh->getMaterial()->getCullBackface();
	if (!cull) asInstance.setFlags(VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR);

	if (mRefToInstanceIndex.find(aRefModel) == mRefToInstanceIndex.end()) {
		mRefToInstanceIndex.insert(std::pair(aRefModel, mTlas.size()));
		mRefToCustomIndex.insert(std::pair(aRefModel, static_cast<uint32_t>(mGeometryData.size())));
		mRefModelToGeometryOffset.insert(std::pair(aRefModel, mGeometryOffset));

		mTlas.addInstance(asInstance);
	} else {
		mTlas.replaceInstance(mRefToInstanceIndex[aRefModel], asInstance);
	}
	mGeometryOffset += aBlas->size();
	if (mTlas.size() > mMaxInstanceCount) spdlog::error("Instance count > tlas size");
}

void GeometryDataTlasVulkan::add(RefLight* aRefLight, const rvk::BottomLevelAS* aBlas)
{
	uint32_t sbtoffset = 0;
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	if (aRefLight->light->getType() == Light::Type::POINT) {
		const auto& pl = dynamic_cast<PointLight&>(*aRefLight->light);
		if (pl.getRadius() == 0.0f) return;
		modelMatrix = glm::mat4(pl.getRadius());
		sbtoffset = 1;
	}
	else if (aRefLight->light->getType() == Light::Type::SPOT) {
		const auto& sl = dynamic_cast<SpotLight&>(*aRefLight->light);
		if (sl.getRadius() == 0.0f) return;
		modelMatrix = glm::mat4(sl.getRadius());
		sbtoffset = 1;
	}
	else if (aRefLight->light->getType() == Light::Type::IES) {
		const auto& ies = dynamic_cast<IESLight&>(*aRefLight->light);
		if (ies.getRadius() == 0.0f) return;
		modelMatrix = glm::mat4(ies.getRadius());
		sbtoffset = 1;
	}
	else if (aRefLight->light->getType() == Light::Type::SURFACE) {
		const auto& sl = dynamic_cast<SurfaceLight&>(*aRefLight->light);
		if (sl.getShape() != SurfaceLight::Shape::SQUARE && sl.getShape() != SurfaceLight::Shape::RECTANGLE) return;
		modelMatrix = glm::transpose(aRefLight->model_matrix);
	}
	else return;

	mLights.push_back(aRefLight);
	
	modelMatrix[0][3] = aRefLight->position.x;
	modelMatrix[1][3] = aRefLight->position.y;
	modelMatrix[2][3] = aRefLight->position.z;
	
	rvk::ASInstance asInstance(aBlas);
	asInstance.setTransform(&modelMatrix[0][0]);
	asInstance.setCustomIndex(static_cast<uint32_t>(mGeometryData.size()));
	asInstance.setMask(aRefLight->mask);
	asInstance.setSBTRecordOffset(sbtoffset);

	if (mRefToInstanceIndex.find(aRefLight) == mRefToInstanceIndex.end()) {
		mRefToInstanceIndex.insert(std::pair(aRefLight, mTlas.size()));
		mRefToCustomIndex.insert(std::pair(aRefLight, static_cast<uint32_t>(mGeometryData.size())));

		mTlas.addInstance(asInstance);
	}
	else {
		mTlas.replaceInstance(mRefToInstanceIndex[aRefLight], asInstance);
	}
	mGeometryOffset += aBlas->size();
	if (mTlas.size() > mMaxInstanceCount) spdlog::error("Instance count > tlas size");
}

void GeometryDataTlasVulkan::build(const rvk::CommandBuffer* aCmdBuffer)
{
	if (mTlas.isBuild()) {
		mTlas.CMD_Build(aCmdBuffer, mTlas.isBuild() ? &mTlas : nullptr);
	}
	else {
		mTlas.prepare(VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);
		mTlas.CMD_Build(aCmdBuffer, nullptr);
	}
}

uint32_t GeometryDataTlasVulkan::size() const
{ return mTlas.size(); }

rvk::TopLevelAS* GeometryDataTlasVulkan::getTlas()
{ return &mTlas; }

rvk::Buffer* GeometryDataTlasVulkan::getGeometryDataBuffer()
{ return &mGeometryDataBuffer; }

void GeometryDataTlasVulkan::clear()
{
	mTlas.clear();
	mRefToInstanceIndex.clear();
	mRefModelToGeometryOffset.clear();
	mRefToCustomIndex.clear();
	mGeometryOffset = 0;
	mModels.clear();
	mLights.clear();
	mGeometryData.clear();
}

int GeometryDataTlasVulkan::getInstanceIndex(RefModel* aRefModel)
{
	if (aRefModel == nullptr || (mRefToInstanceIndex.find(aRefModel) == mRefToInstanceIndex.end())) return -1;
	return static_cast<int>(mRefToInstanceIndex[aRefModel]);
}

int GeometryDataTlasVulkan::getCustomIndex(RefModel* aRefModel)
{
	if (aRefModel == nullptr || (mRefToCustomIndex.find(aRefModel) == mRefToCustomIndex.end())) return -1;
	return static_cast<int>(mRefToCustomIndex[aRefModel]);
}

int GeometryDataTlasVulkan::getGeometryOffset(RefModel* aRefModel)
{
	if (aRefModel == nullptr || (mRefModelToGeometryOffset.find(aRefModel) == mRefModelToGeometryOffset.end())) return -1;
	return static_cast<int>(mRefModelToGeometryOffset[aRefModel]);
}

std::deque<RefModel*>& GeometryDataTlasVulkan::getModels()
{
	return mModels;
}

std::deque<RefLight*>& GeometryDataTlasVulkan::getLights()
{
	return mLights;
}
