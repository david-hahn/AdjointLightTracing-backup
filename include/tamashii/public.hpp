#pragma once
#include <tamashii/core/defines.h>

#include <spdlog/spdlog.h>

#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#define GLM_FORCE_INTRINSICS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            spdlog::error("Assertion `" #condition "` failed in {} line {}: {}", __FILE__, __LINE__, message); \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) condition
#endif

T_BEGIN_NAMESPACE
template<typename T>
T_INLINE bool inBounds(const T low, const T high, const T val) { return ((val >= low) && (val <= high)); }
template<typename T>
T_INLINE bool isBitSet(const T value, const T bit) { return ((value & bit) == bit); }
template<typename T>
T_INLINE bool isAnyBitSet(const T value, const T bits) { return ((value & bits) > 0); }

template<typename T>
T_INLINE T rountUpToMultipleOf(const T numToRound, const T multiple) { return (numToRound + multiple - 1) & -multiple; }
T_END_NAMESPACE


constexpr uint32_t VAR_SYSTEM_BUCKET_SIZE = 8;
constexpr uint32_t CMD_SYSTEM_BUCKET_SIZE = 8;

constexpr float LIGHT_OVERLAY_RADIUS = 0.1f;

constexpr char const* SCREENSHOT_FILE_NAME = "screenshot";

constexpr char const* DEFAULT_CAMERA_NAME = "Default";
constexpr char const* DEFAULT_MATERIAL_NAME = "default_material";

constexpr uint32_t FPS_FRAMES = 30;
