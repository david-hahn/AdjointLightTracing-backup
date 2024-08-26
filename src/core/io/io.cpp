#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/model.hpp>

T_USE_NAMESPACE

std::unique_ptr<Model> io::Import::load_model(const std::string& aFile)
{
	spdlog::info("Load Model: {}", aFile);
	std::string ext = std::filesystem::path(aFile).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (ext == ".ply") {
		return load_ply(aFile);
	}
	else if (ext == ".obj") {
		return load_obj(aFile);
	}
	spdlog::warn("...format not supported");
	return nullptr;
}

std::unique_ptr<Mesh> io::Import::load_mesh(const std::string& aFile)
{
	spdlog::info("Load Mesh: {}", aFile);
	std::string ext = std::filesystem::path(aFile).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (ext == ".ply") {
		return load_ply_mesh(aFile);
	}
	spdlog::warn("...format not supported");
	return nullptr;
}

std::unique_ptr<Light> io::Import::load_light(const std::filesystem::path& aFile) {
	spdlog::info("Load Light: {}", aFile.string());
	std::string ext = aFile.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (ext == ".ies") {
		return std::unique_ptr<Light>(load_ies(aFile).release());
	}
	else if (ext == ".ldt") {
		return std::unique_ptr<Light>(load_ldt(aFile).release());
	}
	spdlog::warn("...format not supported");
	return nullptr;
}