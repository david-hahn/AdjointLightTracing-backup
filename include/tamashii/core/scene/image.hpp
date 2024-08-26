#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/scene/asset.hpp>

T_BEGIN_NAMESPACE
class Image : public Asset {
public:
	enum class Format { 
		UNKNOWN,
		
		
		R8_UNORM,
		RG8_UNORM,
		RGB8_UNORM,
		RGBA8_UNORM,
		
		RGB8_SRGB,
		RGBA8_SRGB,

		
		
		R16_UNORM,
		RG16_UNORM,
		RGB16_UNORM,
		RGBA16_UNORM,

		
		
		R32_FLOAT,
		RG32_FLOAT,
		RGB32_FLOAT,
		RGBA32_FLOAT,
		
		
		R64_FLOAT,
		RG64_FLOAT,
		RGB64_FLOAT,
		RGBA64_FLOAT
	};
												Image(std::string_view aName);
												~Image() override;
	
	static Image*								alloc(std::string_view aName);

	void										init(uint32_t const& aWidth, uint32_t const& aHeight, Format aFormat, const void* aData);

	uint32_t									getWidth() const;
	uint32_t									getHeight() const;
	int											getImageSizeInBytes() const;
	int											getPixelSizeInBytes() const;
	uint8_t*									getData();
	std::vector<uint8_t>&						getDataVector();
	Format										getFormat() const;

	void										needsMipMaps(bool aMipmaps);
	bool										needsMipMaps() const;

	void										setSRGB(bool aSrgb);

	static int									textureFormatToBytes(Format aFormat);
private:

	uint32_t									mWidth;
	uint32_t									mHeight;
	int											mSizeInBytes;	
	Format										mFormat;
	bool										mMipmaps;
	std::vector<uint8_t>						mData;
};

struct Sampler {
	
	enum class Filter {
		NEAREST,
		LINEAR
	};
	enum class Wrap {
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};
	Filter										min;
	Filter										mag;

	Filter										mipmap;

	Wrap										wrapU;
	Wrap										wrapV;
	Wrap										wrapW;
	float										minLod;
	float										maxLod;
};

struct Texture {
	Image*										image;
	Sampler										sampler;
	int											texCoordIndex;
	int											index;					

												Texture() : image(nullptr), sampler(), texCoordIndex(-1), index(-1) {}
												Texture(const Texture& aTexture) = default;
	static Texture*								alloc() { return new Texture(); }
};
T_END_NAMESPACE
