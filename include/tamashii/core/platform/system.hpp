#pragma once
#include <tamashii/public.hpp>

#include <string>
#include <filesystem>

T_BEGIN_NAMESPACE
typedef struct {
	enum class Platform_e {
		WINDOWS,
		LINUX,
		APPLE
	};
				
	int			monitorMaxWidth;
	int			monitorMaxHeight;
	int			monitorPixelDepth;
	int			monitorRefreshRate;

	Platform_e	os;
	int			majorVersion;
	int			minorVersion;

	int			processorCoreCount;
	int			processorThreadCount;
	int			processorPackageCount;
				
	int			totalSystemMemory;
} SystemInfo_s;

namespace sys {
	/**
	* System stuff
	**/
	void						initSystem(SystemInfo_s& aSystemInfo);
	std::filesystem::path		openFileDialog(const std::string& aTitle, const std::vector<std::pair<std::string, std::vector<std::string>>>
	                                           & aFilter);
	std::filesystem::path		saveFileDialog(const std::string& aTitle, const std::string& aExtension);
	void						shutdown();

	/**
	* System Console
	**/
	void						createConsole(const std::string &aName);
	void						destroyConsole();

	/**
	* Library
	**/
	void*						loadLibrary(const std::string& aName);
	bool						unloadLibrary(void* aLib);
	void*						loadFunction(void* aLib, const std::string& aName);
}

T_END_NAMESPACE
