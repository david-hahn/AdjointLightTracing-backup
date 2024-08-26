#pragma once
#include <rvk/parts/rvk_public.hpp>

#include <string>
#include <functional>

RVK_BEGIN_NAMESPACE
struct SamplerConfig {
	VkFilter mag; VkFilter min; VkSamplerMipmapMode mipmap;
	VkSamplerAddressMode wrapU; VkSamplerAddressMode wrapV; VkSamplerAddressMode wrapW;
	float minLod; float	maxLod;
	
	
	bool operator==(const SamplerConfig& aSc) const
	{
		return mag == aSc.mag && min == aSc.min && mipmap == aSc.mipmap && wrapU == aSc.wrapU &&
			wrapV == aSc.wrapV && wrapW == aSc.wrapW && minLod == aSc.minLod && maxLod == aSc.maxLod;
	}
	
	size_t operator()(const SamplerConfig& aSc) const
	{
		const std::string s(std::to_string(aSc.mag) + std::to_string(aSc.min) + std::to_string(aSc.mipmap) + std::to_string(aSc.wrapU) +
			std::to_string(aSc.wrapV) + std::to_string(aSc.wrapW) + std::to_string(aSc.minLod) + std::to_string(aSc.maxLod));
		constexpr std::hash<std::string> strHash;
		return strHash(s);
	}
};
RVK_END_NAMESPACE