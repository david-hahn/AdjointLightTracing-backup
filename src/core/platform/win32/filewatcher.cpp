#include <tamashii/core/platform/filewatcher.hpp>

#include <direct.h>
#include <Shlwapi.h>
#include <algorithm>
#include <filesystem>
#include <utility>

T_USE_NAMESPACE
/*
* FileWatcher
*/
FileWatcher::FileWatcher() : hDir(nullptr) {}
FileWatcher::~FileWatcher() = default;
void FileWatcher::watchFile(std::string aFilePath, std::function<void()> aCallback)
{
	const std::lock_guard lock(mMutex);
	if (aFilePath.empty()) return;
	
	const std::string remove = "./";
	size_t index;
	if ((index = aFilePath.find(remove)) != std::string::npos && index == 0) aFilePath.erase(0, remove.size());
	aFilePath = std::filesystem::path(aFilePath).make_preferred().string();

	
	if (mWatchHashList.find(aFilePath) != mWatchHashList.end()) return;

	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	while (fileHandle == INVALID_HANDLE_VALUE) fileHandle = CreateFileA(aFilePath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	FILETIME fileTime;
	GetFileTime(fileHandle, nullptr, nullptr, &fileTime);
	CloseHandle(fileHandle);
	const uint64_t time = (static_cast<uint64_t>(fileTime.dwHighDateTime) << 32) | (fileTime.dwLowDateTime);

	

	std::pair<std::string, triple_s> pair(aFilePath, { aFilePath, time, std::move(aCallback)});
	mWatchHashList.insert(pair);
}
void FileWatcher::removeFile(std::string aFilePath)
{
	const std::lock_guard lock(mMutex);
	if (aFilePath.empty()) return;
	
	const std::string remove = "./";
	size_t index;
	if ((index = aFilePath.find(remove)) != std::string::npos && index == 0) aFilePath.erase(0, remove.size());
	aFilePath = std::filesystem::path(aFilePath).make_preferred().string();

	mWatchHashList.erase(aFilePath);
}
inline std::string wcharToString(const WCHAR* aWchar, const int aLength) {
	char filename[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, aWchar, aLength / 2, filename, sizeof(filename), nullptr, nullptr);
	filename[aLength / 2] = '\0';
	
	
	return filename;
}

void FileWatcher::check()
{
	constexpr int byBufferSize = 32 * 1024; 
	BYTE* byBuffer = new BYTE[byBufferSize];
	DWORD dwBytesRet;

	if(mInitCallback) mInitCallback();

	hDir = CreateFileA(
		"./",
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr
	);
	if (hDir == nullptr) {
		spdlog::error("Filewatcher: Could not open root dir");
		return;
	}

	OVERLAPPED pollingOverlap = {};
	pollingOverlap.OffsetHigh = 0;
	pollingOverlap.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
	if (!pollingOverlap.hEvent) spdlog::error("Filewatcher: Could not create Event.");

	while (ReadDirectoryChangesW(hDir, byBuffer, byBufferSize, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION, &dwBytesRet, &pollingOverlap, nullptr))
	{
		BYTE* p = &byBuffer[0];
		for (int i = 0;; i++) {
			const auto pNotify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(p);
			if (pNotify->Action == FILE_ACTION_MODIFIED) {
				std::string str = wcharToString(pNotify->FileName, pNotify->FileNameLength);
				auto search = mWatchHashList.find(str);
				if (search != mWatchHashList.end()) {
					
					FILETIME fileTime;
					HANDLE fileHandle = INVALID_HANDLE_VALUE;
					while (fileHandle == INVALID_HANDLE_VALUE) fileHandle = CreateFileA(str.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
					GetFileTime(fileHandle, nullptr, nullptr, &fileTime);
					CloseHandle(fileHandle);
					const uint64_t time = (static_cast<uint64_t>(fileTime.dwHighDateTime) << 32) | (fileTime.dwLowDateTime);

					
					triple_s& f = search->second;
					if (time > f.lastWrite) {
						f.lastWrite = time + 500000; 
						std::this_thread::sleep_for(std::chrono::milliseconds(2));
						f.callback();
					}
					break;
				}
			}
			if (!pNotify->NextEntryOffset) break;
			p += pNotify->NextEntryOffset;
		}
	}
	delete[] byBuffer;
	CloseHandle(hDir);
	if (mShutdownCallback) mShutdownCallback();
}
void FileWatcher::print() const
{
	spdlog::info("tFileWatcher content:");
	for (const auto& f : mWatchHashList) {
		spdlog::info("   watching: {}", f.second.filePath);
	}
}
std::thread FileWatcher::spawn()
{
	std::thread t(&FileWatcher::check, this);
	spdlog::info("FileWatcher thread spawned");
	return t;
}
void FileWatcher::terminate() {
	spdlog::info("...terminating FileWatcher thread");
	CancelIoEx(hDir, nullptr);
}