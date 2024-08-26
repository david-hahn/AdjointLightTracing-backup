#include <tamashii/core/platform/system.hpp>
#include <tamashii/core/common/input.hpp>

#include <filesystem>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <libgen.h>
#include <csignal>
#include <dlfcn.h>

T_USE_NAMESPACE

/**
* System stuff
**/
void sys::initSystem(SystemInfo_s& aSystemInfo) {
    
    std::signal(SIGINT, [](int) { EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT); });
    





    aSystemInfo.os = SystemInfo_s::Platform_e::LINUX;
    aSystemInfo.processorPackageCount = 1;
    aSystemInfo.processorThreadCount = sysconf( _SC_NPROCESSORS_ONLN );
    aSystemInfo.processorCoreCount = std::thread::hardware_concurrency();

    uint64_t nBytes = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
    aSystemInfo.totalSystemMemory = nBytes / 1024 / 1024;
}
std::filesystem::path sys::openFileDialog(const std::string& aTitle, const std::vector<std::pair<std::string, std::vector<std::string>>>& aFilter){
    char filename[1024];
    
    std::string open_str = "zenity --file-selection --title='" + aTitle + "'";

    for (auto it = aFilter.begin(); it != aFilter.end(); it++){
        open_str += " --file-filter='" + it->first + " |";
        for (const std::string &str : it->second) {
            open_str += " " + str;
        }
        open_str += "'";
    }

    FILE *f = popen(open_str.c_str(), "r");
    if(fgets(filename, 1024, f) == nullptr) return "";
    int ret = pclose(f);
    if(ret != 0) return "";
    std::string path(filename);
    path.erase(std::remove(path.begin(), path.end(), '\n'), path.end());
    return path;
}
std::filesystem::path sys::saveFileDialog(const std::string& aTitle, const std::string& aExtension)
{
	std::string ext = aExtension;
	ext.erase(remove(ext.begin(), ext.end(), '.'), ext.end());
    char filename[1024];
    
    std::string open_str = "zenity --file-selection --save --title='" + aTitle + "' --filename='untitled." + ext +"'";

    FILE *f = popen(open_str.c_str(), "r");
    if(fgets(filename, 1024, f) == nullptr) return "";
    int ret = pclose(f);
    if(ret != 0) return "";
    std::string path(filename);
    path.erase(std::remove(path.begin(), path.end(), '\n'), path.end());
    if(std::filesystem::path(path).extension().string() != ("." + ext)) {
        path += "." + ext;
    }
    return path;
}
void sys::shutdown() {
	exit(0);
}


































































/**
* System Console
**/
void sys::createConsole(const std::string& aName) {
    if(isatty(fileno(stdin))){
        
    } else {
        
        
    }
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



