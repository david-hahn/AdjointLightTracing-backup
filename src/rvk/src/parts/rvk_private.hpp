#pragma once
#include <rvk/rvk.hpp>

#ifndef NDEBUG
#define ASSERT(condition, message) \
    if (! (condition)) { \
        Logger::error("Assertion `" #condition "` failed in " + std::string(__FILE__) + " line " + std::to_string(__LINE__) + ": " + std::string(message) + ""); \
        std::terminate(); \
    }
#else
#define ASSERT
#endif

#define SET_DEVICE(device) device = (device) ? (device) : InstanceManager::getDefaultInstance()->defaultDevice();

RVK_BEGIN_NAMESPACE

template <typename T, typename A>
T* toArrayPointer(std::vector<T, A>& vector) { return vector.size() > 0 ? vector.data() : nullptr; }

template<typename T>
T rountUpToMultipleOf(const T numToRound, const T multiple) { return (numToRound + multiple - 1) & -multiple; }
template<typename T>
T isBitSet(const T value, const T bit) { return (value & bit) == bit; }
template<typename T>
T isOnlyAnyBitSet(const T value, const T bit) { return (value & bit) == value; }
template<typename T>
T isOnlyBitSet(const T value, const T bit) { return (value == bit); }
template<typename T>
T isAnyBitSet(const T value, const T bits) { return (value & bits) > 0; }
template<typename T>
T areAllBitsSet(const T value, const T bits) { return (value & bits) == value; }

RVK_END_NAMESPACE