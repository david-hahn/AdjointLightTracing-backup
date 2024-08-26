#pragma once

#include <rvk/parts/shader.hpp>
#include <string>

namespace scomp {
	
	void init();
	void finalize();
	void cleanup();
	
    bool preprocess(rvk::Shader::Source aRvkSource, rvk::Shader::Stage aRvkStage, std::string_view aPath, std::string_view aCode, std::vector<std::string> const& aDefines, std::string_view aEntryPoint);
	std::size_t getHash();
    bool compile(std::string& aSpv, bool aCache = false, std::string_view = "");
}
