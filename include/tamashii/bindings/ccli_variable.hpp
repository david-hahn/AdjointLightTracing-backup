#pragma once

#include <tamashii/bindings/forward.hpp>
#include <ccli/ccli.h>
#include <nanobind/nanobind.h>
#include <cassert>

T_BEGIN_PYTHON_NAMESPACE

class CcliVar {};

template<typename T>
T ccliGetSingle(ccli::VarBase& var, std::size_t idx= 0) = delete;

template<>
inline bool ccliGetSingle<bool>(ccli::VarBase& var, const std::size_t idx) {
	const auto v = var.asBool( idx );
	assert(v.has_value());
	return *v;
}

template<>
inline long long ccliGetSingle<long long>(ccli::VarBase& var, const std::size_t idx) {
	const auto v = var.asInt( idx );
	assert(v.has_value());
	return *v;
}

template<>
inline double ccliGetSingle<double>(ccli::VarBase& var, const std::size_t idx) {
	const auto v = var.asFloat(idx);
	assert(v.has_value());
	return *v;
}

template<>
inline std::string_view ccliGetSingle<std::string_view>(ccli::VarBase& var, const std::size_t idx) {
	const auto v = var.asString(idx);
	assert(v.has_value());
	return *v;
}


template<typename T>
std::tuple<T, T> ccliGetDual(ccli::VarBase& var) {
	return { ccliGetSingle<T>(var, 0), ccliGetSingle<T>(var, 1) };
}

template<typename T>
std::tuple<T, T, T> ccliGetTriple(ccli::VarBase& var) {
	return { ccliGetSingle<T>(var, 0), ccliGetSingle<T>(var, 1), ccliGetSingle<T>(var, 2) };
}





template<typename T>
struct ccliGetMany {
	static std::vector<T> get(ccli::VarBase& var) {
		std::vector<T> vec;
		vec.reserve(var.size());
		for (size_t i = 0; i != var.size(); i++) {
			vec.emplace_back(ccliGetSingle<T>(var, i));
		}
		return vec;
	}
};

template<>
struct ccliGetMany<bool> {
	static std::vector<unsigned char> get(ccli::VarBase& var) {
		std::vector<unsigned char> vec;
		vec.reserve(var.size());
		for (size_t i = 0; i != var.size(); i++) {
			vec.emplace_back(ccliGetSingle<bool>(var, i));
		}
		return vec;
	}
};

namespace detail {
	template<typename T>
	void ccliSetSingle(ccli::VarBase& var, T val, std::size_t idx = 0) = delete;

	template<>
	inline void ccliSetSingle<bool>(ccli::VarBase& var, const bool val, const std::size_t idx) {
		bool success = var.tryStore(val, idx);
		assert(success);
	}

	template<>
	inline void ccliSetSingle<long long>(ccli::VarBase& var, const long long val, const std::size_t idx) {
		bool success = var.tryStore(val, idx);
		assert(success);
	}

	template<>
	inline void ccliSetSingle<double>(ccli::VarBase& var, const double val, const std::size_t idx) {
		bool success = var.tryStore(val, idx);
		assert(success);
	}

	template<>
	inline void ccliSetSingle<std::string_view>(ccli::VarBase& var, const std::string_view val, const std::size_t idx) {
		bool success = var.tryStore(std::string{ val }, idx);
		assert(success);
	}
}

template<typename T>
void ccliSetSingle(ccli::VarBase& var, T value) {
	detail::ccliSetSingle<T>(var, value, 0);
	var.chargeCallback();
	var.executeCallback();
}

template<typename T>
void ccliSetDual(ccli::VarBase& var, std::tuple<T, T> pair) {
	detail::ccliSetSingle<T>(var, std::get<0>(pair), 0);
	detail::ccliSetSingle<T>(var, std::get<1>(pair), 1);
	var.chargeCallback();
	var.executeCallback();
}

template<typename T>
void ccliSetTriple(ccli::VarBase& var, std::tuple<T, T, T> triplet) {
	detail::ccliSetSingle<T>(var, std::get<0>(triplet), 0);
	detail::ccliSetSingle<T>(var, std::get<1>(triplet), 1);
	detail::ccliSetSingle<T>(var, std::get<2>(triplet), 2);
	var.chargeCallback();
	var.executeCallback();
}

template<typename T>
void ccliSetMany(ccli::VarBase& var, const std::vector<T>& vals) {
	const size_t numItems = std::min(vals.size(), var.size());
	for (size_t i = 0; i != numItems; i++) {
		detail::ccliSetSingle<T>(var, vals[i], i);
	}
	var.chargeCallback();
	var.executeCallback();
}

template<typename T>
void attachVariable(nanobind::class_<CcliVar>& nbVar, ccli::VarBase& var) {
	auto& name = var.longName().empty() ? var.shortName() : var.longName();

	if (var.isCliOnly() || name.empty()) {
		return;
	}
	nbVar.def("__doc__", [&]() { return var.description(); });
	const auto sz = var.size();
	if (var.isReadOnly()) {
		switch(sz) {
		case 1:
			nbVar.def_prop_ro_static(name.c_str(), [&](nb::handle) { return ccliGetSingle<T>(var); });
			return;
		case 2:
			nbVar.def_prop_ro_static(name.c_str(), [&](nb::handle) { return ccliGetDual<T>(var); });
			return;
		case 3:
			nbVar.def_prop_ro_static(name.c_str(), [&](nb::handle) { return ccliGetTriple<T>(var); });
			return;
		default:
			nbVar.def_prop_ro_static(name.c_str(), [&](nb::handle) { return ccliGetMany<T>::get(var); });
			return;
		}
	}

	switch (sz) {
	case 1:
		nbVar.def_prop_rw_static(
			name.c_str(),
			[&](nb::handle) { return ccliGetSingle<T>(var); },
			[&](nb::handle,T val) { return ccliSetSingle<T>(var, val); }
		);
		return;
	case 2:
		nbVar.def_prop_rw_static(
			name.c_str(),
			[&](nb::handle) { return ccliGetDual<T>(var); },
			[&](nb::handle, std::tuple<T, T> pair) { return ccliSetDual<T>(var, pair); }
		);
		return;
	case 3:
		nbVar.def_prop_rw_static(
			name.c_str(),
			[&](nb::handle) { return ccliGetTriple<T>(var); },
			[&](nb::handle, std::tuple<T, T, T> triplet) { return ccliSetTriple<T>(var, triplet); }
		);
		return;
	default:
		nbVar.def_prop_rw_static(
			name.c_str(),
			[&](nb::handle) { return ccliGetMany<T>::get(var); },
			[&](nb::handle, const std::vector<T>& vals) { return ccliSetMany<T>(var, vals); }
		);
	}
}

T_END_PYTHON_NAMESPACE
