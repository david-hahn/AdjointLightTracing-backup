#include <tamashii/public.hpp>
#include <tamashii/core/common/common.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>
#include <VersionHelpers.h>
#include <Shlwapi.h>

T_USE_NAMESPACE
/**
* System stuff
**/
void sys::initSystem(SystemInfo_s& aSystemInfo) {
	
	SetErrorMode(SEM_FAILCRITICALERRORS);
	
	timeBeginPeriod(1);

	
	HDC	hDC = GetDC(GetDesktopWindow());
	aSystemInfo.monitorMaxWidth = GetDeviceCaps(hDC, HORZRES);
	aSystemInfo.monitorMaxHeight = GetDeviceCaps(hDC, VERTRES);
	aSystemInfo.monitorPixelDepth = GetDeviceCaps(hDC, BITSPIXEL);
	aSystemInfo.monitorRefreshRate = GetDeviceCaps(hDC, VREFRESH);
	ReleaseDC(GetDesktopWindow(), hDC);

	aSystemInfo.os = SystemInfo_s::Platform_e::WINDOWS;
	aSystemInfo.minorVersion = 0;
	if (IsWindows10OrGreater()) aSystemInfo.majorVersion = 10;
	else if (IsWindows8Point1OrGreater()) {
		aSystemInfo.majorVersion = 8;
		aSystemInfo.minorVersion = 1;
	} else if (IsWindows8OrGreater()) aSystemInfo.majorVersion = 8;
	else if (IsWindows7OrGreater()) aSystemInfo.majorVersion = 7;

	DWORD length = 0;
	GetLogicalProcessorInformation(nullptr, &length);
	uint8_t *buffer = new uint8_t[length];
	GetLogicalProcessorInformation((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)buffer, &length);
	for (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)buffer; length - sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) > 0; ptr++, length -= sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) {
		if (ptr->Relationship == RelationProcessorCore) {
			aSystemInfo.processorCoreCount++;
			DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
			ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
			for (DWORD i = 0; i <= LSHIFT; i++) {
				aSystemInfo.processorThreadCount += ((ptr->ProcessorMask & bitTest) ? 1 : 0);
				bitTest /= 2;
			}
		} else if(ptr->Relationship == RelationProcessorPackage) aSystemInfo.processorPackageCount++;
	}
	delete[] buffer;

	ULONGLONG TotalMemoryInKilobytes;
	GetPhysicallyInstalledSystemMemory(&TotalMemoryInKilobytes);
	aSystemInfo.totalSystemMemory = static_cast<int>(TotalMemoryInKilobytes / 1024);
}
std::filesystem::path sys::openFileDialog(const std::string& aTitle, const std::vector<std::pair<std::string, std::vector<std::string>>>& aFilter)
{
	
	int pointer = 0;
	char filter_arr[MAX_PATH];
	std::string filter_str;
	for (std::pair<std::string, std::vector<std::string>> pair : aFilter) {
		strncpy(filter_arr + pointer, pair.first.c_str(), pair.first.size());
		strncpy(filter_arr + pointer + pair.first.size(), "\0", 1);
		pointer += pair.first.size() + 1;
		for (std::string str : pair.second) {
			filter_str += str + ";";
			strncpy(filter_arr + pointer, str.c_str(), str.size());
			strncpy(filter_arr + pointer + str.size(), ";", 1);
			pointer += str.size() + 1;
		}
		strncpy(filter_arr + pointer - 1, "\0", 1);
	}
	strncpy(filter_arr + pointer, "\0", MAX_PATH - pointer);

	char szFile[MAX_PATH];
	OPENFILENAMEA ofn = {};
	ofn.lpstrTitle = aTitle.c_str();
	ofn.lStructSize = sizeof(ofn);
	
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter_arr;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameA(&ofn)) {
		return std::string(szFile);
	}
	return std::string();
}
std::filesystem::path sys::saveFileDialog(const std::string& aTitle, const std::string& aExtension)
{
	std::string ext = aExtension;
	ext.erase(remove(ext.begin(), ext.end(), '.'), ext.end());
	char szFile[MAX_PATH];
	szFile[0] = 'u';szFile[1] = 'n';szFile[2] = 't';szFile[3] = 'i';szFile[4] = 't';
	szFile[5] = 'l';szFile[6] = 'e';szFile[7] = 'd';szFile[8] = '\0';
	OPENFILENAMEA ofn = {};
	ofn.lpstrTitle = aTitle.c_str();
	ofn.lStructSize = sizeof(ofn);
	
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrInitialDir = nullptr;
	ofn.lpstrDefExt = ext.c_str();
	ofn.Flags = OFN_NOCHANGEDIR;
	if (GetSaveFileNameA(&ofn)) {
		return std::string(szFile);
	}
	return std::string();
}
void sys::shutdown() {
	exit(0);
}

/**
* System Console
**/
void sys::createConsole(const std::string& aName) {
	SetConsoleTitle(aName.c_str());
	AllocConsole();
	/*win32.expliciteConsoleW = false;
	if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
		AllocConsole();
		win32.expliciteConsoleW = true;
	}
	freopen_s(&win32.console_streams, "CONIN$", "r", stdin);
	freopen_s(&win32.console_streams, "CONOUT$", "w", stderr);
	freopen_s(&win32.console_streams, "CONOUT$", "w", stdout);*/
}
void sys::destroyConsole() {
	FreeConsole();
	/*if (win32.expliciteConsoleW) {
		FreeConsole();
	}
	fclose(win32.console_streams);*/
}

void* sys::loadLibrary(const std::string& aName)
{
	return LoadLibrary(aName.c_str());
}

bool sys::unloadLibrary(void* aLib)
{
	return FreeLibrary(static_cast<HMODULE>(aLib)) > 0;
}

void* sys::loadFunction(void* aLib, const std::string& aName)
{
	return (void*) GetProcAddress(static_cast<HMODULE>(aLib) , aName.c_str());
}

