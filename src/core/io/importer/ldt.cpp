#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tiny_ldt.hpp>
T_USE_NAMESPACE

std::unique_ptr<Light> io::Import::load_ldt(const std::filesystem::path& aFile) {
    tiny_ldt<float>::light ldt;
    std::string err;
    std::string warn;
    if (!tiny_ldt<float>::load_ldt(aFile.string(),err, warn, ldt)) {
        spdlog::error("{}", err);
    }
    if (!err.empty()) spdlog::error("{}", err);
    if (!warn.empty()) spdlog::warn("{}", warn);

	
	
	

    return std::make_unique<SpotLight>();
}