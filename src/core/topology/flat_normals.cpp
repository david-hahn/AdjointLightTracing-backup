#include <tamashii/core/topology/topology.hpp>
#include <tamashii/core/scene/model.hpp>
#include <deque>

T_USE_NAMESPACE

void topology::calcNormals(Mesh* aMesh)
{
	if (aMesh->getTopology() != Mesh::Topology::TRIANGLE_LIST)
	{
		spdlog::error("Calculate Normals: currently only implemented for TRIANGLE_LIST");
		return;
	}

	if (aMesh->hasIndices()) topology::calcSmoothNormals(aMesh);
	else topology::calcFlatNormals(aMesh);
}

void topology::calcSmoothNormals(Mesh* aMesh)
{
	if(aMesh->getTopology() != Mesh::Topology::TRIANGLE_LIST)
	{
		spdlog::error("SmoothNormals: currently only implemented for TRIANGLE_LIST");
		return;
	}
	if(!aMesh->hasIndices())
	{
		spdlog::error("SmoothNormals: requires indices");
		return;
	}

	vertex_s* vertices = aMesh->getVerticesArray();
	const uint32_t* indices = aMesh->getIndicesArray();
	std::vector<std::deque<glm::vec3>> normals;
	normals.resize(aMesh->getVertexCount());
	for (uint32_t i = 0; i < aMesh->getIndexCount(); i += 3) {
		vertex_s& v0 = vertices[indices[i + 0]];
		vertex_s& v1 = vertices[indices[i + 1]];
		vertex_s& v2 = vertices[indices[i + 2]];
		glm::vec3 normal = glm::cross(glm::vec3(v1.position - v0.position), glm::vec3(v2.position - v0.position));
		
		if (glm::all(glm::equal(normal, glm::vec3(0.0f)))) continue;

		normals[indices[i]].push_back(normal);
		normals[indices[i + 1]].push_back(normal);
		normals[indices[i + 2]].push_back(normal);
	}
	for (uint32_t i = 0; i < aMesh->getVertexCount(); i++) {
		auto normal = glm::vec3(0);
		for (const glm::vec3& n : normals[i]) normal += n;
		normal = normalize(normal);
		if (glm::all(glm::equal(normal, glm::vec3(0.0f)))) {
			spdlog::error("SmoothNormals: normal could not be calculated for one vertex");
			normal = glm::vec3(1, 0, 0);
		}
		vertices[i].normal = glm::vec4(normal, 0);
	}
	aMesh->hasNormals(true);
}

void topology::calcFlatNormals(Mesh* aMesh)
{
	if (aMesh->getTopology() != Mesh::Topology::TRIANGLE_LIST)
	{
		spdlog::error("FlatNormals: currently only implemented for TRIANGLE_LIST");
		return;
	}
	if (aMesh->hasIndices())
	{
		spdlog::error("FlatNormals: use if no indices are available");
		return;
	}

	vertex_s* vertices = aMesh->getVerticesArray();
	for (uint32_t i = 0; i < aMesh->getVertexCount(); i += 3) {
		vertex_s& v0 = vertices[i];
		vertex_s& v1 = vertices[i + 1];
		vertex_s& v2 = vertices[i + 2];
		glm::vec3 dir01 = v1.position - v0.position;
		glm::vec3 dir02 = v2.position - v0.position;
		glm::vec3 normal = glm::cross(dir01, dir02);

		
		if (glm::all(glm::equal(normal, glm::vec3(0.0f)))) normal = glm::vec3(1, 0, 0);

		v0.normal = v1.normal = v2.normal = glm::vec4(glm::normalize(normal), 0);
	}
}
