#include <tamashii/core/io/io.hpp>
#include <fstream>
T_USE_NAMESPACE
std::string io::Import::load_file(std::string const& aFilename) {
	std::ifstream file(aFilename, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		spdlog::error("tLoader: Could not open file {} for reading", aFilename);
		return {};
	}

	const size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return std::string(buffer.begin(), buffer.end());
}
