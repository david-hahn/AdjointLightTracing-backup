#include <tamashii/engine/platform/system.hpp>

#include <tamashii/engine/platform/window.hpp>

#include <dlfcn.h>

T_USE_NAMESPACE

/**
* System stuff
**/
void sys::initSystem(SystemInfo_s& system_info) {
    
}
std::filesystem::path sys::openFileDialog(const std::string& aTitle, const std::vector<std::pair<std::string, std::vector<std::string>>>& aFilter) {
    
}
std::filesystem::path sys::saveFileDialog(const std::string& aTitle, const std::string& aExtension) {
    
}
void sys::shutdown() {
	exit(0);
}

/**
* System Console
**/
void sys::createConsole(const std::string &name) {
}
void sys::destroyConsole() {
}

void* sys::loadLibrary(const std::string& aName)
{
    return dlopen(aName.c_str(), RTLD_LAZY | RTLD_GLOBAL);
}

bool sys::unloadLibrary(void* aLib)
{
    return dlclose(aLib);
}

void* sys::loadFunction(void* aLib, const std::string& aName)
{
    return dlsym(aLib, aName.c_str());
}

