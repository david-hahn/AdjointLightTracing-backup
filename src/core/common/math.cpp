#include <tamashii/core/common/math.hpp>
#include <glm/gtc/color_space.hpp>
#include <random>

T_USE_NAMESPACE

bool tamashii::math::decomposeTransform(const glm::mat4& aTransform, glm::vec3& aTranslation, glm::quat& aRotation, glm::vec3& aScale)
{
	
	
	using namespace glm;
	using T = float;

	mat4 localMatrix(aTransform);
	
	if (epsilonEqual(localMatrix[3][3], static_cast<float>(0), epsilon<T>())) return false;
	
	if (
		epsilonNotEqual(localMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
		epsilonNotEqual(localMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
		epsilonNotEqual(localMatrix[2][3], static_cast<T>(0), epsilon<T>()))
	{
		
		localMatrix[0][3] = localMatrix[1][3] = localMatrix[2][3] = static_cast<T>(0);
		localMatrix[3][3] = static_cast<T>(1);
	}
	
	aTranslation = vec3(localMatrix[3]);
	localMatrix[3] = vec4(0, 0, 0, localMatrix[3].w);

	vec3 row[3];
	
	for (length_t i = 0; i < 3; ++i)
		for (length_t j = 0; j < 3; ++j)
			row[i][j] = localMatrix[i][j];

	
	aScale.x = length(row[0]);
	row[0] = detail::scale(row[0], static_cast<T>(1));
	aScale.y = length(row[1]);
	row[1] = detail::scale(row[1], static_cast<T>(1));
	aScale.z = length(row[2]);
	row[2] = detail::scale(row[2], static_cast<T>(1));
	
	T root;
	const T trace = row[0].x + row[1].y + row[2].z;
	if (trace > static_cast<T>(0))
	{
		root = sqrt(trace + static_cast<T>(1.0));
		aRotation.w = static_cast<T>(0.5) * root;
		root = static_cast<T>(0.5) / root;
		aRotation.x = root * (row[1].z - row[2].y);
		aRotation.y = root * (row[2].x - row[0].z);
		aRotation.z = root * (row[0].y - row[1].x);
	} 
	else
	{
		static int next[3] = { 1, 2, 0 };
		int i = 0;
		if (row[1].y > row[0].x) i = 1;
		if (row[2].z > row[i][i]) i = 2;
		const int j = next[i];
		const int k = next[j];

		root = sqrt(row[i][i] - row[j][j] - row[k][k] + static_cast<T>(1.0));

		aRotation[i] = static_cast<T>(0.5) * root;
		root = static_cast<T>(0.5) / root;
		aRotation[j] = root * (row[i][j] + row[j][i]);
		aRotation[k] = root * (row[i][k] + row[k][i]);
		aRotation.w = root * (row[j][k] - row[k][j]);
	} 

	return true;
}

glm::vec3 color::srgb_to_XYZ(const glm::vec3& asrgb)
{
	glm::vec3 rgb = glm::convertSRGBToLinear(asrgb);
	
	const glm::mat3 srgbToXYZ = {
		0.4124564f,  0.3575761f,  0.1804375f,
		0.2126729f,  0.7151522f,  0.0721750f,
		0.0193339f,  0.1191920f,  0.9503041f
	};
	return rgb * srgbToXYZ;
}

glm::vec3 color::XYZ_to_xyY(const glm::vec3& aXYZ)
{
	const float sum = glm::compAdd(aXYZ);
	if (sum == 0.0f) return glm::vec3(0.0f);
	return { aXYZ.x / sum, aXYZ.y / sum, aXYZ.y };
}

glm::vec3 color::XYZ_to_srgb(const glm::vec3& aXYZ)
{
	
	const glm::mat3 xyzTosrgb = {
		 3.2404542, -1.5371385, -0.4985314,
		-0.9692660,  1.8760108,  0.0415560,
		 0.0556434, -0.2040259,  1.0572252
	};
	return glm::convertLinearToSRGB(aXYZ * xyzTosrgb);
}

glm::vec3 color::xyY_to_XYZ(const glm::vec3& axyY)
{
	glm::vec3 XYZ(0.0f);
	if(axyY.y != 0.0f) XYZ.x = (axyY.x * axyY.z) / axyY.y;
	XYZ.y = axyY.z;
	if (axyY.y != 0.0f) XYZ.z = ((1.0f - axyY.x - axyY.y) * axyY.z) / axyY.y;
	return XYZ;
}

glm::vec3 color::rgb_to_hsv(const glm::vec3& argb)
{
	glm::vec3 hsvColor(0.0f);

	const float cmin = std::min(argb.x, std::min(argb.y, argb.z));
	const float cmax = std::max(argb.x, std::max(argb.y, argb.z));
	const float delta = cmax - cmin;

	
	if (cmin == cmax) hsvColor.x = 0;
	else if (argb.x == cmax) hsvColor.x = ((argb.y - argb.z) / delta) + (argb.y < argb.z ? 6 : 0); 
	else if (argb.y == cmax) hsvColor.x = 2.0f + ((argb.z - argb.x) / delta);	
	else if (argb.z == cmax) hsvColor.x = 4.0f + ((argb.x - argb.y) / delta);	
	hsvColor.x = hsvColor.x / 6.0f;
	
	if(cmax != 0.0f) hsvColor.y = delta / cmax;
	
	hsvColor.z = cmax;

	return hsvColor;
}

glm::vec3 color::hsv_to_rgb(const glm::vec3& ahsv)
{
	const float h = ahsv.x;
	const float s = ahsv.y;
	float v = ahsv.z;
	glm::vec3 rgbColor;

	const auto i = static_cast<uint32_t>(std::floor(h * 6.0f));
	const float f = h * 6.0f - static_cast<float>(i);
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);

	switch (i % 6) {
	case 0: return { v,t,p };
	case 1: return { q,v,p };
	case 2: return { p,v,t };
	case 3: return { p,q,v };
	case 4: return { t,p,v };
	case 5: return { v,p,q };
	}
	return glm::vec3(0.0f);
}

UUID uuid::getUUID()
{
	static std::random_device						rd;
	static std::mt19937_64							gen(rd());
	static std::uniform_int_distribution<uint64_t>	dis;
	return dis( gen );
}
