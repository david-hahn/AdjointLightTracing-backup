#include <tamashii/core/io/io.hpp>
#include <tamashii/core/topology/topology.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/material.hpp>
#define TINYOBJLOADER_IMPLEMENTATION 
#include <tiny_obj_loader.h>

T_USE_NAMESPACE
std::unique_ptr<Mesh> io::Import::load_obj_mesh(const std::string& aFile) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn, err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, aFile.c_str());

	if (!warn.empty()) spdlog::warn("Obj Loader: {}", warn);
	if (!err.empty()) spdlog::error("Obj Loader: {}", err);
	if (!ret) return nullptr;

    uint32_t indicesSize = 0;
    for (const auto& shape : shapes) indicesSize += shape.mesh.indices.size();

    aabb_s aabb = aabb_s(glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::min()));
    std::unique_ptr tmesh = Mesh::alloc();

    Material* mat = Material::alloc(DEFAULT_MATERIAL_NAME);
    tmesh->setMaterial(mat);

    tmesh->setTopology(Mesh::Topology::TRIANGLE_LIST);
    std::vector<uint32_t>* indices = tmesh->getIndicesVector();
    indices->reserve(indicesSize);

    
    for (const auto& shape : shapes)
    {
        
        size_t indexOffset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            size_t fv = shape.mesh.num_face_vertices[f];
            if (fv != 3) {
                spdlog::error("Obj Loader: faces != 3 not supported");
                return nullptr;
            }

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                indices->push_back(idx.vertex_index);
            }
            indexOffset += fv;
        }
    }
    if (!indices->empty()) tmesh->hasIndices(true);

    std::vector<vertex_s>* vertices = tmesh->getVerticesVector();
    vertices->reserve(attrib.vertices.size());

    size_t vCount = 0;
    for (size_t vOffset = 0; vOffset < attrib.vertices.size(); vOffset+=3) {
        vertex_s vd = {};
        tinyobj::real_t vx = attrib.vertices[3u * vCount + 0u];
        tinyobj::real_t vy = attrib.vertices[3u * vCount + 1u];
        tinyobj::real_t vz = attrib.vertices[3u * vCount + 2u];
        vd.position = glm::vec4(vx, vy, vz, 1);

        
        if (vd.position.x < aabb.mMin.x) aabb.mMin.x = vd.position.x;
        if (vd.position.y < aabb.mMin.y) aabb.mMin.y = vd.position.y;
        if (vd.position.z < aabb.mMin.z) aabb.mMin.z = vd.position.z;
        
        if (vd.position.x > aabb.mMax.x) aabb.mMax.x = vd.position.x;
        if (vd.position.y > aabb.mMax.y) aabb.mMax.y = vd.position.y;
        if (vd.position.z > aabb.mMax.z) aabb.mMax.z = vd.position.z;

        
        /*if (!attrib.normals.empty()) {
            tinyobj::real_t nx = attrib.normals[3u * vCount + 0u];
            tinyobj::real_t ny = attrib.normals[3u * vCount + 1u];
            tinyobj::real_t nz = attrib.normals[3u * vCount + 2u];
            vd.normal = glm::vec4(nx, ny, nz, 0);
        }*/

        
        if (!attrib.texcoords.empty()) {
            tinyobj::real_t tx = attrib.texcoords[2u * vCount + 0u];
            tinyobj::real_t ty = attrib.texcoords[2u * vCount + 1u];
            vd.texture_coordinates_0 = glm::vec2(tx, ty);
        }

        if (!attrib.colors.empty()) {
            tinyobj::real_t red = attrib.colors[3u * vCount + 0u];
            tinyobj::real_t green = attrib.colors[3u * vCount + 1u];
            tinyobj::real_t blue = attrib.colors[3u * vCount + 2u];
            vd.color_0 = glm::vec4(red, green, blue, 1);
        }
        vertices->push_back(vd);

        vCount++;
    }

    if (!attrib.vertices.empty()) tmesh->hasPositions(true);
    
    if (!attrib.texcoords.empty()) tmesh->hasTexCoords0(true);
    if (!attrib.colors.empty()) tmesh->hasColors0(true);


    if (!tmesh->hasNormals() && tmesh->getTopology() == Mesh::Topology::TRIANGLE_LIST) topology::calcNormals(tmesh.get());
    if (!tmesh->hasTangents() && tmesh->getTopology() == Mesh::Topology::TRIANGLE_LIST) topology::calcMikkTSpaceTangents(tmesh.get());
    tmesh->setAABB(aabb);

	return tmesh;
}

std::unique_ptr<Model> io::Import::load_obj(const std::string& aFile) {
	const std::shared_ptr tmesh = load_obj_mesh(aFile);

    auto model = Model::alloc("_obj");
    model->setFilepath(aFile);

    model->addMesh(tmesh);
    const aabb_s aabb = tmesh->getAABB();
    model->setAABB(aabb);
    return model;
}
