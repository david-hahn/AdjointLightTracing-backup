#include <tamashii/core/io/io.hpp>
#include <tamashii/core/topology/topology.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/material.hpp>

#include <fstream>
#define TINYPLY_IMPLEMENTATION
#include <tinyply.h>

namespace
{
	template<typename T>
	void setIndices(uint32_t aIPF, tinyply::Buffer& aBuffer, std::vector<uint32_t>* aIndicesVector)
	{
		const uint32_t triangles = 1 + (aIPF - 3);
		const auto indexCount = static_cast<uint32_t>(aBuffer.size_bytes() / sizeof(T));

		const auto arr = reinterpret_cast<T*>(aBuffer.get());
		for (uint32_t offset = 0; offset < indexCount; offset+=aIPF) {
			for (uint32_t i = 0; i < triangles; i++) {
				aIndicesVector->push_back(arr[offset + 0]);
				aIndicesVector->push_back(arr[offset + i + 1]);
				aIndicesVector->push_back(arr[offset + i + 2]);
			}
		}
	}
}

T_USE_NAMESPACE
std::unique_ptr<Mesh> io::Import::load_ply_mesh(const std::string& aFile) {
	std::ifstream fileStream(aFile, std::ios::binary);
	
	try
	{
		if (!fileStream || fileStream.fail()) throw std::runtime_error("file_stream failed to open " + aFile);

		tinyply::PlyFile file;
		file.parse_header(fileStream);

		std::shared_ptr<tinyply::PlyData> vertices, normals, colors, texcoords, faces, tripstrip;
		try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }); }
		catch (const std::exception& e) { }
		try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }); }
		catch (const std::exception& e) { }
		try { colors = file.request_properties_from_element("vertex", { "red", "green", "blue", "alpha" }); }
		catch (const std::exception& e) { }
		try { if (!colors) colors = file.request_properties_from_element("vertex", {"r", "g", "b", "a"}); }
		catch (const std::exception& e) { }
		try { texcoords = file.request_properties_from_element("vertex", {"u", "v"}); }
		catch (const std::exception& e) { }
		try { if(!texcoords) texcoords = file.request_properties_from_element("vertex", { "s", "t" }); }
		catch (const std::exception& e) {}
		try { faces = file.request_properties_from_element("face", { "vertex_indices" }, 0); }
		catch (const std::exception& e) { }
		try { if (!colors) colors = file.request_properties_from_element("face", { "red", "green", "blue", "alpha" }, 0); }
		catch (const std::exception& e) {}
		try { tripstrip = file.request_properties_from_element("tristrips", { "vertex_indices" }, 0); }
		catch (const std::exception& e) { }
		file.read(fileStream);

		std::unique_ptr<Mesh> tmesh = Mesh::alloc();
		tmesh->setTopology(Mesh::Topology::TRIANGLE_LIST);
		auto aabb = aabb_s(glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::min()));

		uint32_t indicesPerFace = 3;
		for (const auto& e : file.get_elements()) {
			if(e.name == "face") {
				for (const auto& p : e.properties) {
					if (p.name == "vertex_indices" && p.isList) {
						indicesPerFace = static_cast<uint32_t>(p.listCount);
						break;
					}
				}
			}
		}

		if (faces) {
			const size_t indexCount = (3 + (indicesPerFace - 3) * 3) * faces->count;

			tmesh->getIndicesVector()->reserve(indexCount);
			if (faces->t == tinyply::Type::INT8) setIndices<int8_t>(indicesPerFace, faces->buffer, tmesh->getIndicesVector());
			else if (faces->t == tinyply::Type::UINT8) setIndices<uint8_t>(indicesPerFace, faces->buffer, tmesh->getIndicesVector());
			else if (faces->t == tinyply::Type::INT16) setIndices<int16_t>(indicesPerFace, faces->buffer, tmesh->getIndicesVector());
			else if (faces->t == tinyply::Type::UINT16) setIndices<uint16_t>(indicesPerFace, faces->buffer, tmesh->getIndicesVector());
			else if (faces->t == tinyply::Type::INT32) setIndices<int32_t>(indicesPerFace, faces->buffer, tmesh->getIndicesVector());
			else if (faces->t == tinyply::Type::UINT32) setIndices<uint32_t>(indicesPerFace, faces->buffer, tmesh->getIndicesVector());
			tmesh->hasIndices(true);
		}

		if (vertices) {
			assert(vertices->t == tinyply::Type::FLOAT32);
			tmesh->hasPositions(true);
		}
		if (normals) {
			assert(normals->t == tinyply::Type::FLOAT32);
			tmesh->hasNormals(true);
		}
		if (texcoords) {
			assert(texcoords->t == tinyply::Type::FLOAT32);
			tmesh->hasTexCoords0(true);
		}
		if (colors) {
			assert(colors->t == tinyply::Type::UINT8);
			tmesh->hasColors0(true);
		}

		std::vector<vertex_s>& meshVertices = tmesh->getVerticesVectorRef();
		meshVertices.resize(vertices->count);
		for (size_t i = 0; i < vertices->count; i++) {
			if (vertices) {
				const auto vp = reinterpret_cast<glm::vec3*>(vertices->buffer.get());
				meshVertices[i].position = glm::vec4(vp[i], 1);
			}
			if (normals) {
				const auto np = reinterpret_cast<glm::vec3*>(normals->buffer.get());
				meshVertices[i].normal = glm::vec4(np[i], 0);
			}
			if (texcoords) {
				const auto tp = reinterpret_cast<glm::vec2*>(texcoords->buffer.get());
				meshVertices[i].texture_coordinates_0 = tp[i];
				meshVertices[i].texture_coordinates_0.y = 1.0f - meshVertices[i].texture_coordinates_0.y;
			}
			if (colors) {
				if (colors->t == tinyply::Type::UINT8) {
					const auto tp = reinterpret_cast<glm::u8vec4*>(colors->buffer.get());
					meshVertices[i].color_0 = glm::vec4(tp[i]) / 255.0f;
				}
			}

			
			if (meshVertices[i].position.x < aabb.mMin.x) aabb.mMin.x = meshVertices[i].position.x;
			if (meshVertices[i].position.y < aabb.mMin.y) aabb.mMin.y = meshVertices[i].position.y;
			if (meshVertices[i].position.z < aabb.mMin.z) aabb.mMin.z = meshVertices[i].position.z;
			
			if (meshVertices[i].position.x > aabb.mMax.x) aabb.mMax.x = meshVertices[i].position.x;
			if (meshVertices[i].position.y > aabb.mMax.y) aabb.mMax.y = meshVertices[i].position.y;
			if (meshVertices[i].position.z > aabb.mMax.z) aabb.mMax.z = meshVertices[i].position.z;
		}

		
		if (!tmesh->hasNormals() && tmesh->getTopology() == Mesh::Topology::TRIANGLE_LIST) topology::calcNormals(tmesh.get());
		if (!tmesh->hasTangents() && tmesh->hasTexCoords0() && tmesh->getTopology() == Mesh::Topology::TRIANGLE_LIST) topology::calcMikkTSpaceTangents(tmesh.get());
		if (!tmesh->hasTangents()) topology::calcStarkTangents(tmesh.get());
		
		Material* mat = Material::alloc(DEFAULT_MATERIAL_NAME);
		tmesh->setMaterial(mat);

		tmesh->setAABB(aabb);
		return tmesh;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Caught tinyply exception: " << e.what() << std::endl;
		return nullptr;
	}
}

std::unique_ptr<Model> io::Import::load_ply(const std::string& aFile) {
	std::shared_ptr<Mesh> tmesh = load_ply_mesh(aFile);

	auto model = Model::alloc("_ply");
	model->setFilepath(aFile);

	model->addMesh(tmesh);
	const aabb_s aabb =  tmesh->getAABB();
	model->setAABB(aabb);
	return model;
}
