#include <tamashii/core/topology/topology.hpp>
#include <tamashii/core/scene/model.hpp>


T_USE_NAMESPACE

glm::vec3 topology::calcStarkTangent(const glm::vec3& aNormal) {
	const glm::vec3 a = glm::abs(aNormal);
	const uint32_t xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	const uint32_t ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	const uint32_t zm = 1 ^ (xm | ym);
	return { glm::cross(aNormal, glm::vec3(xm, ym, zm)) };
}

void topology::calcStarkTangents(Mesh* aMesh)
{
	for(vertex_s& v : aMesh->getVerticesVectorRef())
	{
		v.tangent = { calcStarkTangent(v.normal), 0 };
	}
	aMesh->hasTangents(true);
}
