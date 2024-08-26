#pragma once
#include <tamashii/public.hpp>
#include <ccli/ccli.h>

T_BEGIN_NAMESPACE
namespace var {
	template<typename T>
	glm::vec<2, T> varToVec(const ccli::Var<T, 2>& var) { return { var.value().at(0), var.value().at(1) }; }
	template<typename T>
	glm::vec<3, T> varToVec(const ccli::Var<T, 3>& var) { return { var.value().at(0), var.value().at(1), var.value().at(2) }; }
	template<typename T>
	glm::vec<4, T> varToVec(const ccli::Var<T, 4>& var) { return { var.value().at(0), var.value().at(1), var.value().at(2), var.value().at(3) }; }

	template<typename T>
	ccli::Storage<T, 2> vecToVar(glm::vec<2, T> vec) { return { vec.x, vec.y }; }
	template<typename T>
	ccli::Storage<T, 3> vecToVar(glm::vec<3, T> vec) { return { vec.at(0), vec.at(1), vec.at(2) }; }
	template<typename T>
	ccli::Storage<T, 4> vecToVar(glm::vec<4, T> vec) { return { vec.at(0), vec.at(1), vec.at(2), vec.at(3) }; }


	extern ccli::Var<std::string> program_name;
	
	extern ccli::Var<std::string> window_title;
	extern ccli::Var<int32_t, 2> window_size;
	extern ccli::Var<bool> window_maximized;
	extern ccli::Var<int32_t, 2> window_pos;
	extern ccli::Var<bool> window_thread;
	extern ccli::Var<std::string> default_camera;
	extern ccli::Var<bool> headless;
	extern ccli::Var<bool> play_animation;
	extern ccli::Var<std::string> cfg_filename;
	extern ccli::Var<std::string> logLevel;
	extern ccli::Var<bool> gltf_io_use_watt;

	
	extern ccli::Var<std::string> render_backend;
	extern ccli::Var<bool> render_thread;
	extern ccli::Var<bool> hide_default_gui;
	extern ccli::Var<int32_t, 2> render_size;
	extern ccli::Var<bool> gpu_debug_info;
	extern ccli::Var<std::string> default_font;
	extern ccli::Var<std::string> work_dir;
	extern ccli::Var<std::string> cache_dir;
	extern ccli::Var<std::string> default_implementation;
	extern ccli::Var<uint32_t, 3> bg;
	extern ccli::Var<uint32_t, 3> light_overlay_color;
	extern ccli::Var<uint32_t> max_fps;

	extern ccli::Var<bool> unique_model_refs;
	extern ccli::Var<bool> unique_material_refs;
	extern ccli::Var<bool> unique_light_refs;

	
	extern ccli::Var<bool> vsync;
}
T_END_NAMESPACE
