#pragma once
#include <tamashii/public.hpp>

T_BEGIN_NAMESPACE
namespace math {
	bool decomposeTransform(const glm::mat4& aTransform, glm::vec3& aTranslation, glm::quat& aRotation, glm::vec3& aScale);
}
namespace color
{
	
	glm::vec3 srgb_to_XYZ(const glm::vec3& asrgb);
	
	glm::vec3 XYZ_to_xyY(const glm::vec3& aXYZ);
	
	glm::vec3 XYZ_to_srgb(const glm::vec3& aXYZ);
	
	glm::vec3 xyY_to_XYZ(const glm::vec3& axyY);

	
	glm::vec3 rgb_to_hsv(const glm::vec3& argb);
	glm::vec3 hsv_to_rgb(const glm::vec3& ahsv);
}



typedef uint64_t UUID;
namespace uuid
{
	UUID getUUID();
}
T_END_NAMESPACE
