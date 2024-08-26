#pragma once

#include <tamashii/bindings/bindings.hpp>
#include <tamashii/core/render/render_backend.hpp>

#include <memory>

T_BEGIN_PYTHON_NAMESPACE
	class CoreModule {
public:
	static Exports& exportCore(nanobind::module_&);

	static void initGuard();
	static void exitCallback(std::function<void()> cb);

	template<typename... Args>
	static void exitWithError(const fmt::format_string<Args...> fmt, Args &&...args)
	{
		const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
		throw std::runtime_error(msg.c_str());
	}
private:
	CoreModule() = default;
	~CoreModule() = default;
	static CoreModule& the()
	{
		static CoreModule instance;
		return instance;
	}

	std::unique_ptr<Exports> mExports;
	std::shared_ptr<spdlog::logger> mScriptLogger;
};

T_END_PYTHON_NAMESPACE
