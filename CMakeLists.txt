cmake_minimum_required(VERSION 3.21.0)
###########################################################################
# Disable insource build

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR
		"CMake generation for this project is not allowed within the source directory!"
		"\n Remove the CMakeCache.txt file and try again from another folder, e.g.:"
		"\n "
		"\n rm CMakeCache.txt"
		"\n cd .."
		"\n mkdir cmake-make"
		"\n cd cmake-make"
		"\n cmake ../project"
	)
endif()

# look for vcpkg environment variable
# if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#   	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
# endif()

# PROJECT
project(Tamashii DESCRIPTION "Tamashii" LANGUAGES CXX C)
# SET
set(APP tamashii)
set(LIB_CORE tamashii_core)
set(LIB_CORE_BINDINGS tamashii_core_bindings)
set(LIB_RENDERER_VK tamashii_renderer_vk)
set(LIB_CUDA_HELPER tamashii_cuda_helper)
set(LIB_IMPL tamashii_implementations)
set(LIB_RVK rvk)
set(ASSETS assets)

set(FRAMEWORK_EXTERNAL_FOLDER "External")
set(FRAMEWORK_TARGET_FOLDER "Framework")
set(FRAMEWORK_EXAMPLE_FOLDER "Examples")
set(FRAMEWORK_TEST_FOLDER "Test")

set(CUDA_HELPER off)

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(INCLUDE_DIR "${SOURCE_DIR}/include")
set(RVK_INCLUDE_DIR "${SOURCE_DIR}/src/rvk/include")
set(EXTERNAL_DIR "${SOURCE_DIR}/external")

option(BUILD_TAMASHII "Build tamashii" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(USE_AVX "Activate AVX instructions" ON)
option(USE_AVX2 "Activate AVX2 instructions" ON)
option(TAMASHII_TESTS "Activate Testing" OFF)
option(TAMASHII_VULKAN_VALIDATION_LAYERS "Activate Vulkan Validation Layers" ON)

# COMPILER
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Qpar")
	if(USE_AVX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
	endif()
	if(USE_AVX2)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
	endif()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings")
	if(USE_AVX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
	endif()
	if(USE_AVX2)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
	endif()

	if(WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldwmapi")
	elseif(APPLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")
	endif()
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  	message("IPO is not supported: ${output}")
endif()

# DEFINES
#add_definitions(-DVK_NO_PROTOTYPES)
if(WIN32)
	add_definitions(
		-DNOMINMAX
		-D_SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_NONSTDC_NO_DEPRECATE
		-D_SCL_SECURE_NO_WARNINGS
		-DVK_USE_PLATFORM_WIN32_KHR
		-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
	)
elseif(APPLE)
	add_definitions(
		-DVK_USE_PLATFORM_MACOS_MVK
		-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
	)
elseif(UNIX)
	add_definitions(
		-DVK_USE_PLATFORM_XCB_KHR
		-DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
	)
endif()
if(TAMASHII_VULKAN_VALIDATION_LAYERS)
	add_definitions(
		-DTAMASHII_VULKAN_VALIDATION_LAYERS
	)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# OUTPUT DIR
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")

# External Builds
message(STATUS "<fmt>")
add_subdirectory("${EXTERNAL_DIR}/fmt")
message(STATUS "</fmt>")
message(STATUS "<spdlog>")
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")
add_subdirectory("${EXTERNAL_DIR}/spdlog" EXCLUDE_FROM_ALL)
message(STATUS "</spdlog>")
set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(fmt spdlog PROPERTIES FOLDER ${FRAMEWORK_EXTERNAL_FOLDER})
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HLSL OFF)
set(ENABLE_CTEST OFF)
set(ENABLE_GLSLANG_INSTALL OFF)
set(ENABLE_PCH OFF)
set(ENABLE_OPT OFF)
set(ENABLE_SPVREMAPPER OFF)
set(BUILD_EXTERNAL OFF)
message(STATUS "<glslang>")
add_subdirectory("${EXTERNAL_DIR}/glslang" EXCLUDE_FROM_ALL)
message(STATUS "</glslang>")
set_target_properties(glslang MachineIndependent GenericCodeGen OSDependent SPIRV glslang-default-resource-limits PROPERTIES FOLDER ${FRAMEWORK_EXTERNAL_FOLDER})

add_subdirectory(src)
add_subdirectory(assets)

if(TAMASHII_TESTS)
	enable_testing()
	add_subdirectory("${EXTERNAL_DIR}/catch2" EXCLUDE_FROM_ALL)
	set_target_properties(Catch2 PROPERTIES FOLDER ${FRAMEWORK_EXTERNAL_FOLDER})
	set_target_properties(Catch2WithMain PROPERTIES FOLDER ${FRAMEWORK_EXTERNAL_FOLDER})
	add_subdirectory(test)
endif()

# ===============================#
# Unset unneeded cache variables #
# ===============================#

# unset(CMAKE_BUILD_TYPE CACHE)
# unset(CMAKE_CONFIGURATION_TYPES CACHE)
# unset(CMAKE_INSTALL_PREFIX CACHE)

unset(ALLOW_EXTERNAL_SPIRV_TOOLS CACHE)
unset(BUILD_SHARED_LIBS CACHE)
unset(ENABLE_EXCEPTIONS CACHE)
unset(ENABLE_GLSLANG_JS CACHE)
unset(ENABLE_HLSL CACHE)
unset(ENABLE_OPT CACHE)
unset(ENABLE_RTTI CACHE)
unset(ENABLE_SPVREMAPPER CACHE)
unset(SKIP_GLSLANG_INSTALL CACHE)
unset(OVERRIDE_MSVCCRT CACHE)
unset(LLVM_USE_CRT_DEBUG CACHE)
unset(LLVM_USE_CRT_MINSIZEREL CACHE)
unset(LLVM_USE_CRT_RELEASE CACHE)
unset(LLVM_USE_CRT_RELWITHDEBINFO CACHE)
unset(USE_CCACHE CACHE)

unset(nanobind_DIR CACHE)

unset(BUILD_WERROR CACHE)
unset(FMT_CMAKE_DIR CACHE)
unset(FMT_TEST CACHE)
unset(FMT_DOC CACHE)
unset(FMT_INSTALL CACHE)
unset(FMT_PEDANTIC CACHE)
unset(FMT_WERROR CACHE)
unset(FMT_CUDA_TEST CACHE)
unset(FMT_FUZZ CACHE)
unset(FMT_INC_DIR CACHE)
unset(FMT_LIB_DIR CACHE)
unset(FMT_OS CACHE)
unset(FMT_DEBUG_POSTFIX CACHE)
unset(FMT_SYSTEM_HEADERS CACHE)
unset(FMT_PKGCONFIG_DIR CACHE)
unset(FMT_MODULE CACHE)

unset(SPDLOG_BUILD_ALL CACHE)
unset(SPDLOG_BUILD_SHARED CACHE)
unset(SPDLOG_ENABLE_PCH CACHE)
unset(SPDLOG_BUILD_PIC CACHE)
unset(SPDLOG_BUILD_EXAMPLE CACHE)
unset(SPDLOG_BUILD_EXAMPLE_HO CACHE)
unset(SPDLOG_BUILD_TESTS CACHE)
unset(SPDLOG_BUILD_TESTS_HO CACHE)
unset(SPDLOG_BUILD_BENCH CACHE)
unset(SPDLOG_SANITIZE_ADDRESS CACHE)
unset(SPDLOG_BUILD_WARNINGS CACHE)
unset(SPDLOG_INSTALL CACHE)
unset(SPDLOG_USE_STD_FORMAT CACHE)
unset(SPDLOG_FMT_EXTERNAL CACHE)
unset(SPDLOG_FMT_EXTERNAL_HO CACHE)
unset(SPDLOG_NO_EXCEPTIONS CACHE)
unset(SPDLOG_WCHAR_SUPPORT CACHE)
unset(SPDLOG_WCHAR_FILENAMES CACHE)
unset(SPDLOG_CLOCK_COARSE CACHE)
unset(SPDLOG_PREVENT_CHILD_FD CACHE)
unset(SPDLOG_NO_THREAD_ID CACHE)
unset(SPDLOG_NO_TLS CACHE)
unset(SPDLOG_DISABLE_DEFAULT_LOGGER CACHE)
unset(SPDLOG_NO_ATOMIC_LEVELS CACHE)
unset(SPDLOG_TIDY CACHE)
unset(SPDLOG_SYSTEM_INCLUDES CACHE)
