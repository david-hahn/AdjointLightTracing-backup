#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/image.hpp>
#include <stb_image.h>
T_USE_NAMESPACE
Image* io::Import::load_image_8_bit(std::string const& aFile, const int aForceNumberOfChannels) {
	int width, height, channels;

	unsigned char* data = stbi_load(aFile.c_str(), &width, &height, &channels, aForceNumberOfChannels);
	if (aForceNumberOfChannels > 0) channels = aForceNumberOfChannels;
    if (data == nullptr) return nullptr;

	Image::Format format = Image::Format::UNKNOWN;
	if (channels == 4) format = Image::Format::RGBA8_UNORM;
	else if (channels == 3) format = Image::Format::RGB8_UNORM;
	else if (channels == 2) format = Image::Format::RG8_UNORM;
	else if (channels == 1) format = Image::Format::R8_UNORM;
	else spdlog::error("tLoader: Could not find image format of image {}", aFile);

	Image* img = Image::alloc(aFile);
	img->init(width, height, format, data);
	stbi_image_free(data);
	return img;
}

Image* io::Import::load_image_16_bit(std::string const& aFile, const int aForceNumberOfChannels) {
	int width, height, channels;

	unsigned char* data = (unsigned char*) stbi_load_16(aFile.c_str(), &width, &height, &channels, aForceNumberOfChannels);
	if (aForceNumberOfChannels > 0) channels = aForceNumberOfChannels;
	if (data == nullptr) return nullptr;

	Image::Format format = Image::Format::UNKNOWN;
	if (channels == 4) format = Image::Format::RGBA16_UNORM;
	else if (channels == 3) format = Image::Format::RGB16_UNORM;
	else if (channels == 2) format = Image::Format::RG16_UNORM;
	else if (channels == 1) format = Image::Format::R16_UNORM;
	else spdlog::error("tLoader: Could not find image format of image {}", aFile);

	Image* img = Image::alloc(aFile);
	img->init(width, height, format, data);
	stbi_image_free(data);
	return img;
}

