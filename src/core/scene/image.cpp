#include <tamashii/core/scene/image.hpp>

T_USE_NAMESPACE

Image::Image(const std::string_view aName) :
	Asset{ Type::IMAGE, aName }, mWidth(-1), mHeight(-1), mSizeInBytes{-1},
mFormat{Format::UNKNOWN}, mMipmaps{false}{}

Image::~Image()
{
	mData.clear();
}

Image* Image::alloc(const std::string_view aName)
{
	return new Image(aName);
}

void Image::init(uint32_t const &aWidth, uint32_t const &aHeight, const Format aFormat, const void* aData)
{
	if(!mData.empty()) throw std::runtime_error("Image init: image already initialized");
	mWidth = aWidth;
	mHeight = aHeight;
	mFormat = aFormat;
	mSizeInBytes = aWidth * aHeight * textureFormatToBytes(aFormat);
	
	mData.resize(mSizeInBytes);
	std::memcpy(mData.data(), aData, mSizeInBytes);
}

uint32_t Image::getWidth() const
{
	return mWidth;
}

uint32_t Image::getHeight() const
{
	return mHeight;
}

int Image::getImageSizeInBytes() const
{
	return mSizeInBytes;
}

uint8_t* Image::getData()
{
	return mData.data();
}

std::vector<uint8_t>& Image::getDataVector()
{
	return mData;
}

Image::Format Image::getFormat() const
{
	return mFormat;
}

void Image::needsMipMaps(const bool aMipmaps)
{
	mMipmaps = aMipmaps;
}

bool Image::needsMipMaps() const
{
	return mMipmaps;
}

int Image::getPixelSizeInBytes() const
{
	return textureFormatToBytes(this->mFormat);
}

void Image::setSRGB(const bool aSrgb)
{
	if(aSrgb){
		switch (mFormat) {
		case Format::RGB8_UNORM:
			mFormat = Format::RGB8_SRGB; break;
		case Format::RGBA8_UNORM:
			mFormat = Format::RGBA8_SRGB; break;
		case Format::RGB8_SRGB:
		case Format::RGBA8_SRGB:
			break;
		default:
			spdlog::warn("SRGB not supported for this format");
		}
	}
	else {
		switch (mFormat) {
		case Format::RGB8_SRGB:
			mFormat = Format::RGB8_UNORM; break;
		case Format::RGBA8_SRGB:
			mFormat = Format::RGBA8_UNORM; break;
		default:
			break;
		}
	}
}

int Image::textureFormatToBytes(const Format aFormat) {
	switch (aFormat) {
	case Format::R8_UNORM:
		return 1;
	case Format::RG8_UNORM:
		return 2;
	case Format::RGB8_UNORM:
	case Format::RGB8_SRGB:
		return 3;
	case Format::RGBA8_UNORM:
	case Format::RGBA8_SRGB:
		return 4;
	
	case Format::R16_UNORM:
		return 2;
	case Format::RG16_UNORM:
		return 4;
	case Format::RGB16_UNORM:
		return 6;
	case Format::RGBA16_UNORM:
		return 8;
	
	case Format::R32_FLOAT:
		return 4;
	case Format::RG32_FLOAT:
		return 8;
	case Format::RGB32_FLOAT:
		return 12;
	case Format::RGBA32_FLOAT:
		return 16;
	
	case Format::R64_FLOAT:
		return 8;
	case Format::RG64_FLOAT:
		return 16;
	case Format::RGB64_FLOAT:
		return 24;
	case Format::RGBA64_FLOAT:
		return 32;
	default:
		return 0;
	}
}