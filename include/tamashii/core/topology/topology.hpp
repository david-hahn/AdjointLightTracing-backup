#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>

T_BEGIN_NAMESPACE
namespace topology {

	void calcNormals(Mesh* aMesh);
	void calcFlatNormals(Mesh* aMesh);
	void calcSmoothNormals(Mesh* aMesh);

	
	void calcMikkTSpaceTangents(Mesh* aMesh);

	
	
	
	
	glm::vec3 calcStarkTangent(const glm::vec3& aNormal);
	void calcStarkTangents(Mesh* aMesh);
}
T_END_NAMESPACE
