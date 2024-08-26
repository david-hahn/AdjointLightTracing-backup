#include <tamashii/core/io/io.hpp>
#include <stb_image_write.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

T_USE_NAMESPACE

void io::Export::save_image_png_8_bit(std::string const& aName, const int aWidth, const int aHeight, const int aChannels, const uint8_t* aPixels)
{
    stbi_write_png(aName.c_str(), aWidth, aHeight, aChannels, aPixels, aWidth * aChannels);
}

void io::Export::save_image_exr(std::string const& aName, const int aWidth, const int aHeight, const float* aPixels, const uint32_t aStride, const std::vector<uint8_t>& aOut)
{
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    const auto dim = static_cast<uint32_t>(aWidth * aHeight);
    
    image.num_channels = static_cast<int>(aOut.size());
    header.num_channels = static_cast<int>(aOut.size());

    std::vector<std::vector<float>> images(aStride);
    for(std::vector<float>& v : images) v.resize(dim);

    
    for (uint32_t i = 0; i < dim; i++) {
        for (uint32_t j = 0; j < images.size(); j++)
        {
        	images[j][i] = aPixels[aStride * i + j];
        }
    }

    
    std::vector<EXRChannelInfo> channelInfos(header.num_channels);
    header.channels = channelInfos.data();

    std::vector<float*> imagePtr;
    imagePtr.reserve(image.num_channels);
    for (const uint8_t& v : aOut)
    {
        const auto index = static_cast<uint32_t>(imagePtr.size());
        if (v == 0) { header.channels[index].name[0] = 'R'; header.channels[index].name[1] = '\0'; }
        else if (v == 1) { header.channels[index].name[0] = 'G'; header.channels[index].name[1] = '\0'; }
        else if (v == 2) { header.channels[index].name[0] = 'B'; header.channels[index].name[1] = '\0'; }
        else if (v == 3) { header.channels[index].name[0] = 'A'; header.channels[index].name[1] = '\0'; }
        else { header.channels[index].name[0] = ' '; header.channels[index].name[1] = '\0'; }
        imagePtr.push_back(images[v].data());
    }

    image.images = reinterpret_cast<unsigned char**>(imagePtr.data());
    image.width = aWidth;
    image.height = aHeight;

    std::vector pixelTypes(header.num_channels, TINYEXR_PIXELTYPE_FLOAT);
    std::vector requestedPixelTypes(header.num_channels, TINYEXR_PIXELTYPE_FLOAT);
    header.pixel_types = pixelTypes.data();
    header.requested_pixel_types = requestedPixelTypes.data();

    const char* err = nullptr;
    const int ret = SaveEXRImageToFile(&image, &header, aName.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        spdlog::error("Save EXR err: {}", err);
        FreeEXRErrorMessage(err); 
        return;
    }
}
