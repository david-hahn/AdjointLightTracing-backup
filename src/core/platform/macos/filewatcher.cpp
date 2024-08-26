#include <tamashii/engine/platform/filewatcher.hpp>


#include <algorithm>

T_USE_NAMESPACE
/*
* FileWatcher
*/
FileWatcher::FileWatcher() {};
FileWatcher::~FileWatcher() {};
void FileWatcher::watchFile(std::string filePath, std::function<void()> callback)
{
	
}
void FileWatcher::removeFile(std::string filePath)
{
	
}

void FileWatcher::check()
{
	
}
void FileWatcher::print() const
{
	
}
std::thread FileWatcher::spawn()
{
	std::thread t(&FileWatcher::check, this);
	spdlog::info("FileWatcher thread spawned");
	return t;
}
void FileWatcher::terminate() {
	spdlog::info("...terminating FileWatcher thread");
}
