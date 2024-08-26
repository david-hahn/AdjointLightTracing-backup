#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class Logger
{
public:
	static void info(const std::string& aString);
	static void debug(const std::string& aString);
	static void warning(const std::string& aString);
	static void critical(const std::string& aString);
	static void error(const std::string& aString);

	/* Set custom callbacks */
	static void setInfoCallback(void (*aCallback)(const std::string&));
	static void setDebugCallback(void (*aCallback)(const std::string&));
	static void setWarningCallback(void (*aCallback)(const std::string&));
	static void setCriticalCallback(void (*aCallback)(const std::string&));
	static void setErrorCallback(void (*aCallback)(const std::string&));
private:
	static std::function<void(const std::string&)> mInfoCallback;
	static std::function<void(const std::string&)> mDebugCallback;
	static std::function<void(const std::string&)> mWarningCallback;
	static std::function<void(const std::string&)> mCriticalCallback;
	static std::function<void(const std::string&)> mErrorCallback;
};

RVK_END_NAMESPACE
