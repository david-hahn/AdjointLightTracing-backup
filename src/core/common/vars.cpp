#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/common/input.hpp>

#include <filesystem>

T_USE_NAMESPACE

ccli::Var<std::string> load_scene("", "load_scene", "", ccli::Flag::ManualExec, "Load a scene on startup", [](const ccli::Storage<std::string>& v) {
	if (v.data.empty()) {
		spdlog::warn("CmdSystem: could not execute load scene cmd");
		return;
	}
	std::filesystem::path path(v.data);
	if (path.is_absolute()) path = path.lexically_proximate(std::filesystem::current_path());
	EventSystem::queueEvent(EventType::ACTION, Input::A_OPEN_SCENE, 0, 0, 0, path.make_preferred().string());
	
});


ccli::Var<std::string> tamashii::var::program_name("", "program_name", "Tamashii", ccli::Flag::CliOnly, "Name of the program");

ccli::Var<std::string> tamashii::var::window_title("", "window_title", "Tamashii", ccli::Flag::CliOnly, "Title of the main window");
ccli::Var<int32_t, 2> tamashii::var::window_size("", "window_size", { 1280, 720 }, ccli::Flag::ConfigRdwr, "Main render window size width,height");
ccli::Var<bool> tamashii::var::window_maximized("", "window_maximized", false, ccli::Flag::ConfigRdwr, "Is main window maximized");
ccli::Var<int32_t, 2> tamashii::var::window_pos("", "window_pos", {0, 0}, ccli::Flag::ConfigRdwr, "Main render window position");
ccli::Var<bool> tamashii::var::window_thread("", "window_thread", true, ccli::Flag::CliOnly | ccli::Flag::ConfigRead, "Use a dedicated window thread");
ccli::Var<std::string> tamashii::var::default_camera("", "default_camera", DEFAULT_CAMERA_NAME, ccli::Flag::ConfigRead, "Camera that should be selected as default when a scene is loaded");
ccli::Var<bool> tamashii::var::headless("", "headless", false, ccli::Flag::ConfigRead, "Start Tamashii without window");
ccli::Var<bool> tamashii::var::play_animation("", "play_animation", false, ccli::Flag::CliOnly, "Play animation on startup");
ccli::Var<std::string> tamashii::var::cfg_filename("", "cfg_filename", "tamashii.cfg", ccli::Flag::None, "Name of the config file");
ccli::Var<bool> tamashii::var::gltf_io_use_watt("", "gltf_io_use_watt", false, ccli::Flag::ConfigRead, "Use watt instead of correct light units for gltf io");

#define LOG_LEVEL_VAR(l) ccli::Var<std::string> tamashii::var::logLevel("", "log_level", (l), ccli::Flag::None, "Set spdlog logging level", [](const std::string& sv) { spdlog::set_level(spdlog::level::from_str(sv)); });
#ifndef NDEBUG
LOG_LEVEL_VAR("debug")
#else
LOG_LEVEL_VAR("info")
#endif
#undef LOG_LEVEL_VAR

ccli::Var<std::string> tamashii::var::render_backend("", "render_backend", "vulkan", ccli::Flag::ConfigRead, "Render backend to use");
ccli::Var<bool> tamashii::var::render_thread("", "render_thread", false, ccli::Flag::ConfigRead, "Use a dedicated rendering thread");
ccli::Var<bool> tamashii::var::hide_default_gui("", "hide_default_gui", false, ccli::Flag::ConfigRead, "Do not show the default gui");
ccli::Var<int32_t, 2> tamashii::var::render_size("", "render_size", { 400,400 }, ccli::Flag::ConfigRead, "Render size width,height; only used when in headless mode");
ccli::Var<bool> tamashii::var::gpu_debug_info("", "gpu_debug_info", false, ccli::Flag::CliOnly, "Print GPU infos like available extentions");
ccli::Var<std::string> tamashii::var::default_font("", "default_font", "assets/fonts/Cousine-Regular.ttf", ccli::Flag::ConfigRdwr, "Path to the font file");
ccli::Var<std::string> tamashii::var::work_dir("", "work_dir", ".", ccli::Flag::None, "Set the working dir");
ccli::Var<std::string> tamashii::var::cache_dir("", "cache_dir", "cache", ccli::Flag::ConfigRdwr, "Dir for storing caches");
ccli::Var<std::string> tamashii::var::default_implementation("", "default_implementation", "Rasterizer", ccli::Flag::ConfigRead, "Startup implementation");
ccli::Var<uint32_t, 3> tamashii::var::bg("", "bg", { 30,30,30 }, ccli::Flag::ConfigRead, "Render background");
ccli::Var<uint32_t, 3> tamashii::var::light_overlay_color("", "light_overlay_color", { 220,220,220 }, ccli::Flag::ConfigRead, "Light overlay color");
ccli::Var<uint32_t> tamashii::var::max_fps("","max_fps", 0, ccli::Flag::ConfigRdwr, "Set max fps (0 = no limit)");

ccli::Var<bool> tamashii::var::unique_model_refs("", "unique_model_refs", false, ccli::Flag::ConfigRead, "");
ccli::Var<bool> tamashii::var::unique_material_refs("", "unique_material_refs", false, ccli::Flag::ConfigRead, "");
ccli::Var<bool> tamashii::var::unique_light_refs("", "unique_light_refs", false, ccli::Flag::ConfigRead, "");


ccli::Var<bool> tamashii::var::vsync("", "vsync", true, ccli::Flag::CliOnly | ccli::Flag::ConfigRdwr, "Turn vsync ON/OFF");