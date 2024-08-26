#include <tamashii/core/io/io.hpp>
#include <fstream>
T_USE_NAMESPACE

bool io::Export::write_file(std::string const& aFilename, std::string const& aContent)
{
	std::ofstream file(aFilename, std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		spdlog::error("tLoader: Could not open file {} for writing", aFilename);
		return false;
	}
	file.write(aContent.c_str(), static_cast<std::streamsize>(aContent.size()));
	file.close();
	return true;
}

bool io::Export::write_file_append(std::string const& aFilename, std::string const& aContent)
{
	std::ofstream file(aFilename, std::ios::out | std::ios::binary | std::ios::app);
	if (!file.is_open()) {
		spdlog::error("tLoader: Could not open file {} for writing", aFilename);
		return false;
	}
	file.write(aContent.c_str(), static_cast<std::streamsize>(aContent.size()));
	file.close();
	return true;
}
