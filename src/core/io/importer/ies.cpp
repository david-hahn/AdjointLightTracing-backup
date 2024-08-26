#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tiny_ies.hpp>
T_USE_NAMESPACE

namespace {
    template <typename T>
    float lerp(T a, T b, T t) { return a + t * (b - a); }

    template <typename T>
    T interpolate_candela(const typename tiny_ies<T>::light& light, const T vertical_angle, const T horizontal_angle)
    {
        const size_t vertical_count = light.vertical_angles.size();
        const size_t horizontal_count = light.horizontal_angles.size();

        size_t vertical_offset = 0;
        size_t horizontal_offset = 0;
    	for(const T& v : light.vertical_angles) {
            if (v > vertical_angle) break;
        	vertical_offset++;
        }
        for (const T& v : light.horizontal_angles) {
            if (v > horizontal_angle) break;
            horizontal_offset++;
        }

        const size_t low_vertical_index = std::clamp<size_t>(vertical_offset - 1ull, 0ull, vertical_count - 1ull);
        const size_t high_vertical_index = std::clamp<size_t>(vertical_offset, 0ull, vertical_count - 1ull);
        const size_t low_horizontal_index = std::clamp<size_t>(horizontal_offset - 1ull, 0ull, horizontal_count - 1ull);
        const size_t high_horizontal_index = std::clamp<size_t>(horizontal_offset, 0ull, horizontal_count - 1ull);

        const T low_vertical_angle = light.vertical_angles[low_vertical_index];
        const T high_vertical_angle = light.vertical_angles[high_vertical_index];
        const T low_horizontal_angle = light.horizontal_angles[low_horizontal_index];
        const T high_horizontal_angle = light.horizontal_angles[high_horizontal_index];

        
        const T candela_a = light.candela[vertical_count * low_horizontal_index + low_vertical_index];
        const T candela_b = light.candela[vertical_count * low_horizontal_index + high_vertical_index];
        
        const T candela_c = light.candela[vertical_count * high_horizontal_index + low_vertical_index];
        const T candela_d = light.candela[vertical_count * high_horizontal_index + high_vertical_index];

        const T vertical_t_denom = high_vertical_angle - low_vertical_angle;
        T vertical_t = 0.0f;
        if(vertical_t_denom != 0.0f) vertical_t = (vertical_angle - low_vertical_angle) / vertical_t_denom;

        const T horizontal_t_denom = high_horizontal_angle - low_horizontal_angle;
        T horizontal_t = 0.0f;
        if (horizontal_t_denom != 0.0f) horizontal_t = (horizontal_angle - low_horizontal_angle) / horizontal_t_denom;

        assert(vertical_t <= static_cast<T>(1.0));
        assert(horizontal_t <= static_cast<T>(1.0));

        const T candela_int0 = lerp(candela_a, candela_b, vertical_t);
        const T candela_int1 = lerp(candela_c, candela_d, vertical_t);
        const T candela = lerp(candela_int0, candela_int1, horizontal_t);

        return candela;
    }

    template <typename T>
    void interpolate_IES(typename tiny_ies<T>::light& light, const uint32_t width, const uint32_t height)
    {
        glm::u32vec2 size(width, height);
        if (light.horizontal_angles.size() == 1) size.x = 1;
        if (light.vertical_angles.size() == 1) size.y = 1;

        
        std::vector<T> vertical_angles;
        vertical_angles.reserve(size.x);
        std::vector<T> horizontal_angles;
        horizontal_angles.reserve(size.y);
        std::vector<T> candela;
        candela.reserve(size.x * size.y);


        
        for (uint32_t y = 0; y < size.y; y++) {
            T vertical_angle = lerp(light.min_vertical_angle, light.max_vertical_angle, static_cast<T>(y) / static_cast<T>(size.y - 1u));
            if ((size.y - 1u) == 0u) vertical_angle = light.min_vertical_angle;
            vertical_angles.push_back(vertical_angle);
        }
        
        for (uint32_t x = 0; x < size.x; x++) {
            T horizontal_angle = lerp(light.min_horizontal_angle, light.max_horizontal_angle, static_cast<T>(x) / static_cast<T>(size.x - 1u));
            if ((size.x - 1u) == 0u) horizontal_angle = light.min_horizontal_angle;
            horizontal_angles.push_back(horizontal_angle);
            for (uint32_t y = 0; y < size.y; y++) {
                const T c = interpolate_candela<T>(light, vertical_angles[y], horizontal_angle);
                candela.push_back(c);
            }
        }

        light.number_horizontal_angles = static_cast<int>(size.x);
        light.number_vertical_angles = static_cast<int>(size.y);
        light.vertical_angles = vertical_angles;
        light.horizontal_angles = horizontal_angles;
        light.candela = candela;
    }
}


std::unique_ptr<IESLight> io::Import::load_ies(const std::filesystem::path& aFile) {
    tiny_ies<float>::light ies;
    std::string err;
    std::string warn;
    if (!tiny_ies<float>::load_ies(aFile.string(), err, warn, ies)) {
        spdlog::error("{}", err);
    }
    if (!err.empty()) spdlog::error("{}", err);
    if (!warn.empty()) spdlog::warn("{}", warn);

    if (ies.photometric_type != 1) spdlog::warn("Only IES Type C Lights are supported.");

    for (float& v : ies.candela) v = v / ies.max_candela;

    
    constexpr size_t res = 256;
    interpolate_IES<float>(ies, res, res);
    

    constexpr Sampler sampler = { 
        Sampler::Filter::LINEAR, Sampler::Filter::LINEAR, Sampler::Filter::LINEAR,
        Sampler::Wrap::CLAMP_TO_BORDER, Sampler::Wrap::MIRRORED_REPEAT,
    	Sampler::Wrap::CLAMP_TO_EDGE, 0, 0
    };

    Image* img = Image::alloc(aFile.filename().string());
    img->init(ies.number_vertical_angles, ies.number_horizontal_angles, Image::Format::R32_FLOAT, ies.candela.data());

    Texture* texture = Texture::alloc();
    texture->image = img;
    texture->sampler = sampler;

    auto light = std::make_unique<IESLight>();
    light->setFilepath(aFile.string());
    light->setCandelaTexture(texture);
    light->setVerticalAngles(ies.vertical_angles);
    light->setHorizontalAngles(ies.horizontal_angles);
    light->setIntensity(ies.max_candela);
    return std::move(light);
}

