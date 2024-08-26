#include <tamashii/core/io/io.hpp>

#include <stb_image.h>
#include <nlohmann/json.hpp>
#include <tiny_gltf.h>

#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/camera.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/scene_graph.hpp>

#include <filesystem>
#include <numbers>


#define VEC3_TO_VECTOR(vec3) {(vec3).x, (vec3).y, (vec3).z}
#define VEC4_TO_VECTOR(vec4) {(vec4).x, (vec4).y, (vec4).z, (vec4).w}
#define MAP_INSIDE(map, key) ((map).find(key) != (map).end())

T_USE_NAMESPACE
namespace {
	uint64_t rountUpToMultipleOf(const uint64_t aNumToRound, const uint64_t aMultiple) { return (aNumToRound + aMultiple - 1) & -aMultiple; }
	constexpr uint64_t ROUND_UP_CONST = 4;

	std::unordered_map<tamashii::Image*, int> imageToIndexDirectory = {};
	std::unordered_map<tamashii::Texture*, int> textureToIndexDirectory = {};
	std::unordered_map<tamashii::Material*, int> materialToIndexDirectory = {};
	std::unordered_map<tamashii::Model*, int> modelToIndexDirectory = {};
	std::unordered_map<tamashii::Camera*, int> cameraToIndexDirectory = {};
	std::unordered_map<tamashii::Light*, int> lightToIndexDirectory = {};

	std::deque<std::string> usedImageFilepaths = {};

	struct gltfBuffer {
		std::vector<uint8_t> &mData;
		uint64_t mPointer;
	};

	
	void textureFormatToTinygltfImage(tinygltf::Image& aImg, Image::Format aFormat);
	std::string filenameToMimeType(const std::filesystem::path& aPath);
	int textureWrapToTinygltfTextureWrapType(Sampler::Wrap aTextureWrap);
	int textureFilterToTinygltfTextureFilter(Sampler::Filter aTextureFilter, bool aMipMap, Sampler::Filter aTextureMipMapFilter);
	const char* getBlendMode(Material::BlendMode aBlendMode);
	int topologyToTinygltfMode(Mesh::Topology aTopology);
	const char* interpolationToString(tamashii::TRS::Interpolation aInterpolation);

	
	tinygltf::Value exportValues(const tamashii::Value& aSrc) {
		switch (aSrc.getType())
		{
			case Value::Type::EMPTY: return {};
			case Value::Type::INT: return tinygltf::Value(aSrc.getInt());
			case Value::Type::BOOL: return tinygltf::Value(aSrc.getBool());
			case Value::Type::FLOAT: return tinygltf::Value(static_cast<double>(aSrc.getFloat()));
			case Value::Type::STRING: return tinygltf::Value(aSrc.getString());
			case Value::Type::BINARY: return tinygltf::Value(aSrc.getBinary());
			case Value::Type::ARRAY:
			{
				auto srcValues = aSrc.getArray();
				std::vector<tinygltf::Value> dstValues;
				dstValues.reserve(srcValues.size());
				for (tamashii::Value& srcValue : srcValues) {
					dstValues.emplace_back(exportValues(srcValue));
				}
				return tinygltf::Value(dstValues);
			}
			case Value::Type::MAP:
			{
				const auto srcValues = aSrc.getMap();
				std::map<std::string, tinygltf::Value> dstValues;
				for (auto& srcValue : srcValues) {
					dstValues.emplace(srcValue.first, exportValues(srcValue.second));
				}
				return tinygltf::Value(dstValues);
			}
		}
		return {};
	}
	void exportCustomProperties(tamashii::Asset& aSrc, tinygltf::Value* aDst) {
		const std::map<std::string, tamashii::Value>* srcMap = aSrc.getCustomPropertyMap();
		std::map<std::string, tinygltf::Value> dstMap;
		for (auto& pair : *srcMap) {
			dstMap.emplace(pair.first, exportValues(pair.second));
		}
		if(!dstMap.empty()) *aDst = tinygltf::Value(dstMap);
	}

	int exportImage(const std::filesystem::path& aOutDir, tinygltf::Model& aGltfModel, tamashii::Image* aImage)
	{
		if (aImage->getName().empty()) aImage->setName("image");
		if (aImage->getFilepath().empty()) aImage->setFilepath("image.png");

		const std::string outExtension = std::filesystem::path(aImage->getFilepath()).extension().string();
		const std::string outStemOrignal = std::filesystem::path(aImage->getFilepath()).stem().string();
		std::string outStem = outStemOrignal + outExtension;
		std::string outFilepath = std::filesystem::path(aOutDir.string() + "/" + outStem).make_preferred().string();

		
		uint32_t appendix = 0;
		while (std::find(usedImageFilepaths.begin(), usedImageFilepaths.end(), outFilepath) != usedImageFilepaths.end()) {
			appendix++; outStem = outStemOrignal + "_" + std::to_string(appendix) + outExtension;
			outFilepath = std::filesystem::path(aOutDir.string() + "/" + outStem).make_preferred().string();
		}
		usedImageFilepaths.push_back(outFilepath);

		const std::filesystem::path outRelativeFilepath = std::filesystem::relative(outFilepath, aOutDir).string();
		tinygltf::BufferView bv;

		const int index = aGltfModel.images.size();
		aGltfModel.images.emplace_back(tinygltf::Image());
		tinygltf::Image& exImage = aGltfModel.images.back();
		exImage.name = aImage->getName();
		exImage.uri = outRelativeFilepath.string();
		exImage.mimeType = filenameToMimeType(outStem);
		textureFormatToTinygltfImage(exImage, aImage->getFormat());
		exImage.width = aImage->getWidth();
		exImage.height = aImage->getHeight();
		exImage.image = aImage->getDataVector();
		return index;
	}


	void exportImages(const std::filesystem::path& aOutDir, tinygltf::Model& aGltfModel, const io::SceneData& aSceneInfo)
	{
		for (const tamashii::Material* material : aSceneInfo.mMaterials)
		{
			if (material->hasBaseColorTexture()) {
				tamashii::Image* img = material->getBaseColorTexture()->image;
				if(!MAP_INSIDE(imageToIndexDirectory, img)) {
					const int index = exportImage(aOutDir, aGltfModel, img);
					imageToIndexDirectory.insert(std::pair(img, index));
				}
			}
			if (material->hasNormalTexture()) {
				tamashii::Image* img = material->getNormalTexture()->image;
				if (!MAP_INSIDE(imageToIndexDirectory, img)) {
					const int index = exportImage(aOutDir, aGltfModel, img);
					imageToIndexDirectory.insert(std::pair(img, index));
				}
			}
			if (material->hasEmissionTexture()) {
				tamashii::Image* img = material->getEmissionTexture()->image;
				if (!MAP_INSIDE(imageToIndexDirectory, img)) {
					const int index = exportImage(aOutDir, aGltfModel, img);
					imageToIndexDirectory.insert(std::pair(img, index));
				}
			}
			if (material->hasOcclusionTexture() || material->hasRoughnessTexture() || material->hasMetallicTexture()) {
				tamashii::Image* occImg = material->hasOcclusionTexture() ? material->getOcclusionTexture()->image : nullptr;
				tamashii::Image* rouImg = material->hasRoughnessTexture() ? material->getRoughnessTexture()->image : nullptr;
				tamashii::Image* metImg = material->hasMetallicTexture() ? material->getMetallicTexture()->image : nullptr;
				tamashii::Image* ancorImage = occImg;
				if(!ancorImage) ancorImage = rouImg;
				else if (!ancorImage) ancorImage = metImg;
				if (ancorImage && !MAP_INSIDE(imageToIndexDirectory, ancorImage)) {
					const size_t imageSize = static_cast<size_t>(ancorImage->getWidth()) * static_cast<size_t>(ancorImage->getHeight());
					std::vector<unsigned char> imageData;
					imageData.reserve(imageSize * 3);
					for (size_t i = 0; i < imageSize; i++) {
						imageData.push_back(occImg ? occImg->getData()[i] : 255);
						imageData.push_back(rouImg ? rouImg->getData()[i] : 255);
						imageData.push_back(metImg ? metImg->getData()[i] : 255);
					}
					tamashii::Image img("");
					img.setFilepath(ancorImage->getFilepath());
					img.init(ancorImage->getWidth(), ancorImage->getHeight(), tamashii::Image::Format::RGB8_UNORM, imageData.data());
					const int index = exportImage(aOutDir, aGltfModel, &img);
					imageToIndexDirectory.insert(std::pair(occImg, index));
					imageToIndexDirectory.insert(std::pair(rouImg, index));
					imageToIndexDirectory.insert(std::pair(metImg, index));
				}
			}
		}
	}


	void exportTexture(tinygltf::Model& aGltfModel, tamashii::Texture* aTexture) {
		tinygltf::Sampler sam;
		sam.minFilter = textureFilterToTinygltfTextureFilter(aTexture->sampler.min, aTexture->image->needsMipMaps(), aTexture->sampler.mipmap);
		sam.magFilter = textureFilterToTinygltfTextureFilter(aTexture->sampler.mag, false, aTexture->sampler.mipmap);
		sam.wrapS = textureWrapToTinygltfTextureWrapType(aTexture->sampler.wrapU);
		sam.wrapT = textureWrapToTinygltfTextureWrapType(aTexture->sampler.wrapV);
		
		int samplerIndex = -1;
		for(uint32_t i = 0; i < aGltfModel.samplers.size(); i++) {
			if(sam == aGltfModel.samplers[i]) {
				samplerIndex = static_cast<int>(i);
				break;
			}
		}
		if(samplerIndex == -1) {
			samplerIndex = static_cast<int>(aGltfModel.samplers.size());
			aGltfModel.samplers.push_back(sam);
		}

		tinygltf::Texture texture;
		texture.sampler = samplerIndex;
		if(imageToIndexDirectory.find(aTexture->image) == imageToIndexDirectory.end())
		{
			spdlog::warn("glTF Export: Image not found.");
			return;
		}
		texture.source = imageToIndexDirectory.at(aTexture->image);
		int textureIndex = -1;
		for (uint32_t i = 0; i < aGltfModel.textures.size(); i++) {
			if (texture == aGltfModel.textures[i]) {
				textureIndex = static_cast<int>(i);
				textureToIndexDirectory.insert(std::pair(aTexture, textureIndex));
				break;
			}
		}
		if (textureIndex == -1) {
			textureIndex = static_cast<int>(aGltfModel.textures.size());
			aGltfModel.textures.push_back(texture);
			textureToIndexDirectory.insert(std::pair(aTexture, textureIndex));
		}
	}

	tinygltf::TextureInfo getTextureInfo(tamashii::Texture* aTexture)
	{
		if (!aTexture) return {};
		tinygltf::TextureInfo ti;
		ti.index = textureToIndexDirectory.at(aTexture);
		ti.texCoord = aTexture->texCoordIndex;
		return ti;
	}
	tinygltf::NormalTextureInfo getNormalTextureInfo(const tamashii::Material* aMaterial)
	{
		tamashii::Texture* tex = aMaterial->getNormalTexture();
		if (!tex) return {};
		tinygltf::NormalTextureInfo ti;
		ti.index = textureToIndexDirectory.at(tex);
		ti.texCoord = tex->texCoordIndex;
		ti.scale = static_cast<double>(aMaterial->getNormalScale());
		return ti;
	}
	tinygltf::OcclusionTextureInfo getOcclusionTextureInfo(const tamashii::Material* aMaterial)
	{
		tamashii::Texture* tex = aMaterial->getOcclusionTexture();
		if (!tex) return {};
		tinygltf::OcclusionTextureInfo ti;
		ti.index = textureToIndexDirectory.at(tex);
		ti.texCoord = tex->texCoordIndex;
		ti.strength = static_cast<double>(aMaterial->getOcclusionStrength());
		return ti;
	}

	void exportMaterial(tinygltf::Model& aGltfModel, tamashii::Material* aMaterial) {
		
		tinygltf::Material mat = {};

		mat.name = aMaterial->getName();
		mat.pbrMetallicRoughness.baseColorFactor = VEC4_TO_VECTOR(aMaterial->getBaseColorFactor());
		mat.pbrMetallicRoughness.baseColorTexture = getTextureInfo(aMaterial->getBaseColorTexture());
		mat.pbrMetallicRoughness.metallicFactor = static_cast<double>(aMaterial->getMetallicFactor());
		mat.pbrMetallicRoughness.roughnessFactor = static_cast<double>(aMaterial->getRoughnessFactor());
		mat.pbrMetallicRoughness.metallicRoughnessTexture = getTextureInfo(aMaterial->getMetallicTexture());
		mat.pbrMetallicRoughness.baseColorTexture = getTextureInfo(aMaterial->getBaseColorTexture());

		mat.alphaCutoff = static_cast<double>(aMaterial->getAlphaDiscardValue());
		mat.doubleSided = !aMaterial->getCullBackface();
		mat.alphaMode = getBlendMode(aMaterial->getBlendMode());
		mat.normalTexture = getNormalTextureInfo(aMaterial);
		mat.occlusionTexture = getOcclusionTextureInfo(aMaterial);
		mat.emissiveFactor = VEC3_TO_VECTOR(aMaterial->getEmissionFactor());
		mat.emissiveTexture = getTextureInfo(aMaterial->getEmissionTexture());
		exportCustomProperties(*aMaterial, &mat.extras);

		
		tinygltf::Value::Object ior;
		ior.insert(std::pair("ior", aMaterial->getIOR()));
		mat.extensions.insert(std::pair("KHR_materials_ior", ior));

		
		tinygltf::Value::Object emissiveStrength;
		emissiveStrength.insert(std::pair("emissiveStrength", aMaterial->getEmissionStrength()));
		mat.extensions.insert(std::pair("KHR_materials_emissive_strength", emissiveStrength));

		
		const auto attenuationColorV3 = aMaterial->getAttenuationColor();
		tinygltf::Value::Array arrayAC = { tinygltf::Value(attenuationColorV3.x), tinygltf::Value(attenuationColorV3.y), tinygltf::Value(attenuationColorV3.z) };
		tinygltf::Value::Object volume;
		volume.insert(std::pair("thicknessFactor", aMaterial->getThicknessFactor()));
		if(aMaterial->getAttenuationDistance() != 0.0f) volume.insert(std::pair("attenuationDistance", aMaterial->getAttenuationDistance()));
		volume.insert(std::pair("attenuationColor", arrayAC));
		if (aMaterial->getThicknessTexture()) {
			tinygltf::Value idxValue(textureToIndexDirectory.at(aMaterial->getThicknessTexture()));
			tinygltf::Value::Object idxObj;
			idxObj.insert(std::pair("index", idxValue));
			volume.insert(std::pair("thicknessTexture", idxObj));
		}
		mat.extensions.insert(std::pair("KHR_materials_volume", volume));

		
		tinygltf::Value::Object transmission;
		transmission.insert(std::pair("transmissionFactor", aMaterial->getTransmissionFactor()));
		if (aMaterial->getTransmissionTexture()) {
			tinygltf::Value idxValue(textureToIndexDirectory.at(aMaterial->getBaseColorTexture()));
			tinygltf::Value::Object idxObj;
			idxObj.insert(std::pair("index", idxValue));
			transmission.insert(std::pair("transmissionTexture", idxObj));
		}
		mat.extensions.insert(std::pair("KHR_materials_transmission", transmission));

		const auto it = std::find(aGltfModel.materials.begin(), aGltfModel.materials.end(), mat);
		const int index = static_cast<int>(it - aGltfModel.materials.begin());
		if (it == aGltfModel.materials.end()) {
			materialToIndexDirectory.insert(std::pair(aMaterial, static_cast<int>(aGltfModel.materials.size())));
			aGltfModel.materials.emplace_back(mat);
		} else materialToIndexDirectory.insert(std::pair(aMaterial, index));
	}

	uint64_t calcBufferSize(const io::SceneData& aSceneInfo)
	{
		uint64_t size = 0;
		for(const auto& model : aSceneInfo.mModels)
		{
			for(const auto& mesh : model->getMeshList())
			{
				if (mesh->hasIndices()) {
					if (mesh->getVertexCount() <= std::numeric_limits<uint8_t>::max()) size += rountUpToMultipleOf(mesh->getIndexCount() * sizeof(uint8_t), ROUND_UP_CONST);
					else if (mesh->getVertexCount() <= std::numeric_limits<uint16_t>::max()) size += rountUpToMultipleOf(mesh->getIndexCount() * sizeof(uint16_t), ROUND_UP_CONST);
					else if (mesh->getVertexCount() <= std::numeric_limits<uint32_t>::max()) size += rountUpToMultipleOf(mesh->getIndexCount() * sizeof(uint32_t), ROUND_UP_CONST);
				}
				{
					const size_t vertexCount = mesh->getVertexCount();
					if(mesh->hasPositions()) size += rountUpToMultipleOf(vertexCount * sizeof(glm::vec3), ROUND_UP_CONST);
					if (mesh->hasColors0()) size += rountUpToMultipleOf(vertexCount * sizeof(glm::vec4), ROUND_UP_CONST);
					if (mesh->hasNormals()) size += rountUpToMultipleOf(vertexCount * sizeof(glm::vec3), ROUND_UP_CONST);
					if (mesh->hasTangents()) size += rountUpToMultipleOf(vertexCount * sizeof(glm::vec4), ROUND_UP_CONST);
					if (mesh->hasTexCoords0()) size += rountUpToMultipleOf(vertexCount * sizeof(glm::vec2), ROUND_UP_CONST);
					if (mesh->hasTexCoords1()) size += rountUpToMultipleOf(vertexCount * sizeof(glm::vec2), ROUND_UP_CONST);
					for (const auto& [id, data] : mesh->getCustomDataMap())
					{
						size += rountUpToMultipleOf(data.bytes() * sizeof(uint8_t), ROUND_UP_CONST);
					}
				}
			}
		}
		
		if (!aSceneInfo.mSceneGraphs.empty()) {
			uint32_t timesteps = 0;
			aSceneInfo.mSceneGraphs[0]->visit([&size](Node* aNode)
			{
				const TRS& trs = aNode->getTRS();
				if (trs.hasTranslationAnimation())
				{
					const uint64_t ts = trs.translationSteps.size();
					const uint64_t tts = trs.translationTimeSteps.size();
					size += ts ? rountUpToMultipleOf(ts * sizeof(trs.translationSteps[0]), ROUND_UP_CONST) : 0;
					size += tts ? rountUpToMultipleOf(tts * sizeof(trs.translationTimeSteps[0]), ROUND_UP_CONST) : 0;
				}
				if (trs.hasRotationAnimation())
				{
					const uint64_t rs = trs.rotationSteps.size();
					const uint64_t rts = trs.rotationTimeSteps.size();
					size += rs ? rountUpToMultipleOf(rs * sizeof(trs.rotationSteps[0]), ROUND_UP_CONST) : 0;
					size += rts ? rountUpToMultipleOf(rts * sizeof(trs.rotationTimeSteps[0]), ROUND_UP_CONST) : 0;
				}
				if (trs.hasScaleAnimation())
				{
					const uint64_t ss = trs.scaleSteps.size();
					const uint64_t sts = trs.scaleTimeSteps.size();
					size += ss ? rountUpToMultipleOf(ss * sizeof(trs.scaleSteps[0]), ROUND_UP_CONST) : 0;
					size += sts ? rountUpToMultipleOf(sts * sizeof(trs.scaleTimeSteps[0]), ROUND_UP_CONST) : 0;
				}
			});
		}
		return size;
	}

	int getAccessor(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, const size_t aSizeInBytes, const size_t aCount)
	{
		tinygltf::BufferView bufferView;
		bufferView.buffer = 0;
		bufferView.byteOffset = aBuffer.mPointer;
		bufferView.byteLength = aSizeInBytes;

		const int posBufferView = static_cast<int>(aGltfModel.bufferViews.size());
		aGltfModel.bufferViews.push_back(bufferView);

		tinygltf::Accessor accessor;
		accessor.bufferView = posBufferView;
		accessor.count = aCount;
		aBuffer.mPointer += rountUpToMultipleOf(aSizeInBytes, ROUND_UP_CONST);

		const int posAccessor = static_cast<int>(aGltfModel.accessors.size());
		aGltfModel.accessors.push_back(accessor);
		return posAccessor;
	}
	int saveIndices(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<uint32_t>& indices = aMesh->getIndicesVectorRef();
		uint32_t bytes = 4;
		if (aMesh->getVertexCount() <= std::numeric_limits<uint8_t>::max()) bytes = 1;
		else if (aMesh->getVertexCount() <= std::numeric_limits<uint16_t>::max()) bytes = 2;
		const uint64_t sizeInBytes = indices.size() * bytes;
		if (bytes == 1) {
			const auto idx = reinterpret_cast<uint8_t*>(aBuffer.mPointer + aBuffer.mData.data());
			for (size_t i = 0; i < indices.size(); i++) idx[i] = static_cast<uint8_t>(indices[i]);
		}
		else if (bytes == 2) {
			const auto idx = reinterpret_cast<uint16_t*>(aBuffer.mPointer + aBuffer.mData.data());
			for (size_t i = 0; i < indices.size(); i++) idx[i] = static_cast<uint16_t>(indices[i]);
		}
		else if (bytes == 4) {
			const auto idx = reinterpret_cast<uint32_t*>(aBuffer.mPointer + aBuffer.mData.data());
			for (size_t i = 0; i < indices.size(); i++) idx[i] = indices[i];
		}

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, indices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		if (bytes == 1) accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		else if (bytes == 2) accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		else if (bytes == 4) accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
		accessor.type = TINYGLTF_TYPE_SCALAR;
		return posAccessor;
	}
	int savePositions(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<vertex_s>& vertices = aMesh->getVerticesVectorRef();
		const uint64_t sizeInBytes = vertices.size() * sizeof(glm::vec3);
		const auto positions = reinterpret_cast<glm::vec3*>(aBuffer.mPointer + aBuffer.mData.data());
		for (size_t i = 0; i < vertices.size(); i++) positions[i] = vertices[i].position;

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, vertices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.type = TINYGLTF_TYPE_VEC3;
		accessor.minValues = VEC3_TO_VECTOR(aMesh->getAABB().mMin);
		accessor.maxValues = VEC3_TO_VECTOR(aMesh->getAABB().mMax);
		return posAccessor;
	}
	int saveColors(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<vertex_s>& vertices = aMesh->getVerticesVectorRef();
		const uint64_t sizeInBytes = vertices.size() * sizeof(glm::vec4);
		const auto colors = reinterpret_cast<glm::vec4*>(aBuffer.mPointer + aBuffer.mData.data());
		for (size_t i = 0; i < vertices.size(); i++) colors[i] = vertices[i].color_0;

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, vertices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.type = TINYGLTF_TYPE_VEC4;
		return posAccessor;
	}
	int saveNormals(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<vertex_s>& vertices = aMesh->getVerticesVectorRef();
		const uint64_t sizeInBytes = vertices.size() * sizeof(glm::vec3);
		const auto normals = reinterpret_cast<glm::vec3*>(aBuffer.mPointer + aBuffer.mData.data());
		for (size_t i = 0; i < vertices.size(); i++) normals[i] = vertices[i].normal;

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, vertices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.type = TINYGLTF_TYPE_VEC3;
		return posAccessor;
	}
	int saveTangents(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<vertex_s>& vertices = aMesh->getVerticesVectorRef();
		const uint64_t sizeInBytes = vertices.size() * sizeof(glm::vec4);
		const auto tangents = reinterpret_cast<glm::vec4*>(aBuffer.mPointer + aBuffer.mData.data());
		for (size_t i = 0; i < vertices.size(); i++) {
			tangents[i] = vertices[i].tangent;
			tangents[i].w = 1;
		}

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, vertices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.type = TINYGLTF_TYPE_VEC4;
		return posAccessor;
	}
	int saveUV0s(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<vertex_s>& vertices = aMesh->getVerticesVectorRef();
		const uint64_t sizeInBytes = vertices.size() * sizeof(glm::vec2);
		const auto uv0 = reinterpret_cast<glm::vec2*>(aBuffer.mPointer + aBuffer.mData.data());
		for (size_t i = 0; i < vertices.size(); i++) uv0[i] = vertices[i].texture_coordinates_0;

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, vertices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.type = TINYGLTF_TYPE_VEC2;
		return posAccessor;
	}
	int saveUV1s(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		const std::vector<vertex_s>& vertices = aMesh->getVerticesVectorRef();
		const uint64_t sizeInBytes = vertices.size() * sizeof(glm::vec2);
		const auto uv0 = reinterpret_cast<glm::vec2*>(aBuffer.mPointer + aBuffer.mData.data());
		for (size_t i = 0; i < vertices.size(); i++) uv0[i] = vertices[i].texture_coordinates_0;

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, vertices.size());
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		accessor.type = TINYGLTF_TYPE_VEC2;
		return posAccessor;
	}

	int saveCustomData(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, const tamashii::Mesh::CustomData& aExtraData)
	{
		const uint64_t sizeInBytes = aExtraData.bytes();
		const auto extraData = reinterpret_cast<uint8_t*>(aBuffer.mPointer + aBuffer.mData.data());
		std::memcpy(extraData, aExtraData.data(), sizeInBytes);

		const int posAccessor = getAccessor(aGltfModel, aBuffer, sizeInBytes, sizeInBytes);
		tinygltf::Accessor& accessor = aGltfModel.accessors[posAccessor];
		accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		accessor.type = TINYGLTF_TYPE_SCALAR;
		return posAccessor;
	}
	
	tinygltf::Primitive exportMesh(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Mesh* aMesh)
	{
		tinygltf::Primitive primitive;
		primitive.mode = topologyToTinygltfMode(aMesh->getTopology());

		if (aMesh->hasIndices()) {
			primitive.indices = saveIndices(aGltfModel, aBuffer, aMesh);
		}
		{
			if (aMesh->hasPositions()) {
				primitive.attributes.insert({ "POSITION", savePositions(aGltfModel, aBuffer, aMesh) });
			}
			if (aMesh->hasNormals()) {
				primitive.attributes.insert({ "NORMAL", saveNormals(aGltfModel, aBuffer, aMesh) });
			}
			if (aMesh->hasTangents()) {
				primitive.attributes.insert({ "TANGENT", saveTangents(aGltfModel, aBuffer, aMesh) });
			}
			if (aMesh->hasColors0()) {
				primitive.attributes.insert({ "COLOR_0", saveColors(aGltfModel, aBuffer, aMesh) });
			}
			if (aMesh->hasTexCoords0()) {
				primitive.attributes.insert({ "TEXCOORD_0", saveUV0s(aGltfModel, aBuffer, aMesh) });
			}
			if (aMesh->hasTexCoords1()) {
				primitive.attributes.insert({ "TEXCOORD_1", saveUV1s(aGltfModel, aBuffer, aMesh) });
			}
			for(const auto& [id, data] : aMesh->getCustomDataMap())
			{
				if(data.bytes()) primitive.attributes.insert({ id, saveCustomData(aGltfModel, aBuffer, data) });
			}
		}
		if (aMesh->getMaterial()) primitive.material = materialToIndexDirectory[aMesh->getMaterial()];
		exportCustomProperties(*aMesh, &primitive.extras);
		return primitive;
	}
    void exportModel(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Model& aModel)
    {
		tinygltf::Mesh tmesh;
		tmesh.name = aModel.getName();
		exportCustomProperties(aModel, &tmesh.extras);
		tmesh.primitives.reserve(aModel.size());
		for(const auto& mesh : aModel.getMeshList())
		{
			tmesh.primitives.push_back(exportMesh(aGltfModel, aBuffer, mesh.get()));
		}
		
		modelToIndexDirectory.insert(std::pair(&aModel, static_cast<int>(aGltfModel.meshes.size())));
		aGltfModel.meshes.push_back(tmesh);
    }

	void exportCamera(tinygltf::Model& aGltfModel, tamashii::Camera& aCamera)
	{
		tinygltf::Camera cam;
		cam.name = aCamera.getName();
		if (aCamera.getType() == Camera::Type::PERSPECTIVE) {
			cam.type = "perspective";
			cam.perspective.znear = static_cast<double>(aCamera.getZNear());
			cam.perspective.zfar = static_cast<double>(aCamera.getZFar());
			cam.perspective.yfov = static_cast<double>(aCamera.getYFov());
			cam.perspective.aspectRatio = static_cast<double>(aCamera.getAspectRation());
		}
		else if (aCamera.getType() == Camera::Type::ORTHOGRAPHIC) {
			cam.type = "orthographic";
			cam.orthographic.znear = static_cast<double>(aCamera.getZNear());
			cam.orthographic.zfar = static_cast<double>(aCamera.getZFar());
			cam.orthographic.xmag = static_cast<double>(aCamera.getXMag());
			cam.orthographic.ymag = static_cast<double>(aCamera.getYMag());
		}
		
		cameraToIndexDirectory.insert(std::pair(&aCamera, static_cast<int>(aGltfModel.cameras.size())));
		aGltfModel.cameras.push_back(cam);
	}
	void exportLight(tinygltf::Model& aGltfModel, tamashii::Light& aLight)
	{
		tinygltf::Light light;
		light.name = aLight.getName();
		light.color = VEC3_TO_VECTOR(aLight.getColor());

		switch(aLight.getType())
		{
		case Light::Type::DIRECTIONAL:
		{
			light.type = "directional";
			
			const auto watt = static_cast<double>(aLight.getIntensity());
			const double lux = watt * 683.0;
			if (var::gltf_io_use_watt) light.intensity = watt;
			else light.intensity = lux;
			break;
		}
		case Light::Type::POINT:
		{
			light.type = "point";
			
			const auto watt = static_cast<double>(aLight.getIntensity());
			const double lumen = watt * 683.0;
			const double candela = lumen / (4.0 * std::numbers::pi);
			if (var::gltf_io_use_watt) light.intensity = watt;
			else light.intensity = candela;
			light.range = static_cast<double>(dynamic_cast<PointLight&>(aLight).getRange());
			break;
		}
		case Light::Type::SPOT:
		{
			const auto& spotLight = dynamic_cast<SpotLight&>(aLight);
			light.type = "spot";
			
			const auto watt = static_cast<double>(aLight.getIntensity());
			const double lumen = watt * 683.0;
			const double candela = lumen / (4.0 * std::numbers::pi);
			if (var::gltf_io_use_watt) light.intensity = watt;
			else light.intensity = candela;
			light.range = static_cast<double>(spotLight.getRange());
			light.spot.innerConeAngle = static_cast<double>(spotLight.getInnerConeAngle());
			light.spot.outerConeAngle = static_cast<double>(spotLight.getOuterConeAngle());
			break;
		}
		case Light::Type::SURFACE:
		{
			const auto& sl = dynamic_cast<SurfaceLight&>(aLight);
			const glm::vec3 dim = sl.getDimensions();
			light.type = "KHR_lights_area";
			light.intensity = static_cast<double>(sl.getIntensity());
			if(tamashii::SurfaceLight::Shape::SQUARE == sl.getShape() || tamashii::SurfaceLight::Shape::RECTANGLE == sl.getShape())
			{
				aLight.addCustomProperty("shape", Value(std::string("rect")));
				aLight.addCustomProperty("width", Value(dim.x));
				aLight.addCustomProperty("height", Value(dim.y));
			}
			if (tamashii::SurfaceLight::Shape::DISK == sl.getShape() || tamashii::SurfaceLight::Shape::SPHERE == sl.getShape())
			{
				aLight.addCustomProperty("shape", Value(std::string("disk")));
				aLight.addCustomProperty("radius", Value(dim.x));
			}
			break;
		}
		case Light::Type::IES:
		{
			const auto& iesLight = dynamic_cast<IESLight&>(aLight);
			light.type = "spot";
			
			const auto watt = static_cast<double>(aLight.getIntensity());
			const double lumen = watt * 683.0;
			const double candela = lumen / (4.0 * std::numbers::pi);
			if (var::gltf_io_use_watt) light.intensity = watt;
			else light.intensity = candela;
			light.range = 0.0;
			aLight.addCustomProperty("ies", Value(std::string(iesLight.getCandelaTexture()->image->getName())));
			break;
		}
		}
		exportCustomProperties(aLight, &light.extras);
		
		
		lightToIndexDirectory.insert(std::pair(&aLight, static_cast<int>(aGltfModel.lights.size())));
		aGltfModel.lights.push_back(light);
	}

	int writeTimeStepsToBuffer(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, const std::vector<float>& aTimeSteps)
	{ 
		const int inCount = static_cast<int>(aTimeSteps.size());
		const uint64_t inSizeInBytes = inCount * sizeof(aTimeSteps[0]);
		const auto inData = reinterpret_cast<float*>(aBuffer.mPointer + aBuffer.mData.data());
		const int inPosAccessor = getAccessor(aGltfModel, aBuffer, inSizeInBytes, inCount);
		float inMin = std::numeric_limits<float>::max();
		float inMax = 0;
		for (int i = 0; i < inCount; i++) {
			const float tts = aTimeSteps[i];
			if (tts < inMin) inMin = tts;
			if (tts > inMax) inMax = tts;
			inData[i] = tts;
		}
		aGltfModel.accessors[inPosAccessor].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		aGltfModel.accessors[inPosAccessor].type = TINYGLTF_TYPE_SCALAR;
		aGltfModel.accessors[inPosAccessor].minValues = { static_cast<double>(inMin) };
		aGltfModel.accessors[inPosAccessor].maxValues = { static_cast<double>(inMax) };
		return inPosAccessor;
	}
	void exportAnimation(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, const tamashii::TRS& aTrs, const int aNodeIndex)
	{
		const int ac = aTrs.hasTranslationAnimation() + aTrs.hasRotationAnimation() + aTrs.hasScaleAnimation();
		tinygltf::Animation animation;
		animation.channels.reserve(ac);
		animation.samplers.reserve(ac);
		
		
		if (aTrs.hasTranslationAnimation())
		{
			tinygltf::AnimationChannel channel;
			channel.target_path = "translation";
			channel.target_node = aNodeIndex;
			channel.sampler = static_cast<int>(animation.samplers.size());
			animation.channels.push_back(channel);

			const int inPosAccessor = writeTimeStepsToBuffer(aGltfModel, aBuffer, aTrs.translationTimeSteps);

			const int outCount = static_cast<int>(aTrs.translationSteps.size());
			const uint64_t outSizeInBytes = outCount * sizeof(aTrs.translationSteps[0]);
			const auto outData = reinterpret_cast<glm::vec3*>(aBuffer.mPointer + aBuffer.mData.data());
			const int outPosAccessor = getAccessor(aGltfModel, aBuffer, outSizeInBytes, outCount);
			for (int i = 0; i < outCount; i++) outData[i] = aTrs.translationSteps[i];
			aGltfModel.accessors[outPosAccessor].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			aGltfModel.accessors[outPosAccessor].type = TINYGLTF_TYPE_VEC3;

			tinygltf::AnimationSampler sampler;
			sampler.interpolation = interpolationToString(aTrs.translationInterpolation);
			sampler.input = inPosAccessor;
			sampler.output = outPosAccessor;
			animation.samplers.push_back(sampler);
		}
		if (aTrs.hasRotationAnimation()) 
		{
			tinygltf::AnimationChannel channel;
			channel.target_path = "rotation";
			channel.target_node = aNodeIndex;
			channel.sampler = static_cast<int>(animation.samplers.size());
			animation.channels.push_back(channel);

			const int inPosAccessor = writeTimeStepsToBuffer(aGltfModel, aBuffer, aTrs.rotationTimeSteps);

			const int outCount = static_cast<int>(aTrs.rotationSteps.size());
			const uint64_t outSizeInBytes = outCount * sizeof(aTrs.rotationSteps[0]);
			const auto outData = reinterpret_cast<glm::vec4*>(aBuffer.mPointer + aBuffer.mData.data());
			const int outPosAccessor = getAccessor(aGltfModel, aBuffer, outSizeInBytes, outCount);
			for (int i = 0; i < outCount; i++) outData[i] = aTrs.rotationSteps[i];
			aGltfModel.accessors[outPosAccessor].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			aGltfModel.accessors[outPosAccessor].type = TINYGLTF_TYPE_VEC4;

			tinygltf::AnimationSampler sampler;
			sampler.interpolation = interpolationToString(aTrs.rotationInterpolation);
			sampler.input = inPosAccessor;
			sampler.output = outPosAccessor;
			animation.samplers.push_back(sampler);
		}
		if (aTrs.hasScaleAnimation())
		{
			tinygltf::AnimationChannel channel;
			channel.target_path = "scale";
			channel.target_node = aNodeIndex;
			channel.sampler = static_cast<int>(animation.samplers.size());
			animation.channels.push_back(channel);

			const int inPosAccessor = writeTimeStepsToBuffer(aGltfModel, aBuffer, aTrs.scaleTimeSteps);

			const int outCount = static_cast<int>(aTrs.scaleSteps.size());
			const uint64_t outSizeInBytes = outCount * sizeof(aTrs.scaleSteps[0]);
			const auto outData = reinterpret_cast<glm::vec3*>(aBuffer.mPointer + aBuffer.mData.data());
			const int outPosAccessor = getAccessor(aGltfModel, aBuffer, outSizeInBytes, outCount);
			for (int i = 0; i < outCount; i++) outData[i] = aTrs.scaleSteps[i];
			aGltfModel.accessors[outPosAccessor].componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			aGltfModel.accessors[outPosAccessor].type = TINYGLTF_TYPE_VEC3;

			tinygltf::AnimationSampler sampler;
			sampler.interpolation = interpolationToString(aTrs.scaleInterpolation);
			sampler.input = inPosAccessor;
			sampler.output = outPosAccessor;
			animation.samplers.push_back(sampler);
		}
		aGltfModel.animations.push_back(animation);
	};
	int exportNode(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, tamashii::Node& aNode)
	{
		tinygltf::Node tnode;
		tnode.name = aNode.getName();
		if (aNode.hasModel())
		{
			const auto it = modelToIndexDirectory.find(aNode.getModel().get());
			if (it != modelToIndexDirectory.end()) {
				tnode.mesh = it->second;
			}
		}
		if (aNode.hasCamera()) {
			const auto it = cameraToIndexDirectory.find(aNode.getCamera().get());
			if (it != cameraToIndexDirectory.end()) {
				tnode.camera = it->second;
			}
		}
		if (aNode.hasLight()) {
			tamashii::Light* light = aNode.getLight().get();
			tamashii::Light::Type type = light->getType();
			if (lightToIndexDirectory.find(light) != lightToIndexDirectory.end()) {
				const int lightIdx = lightToIndexDirectory[light];
				tnode.light = lightIdx;
				tinygltf::Value::Object lightRef;
				lightRef.insert(std::pair("light", lightIdx));
				
				/*if (Light::Type::SURFACE == type)
				{
					tnode.extensions.insert(std::pair("KHR_lights_area", lightRef));
				}
				else {*/
				tnode.extensions.insert(std::pair("KHR_lights_punctual", lightRef));
				
			}
		}

		TRS& trs = aNode.getTRS();
		if (trs.hasTranslation()) tnode.translation = VEC3_TO_VECTOR(trs.translation);
		if (trs.hasScale()) tnode.scale = VEC3_TO_VECTOR(trs.scale);
		if (trs.hasRotation()) tnode.rotation = VEC4_TO_VECTOR(trs.rotation);
		tnode.children.reserve(aNode.numChildNodes());
		for(const auto& n : aNode)
		{
			tnode.children.push_back(exportNode(aGltfModel, aBuffer, *n));
		}
		const int nodeIndex = static_cast<int>(aGltfModel.nodes.size());
		if (trs.hasTranslationAnimation() || trs.hasRotationAnimation() || trs.hasScaleAnimation()) exportAnimation(aGltfModel, aBuffer, aNode.getTRS(), nodeIndex);
		aGltfModel.nodes.push_back(tnode);
		exportCustomProperties(aNode, &tnode.extras);
		return nodeIndex;
	}
	void exportScene(tinygltf::Model& aGltfModel, gltfBuffer& aBuffer, const std::shared_ptr<tamashii::Node>& aSceneGraph)
	{
		tinygltf::Scene gltfScene;
		gltfScene.name = aSceneGraph->getName();
		gltfScene.nodes.reserve(aSceneGraph->numChildNodes());
		for (const auto& n : *aSceneGraph)
		{
			gltfScene.nodes.push_back(exportNode(aGltfModel, aBuffer, *n));
		}
		aGltfModel.scenes.push_back(gltfScene);
		aGltfModel.defaultScene = 0;
	}
}

bool io::Export::save_scene_gltf(const std::string& aOutputFile, const SceneExportSettings aSettings, const SceneData& aSceneInfo)
{
	const std::filesystem::path outputDir = std::filesystem::path(aOutputFile).parent_path();
	imageToIndexDirectory.clear();
	textureToIndexDirectory.clear();
	materialToIndexDirectory.clear();
	modelToIndexDirectory.clear();
	cameraToIndexDirectory.clear();
	lightToIndexDirectory.clear();
	usedImageFilepaths.clear();

	imageToIndexDirectory.reserve(aSceneInfo.mImages.size());
	textureToIndexDirectory.reserve(aSceneInfo.mTextures.size());
	materialToIndexDirectory.reserve(aSceneInfo.mMaterials.size());
	modelToIndexDirectory.reserve(aSceneInfo.mModels.size());
	cameraToIndexDirectory.reserve(aSceneInfo.mCameras.size());
	lightToIndexDirectory.reserve(aSceneInfo.mLights.size());
	
	const uint64_t dataBufferSize = calcBufferSize(aSceneInfo);
	tinygltf::Buffer buffer;
	buffer.uri = std::filesystem::path(aOutputFile).stem().string() + ".bin";
	buffer.data = std::vector<unsigned char>(dataBufferSize);
	gltfBuffer dataBuffer = { buffer.data, 0 };

	tinygltf::Model model;
	model.asset.generator = "Tamashii Export";
	model.asset.version = "2.0";
	model.extensionsUsed = { "KHR_lights_punctual", "KHR_materials_ior", "KHR_materials_emissive_strength", "KHR_materials_transmission", "KHR_materials_volume" };
	model.extensionsRequired = { "KHR_lights_punctual", "KHR_materials_ior", "KHR_materials_emissive_strength", "KHR_materials_transmission", "KHR_materials_volume" };

	uint32_t accessorCount = 0;
	uint32_t animationCount = 0;
	for (const auto& mod : aSceneInfo.mModels) for(const auto& me : mod->getMeshList()) accessorCount += 7;
	if (!aSceneInfo.mSceneGraphs.empty()) {
		uint32_t nodeCount = 0;
		aSceneInfo.mSceneGraphs[0]->visit([&nodeCount, &accessorCount, &animationCount](Node* aNode)
		{
			nodeCount++;
			const TRS& trs = aNode->getTRS();
			accessorCount += (trs.hasTranslationAnimation() + trs.hasRotationAnimation() + trs.hasScaleAnimation()) * 2;
			if (trs.hasTranslationAnimation() || trs.hasRotationAnimation() || trs.hasScaleAnimation()) animationCount++;
			
		});
		model.nodes.reserve(nodeCount);
	}

	model.images.reserve(aSceneInfo.mImages.size());
	model.textures.reserve(aSceneInfo.mTextures.size());
	model.materials.reserve(aSceneInfo.mMaterials.size());
	model.accessors.reserve(accessorCount);
	model.bufferViews.reserve(accessorCount);
	model.animations.reserve(animationCount);
	model.meshes.reserve(aSceneInfo.mModels.size());
	model.cameras.reserve(aSceneInfo.mCameras.size());
	model.lights.reserve(aSceneInfo.mLights.size());
	if (!aSettings.mExcludeModels) {
		exportImages(outputDir, model, aSceneInfo);
		for (tamashii::Texture* tex : aSceneInfo.mTextures) exportTexture(model, tex);
		for (tamashii::Material* mat : aSceneInfo.mMaterials) exportMaterial(model, mat);
		for (auto& mod : aSceneInfo.mModels) exportModel(model, dataBuffer, *mod);
	}
	if (!aSettings.mExcludeCameras) for (auto& cam : aSceneInfo.mCameras) exportCamera(model, *cam);
	if (!aSettings.mExcludeLights) for (auto& light : aSceneInfo.mLights) exportLight(model, *light);
	if (!aSceneInfo.mSceneGraphs.empty()) exportScene(model, dataBuffer, aSceneInfo.mSceneGraphs[0]);

	assert(dataBuffer.mPointer <= buffer.data.size());
	if (dataBuffer.mPointer) model.buffers.push_back(buffer);

    tinygltf::TinyGLTF exporter;
	const tinygltf::WriteImageDataFunction writeImageData = &tinygltf::WriteImageData;
	tinygltf::FsCallbacks fs = { &tinygltf::FileExists, &tinygltf::ExpandFilePath,
	&tinygltf::ReadWholeFile, &tinygltf::WriteWholeFile, nullptr };
	exporter.SetImageWriter(writeImageData, &fs);

	spdlog::info("...using tiny glTF");
	bool success = false;
    if (!exporter.WriteGltfSceneToFile(&model, aOutputFile, aSettings.mEmbedImages, aSettings.mEmbedBuffers, true, aSettings.mWriteBinary)) {
        spdlog::error("Could not save gltf to {}", aOutputFile);
		success = false;
    } else
    {
        spdlog::info("Scene saved to gltf: {}", aOutputFile);
		success = true;
    }
	
	imageToIndexDirectory.clear();
	textureToIndexDirectory.clear();
	materialToIndexDirectory.clear();
	modelToIndexDirectory.clear();
	cameraToIndexDirectory.clear();
	lightToIndexDirectory.clear();
	usedImageFilepaths.clear();
	return success;
}

namespace {
	
	void textureFormatToTinygltfImage(tinygltf::Image& aImg, const Image::Format aFormat) {
		if (aFormat == Image::Format::R8_UNORM) {
			aImg.bits = 8;
			aImg.component = 1;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		} else if (aFormat == Image::Format::RG8_UNORM) {
			aImg.bits = 8;
			aImg.component = 2;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		} else if (aFormat == Image::Format::RGB8_UNORM || aFormat == Image::Format::RGB8_SRGB) {
			aImg.bits = 8;
			aImg.component = 3;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		} else if (aFormat == Image::Format::RGBA8_UNORM || aFormat == Image::Format::RGBA8_SRGB) {
			aImg.bits = 8;
			aImg.component = 4;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		} else if (aFormat == Image::Format::R16_UNORM) {
			aImg.bits = 16;
			aImg.component = 1;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		} else if (aFormat == Image::Format::RG16_UNORM) {
			aImg.bits = 16;
			aImg.component = 2;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		} else if (aFormat == Image::Format::RGB16_UNORM) {
			aImg.bits = 16;
			aImg.component = 3;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		} else if (aFormat == Image::Format::RGBA16_UNORM) {
			aImg.bits = 16;
			aImg.component = 4;
			aImg.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
		}
	}
	std::string filenameToMimeType(const std::filesystem::path& aPath)
	{
		std::string ext = aPath.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		if (ext == ".png") return "image/png";
		if (ext == ".jpg") return "image/jpeg";
		if (ext == ".bmp") return "image/bmp";
		if (ext == ".gif") return "image/gif";
		return "";
	}
	
	int textureWrapToTinygltfTextureWrapType(const Sampler::Wrap aTextureWrap) {
		switch (aTextureWrap)
		{
			case Sampler::Wrap::REPEAT: return TINYGLTF_TEXTURE_WRAP_REPEAT;	
			case Sampler::Wrap::CLAMP_TO_EDGE: return TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;	
			case Sampler::Wrap::MIRRORED_REPEAT: return TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT;	
			default: return 10497;
		}
	}
	int textureFilterToTinygltfTextureFilter(const Sampler::Filter aTextureFilter, const bool aMipMap, const Sampler::Filter aTextureMipMapFilter) {
		if(!aMipMap)
		{
			if (aTextureFilter == Sampler::Filter::NEAREST) return TINYGLTF_TEXTURE_FILTER_NEAREST;
			return TINYGLTF_TEXTURE_FILTER_LINEAR;
		}
		{
			if (aTextureFilter == Sampler::Filter::NEAREST && aTextureMipMapFilter == Sampler::Filter::NEAREST) return TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
			if (aTextureFilter == Sampler::Filter::LINEAR && aTextureMipMapFilter == Sampler::Filter::NEAREST) return TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
			if (aTextureFilter == Sampler::Filter::NEAREST && aTextureMipMapFilter == Sampler::Filter::LINEAR) return TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
			if (aTextureFilter == Sampler::Filter::LINEAR && aTextureMipMapFilter == Sampler::Filter::LINEAR) return TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
		}
		return 9728;
	}
	const char* getBlendMode(const Material::BlendMode aBlendMode)
	{
		switch(aBlendMode)
		{
		case Material::BlendMode::_OPAQUE: return "OPAQUE";
		case Material::BlendMode::MASK: return "MASK";
		case Material::BlendMode::BLEND: return "BLEND";
		}
		return "";
	}

	
	int topologyToTinygltfMode(const Mesh::Topology aTopology) {
		switch (aTopology)
		{
		case Mesh::Topology::POINT_LIST: return 0;
		case Mesh::Topology::LINE_LIST: return 1;
		case Mesh::Topology::LINE_STRIP: return 3;
		case Mesh::Topology::TRIANGLE_LIST: return 4;
		case Mesh::Topology::TRIANGLE_STRIP: return 5;
		case Mesh::Topology::TRIANGLE_FAN: return 6;
		case Mesh::Topology::UNKNOWN: return -1;
		}
		return -1;
	}
	
	const char* interpolationToString(const tamashii::TRS::Interpolation aInterpolation)
	{
		switch(aInterpolation) {
		case TRS::Interpolation::NONE: {
			return "";
		}
			case TRS::Interpolation::LINEAR: return "LINEAR";
			case TRS::Interpolation::STEP: return "STEP";
			case TRS::Interpolation::CUBIC_SPLINE: return "CUBIC_SPLINE";
		}
		return "";
	}
}
