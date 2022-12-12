// defs.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include <unordered_set>
#include <unordered_map>

#include <zst.h>
#include <zpr.h>

#include "error.h"
#include "units.h"

namespace util
{

	// clang-format off
	template <typename A>
	concept has_hash_method = requires(A a)
	{
		{ a.hash() } -> std::same_as<size_t>;
	};
	// clang-format on

	// https://en.cppreference.com/w/cpp/container/unordered_map/find
	// stupid language
	struct hasher
	{
		using is_transparent = void;
		using H = std::hash<std::string_view>;

		size_t operator()(const char* str) const { return H {}(str); }
		size_t operator()(std::string_view str) const { return H {}(str); }
		size_t operator()(const std::string& str) const { return H {}(str); }

		size_t operator()(const has_hash_method auto& a) const { return a.hash(); }
	};

	template <typename K, typename V, typename H = hasher>
	using hashmap = std::unordered_map<K, V, H, std::equal_to<>>;

	template <typename T>
	using hashset = std::unordered_set<T, hasher, std::equal_to<>>;
}

namespace sap
{
	template <typename T>
	using ErrorOr = zst::Result<T, std::string>;

	using zst::Ok;
	using zst::Err;
	using zst::ErrFmt;

	using zst::Result;
	using zst::Failable;

	namespace impl
	{
		template <typename T, typename E>
		struct extract_value_or_return_void
		{
			T extract(Result<T, E>& result) { return std::move(result.unwrap()); }
		};

		template <typename E>
		struct extract_value_or_return_void<void, E>
		{
			void extract([[maybe_unused]] Result<void, E>& result) { }
		};
	}

#define TRY(x)                                                      \
	__extension__({                                                 \
		auto&& r = x;                                               \
		using R = std::decay_t<decltype(r)>;                        \
		using V = typename R::value_type;                           \
		using E = typename R::error_type;                           \
		if(r.is_err())                                              \
			return Err(std::move(r.error()));                       \
		sap::impl::extract_value_or_return_void<V, E>().extract(r); \
	})
}

inline void zst::error_and_exit(const char* str, size_t len)
{
	sap::internal_error("{}", zst::str_view(str, len));
}
