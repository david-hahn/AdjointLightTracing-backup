#include <rvk/parts/logger.hpp>

RVK_USE_NAMESPACE

std::function<void(const std::string&)> rvk::Logger::mInfoCallback;
std::function<void(const std::string&)> rvk::Logger::mDebugCallback;
std::function<void(const std::string&)> rvk::Logger::mWarningCallback;
std::function<void(const std::string&)> rvk::Logger::mCriticalCallback;
std::function<void(const std::string&)> rvk::Logger::mErrorCallback;

void Logger::info(const std::string& aString)
{
	if (mInfoCallback) mInfoCallback(aString);
	else std::cout << aString << std::endl;
}

void Logger::debug(const std::string& aString)
{
	if (mDebugCallback) mDebugCallback(aString);
	else std::cout << aString << std::endl;
}

void Logger::warning(const std::string& aString)
{
	if (mWarningCallback) mWarningCallback(aString);
	else std::cout << aString << std::endl;
}

void Logger::critical(const std::string& aString)
{
	if (mCriticalCallback) mCriticalCallback(aString);
	else std::cout << aString << std::endl;
}

void Logger::error(const std::string& aString)
{
	if (mErrorCallback) mErrorCallback(aString);
	else std::cerr << aString << std::endl;
}

void Logger::setInfoCallback(void(* aCallback)(const std::string&))
{
	mInfoCallback = aCallback;
}

void Logger::setDebugCallback(void(* aCallback)(const std::string&))
{
	mDebugCallback = aCallback;
}

void Logger::setWarningCallback(void(* aCallback)(const std::string&))
{
	mWarningCallback = aCallback;
}

void Logger::setCriticalCallback(void(* aCallback)(const std::string&))
{
	mCriticalCallback = aCallback;
}

void Logger::setErrorCallback(void(* aCallback)(const std::string&))
{
	mErrorCallback = aCallback;
}
