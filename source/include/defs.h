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
	// https://en.cppreference.com/w/cpp/container/unordered_map/find
	// stupid language
	struct hasher
	{
		using is_transparent = void;
		using H = std::hash<std::string_view>;

		size_t operator()(const char* str) const { return H {}(str); }
		size_t operator()(std::string_view str) const { return H {}(str); }
		size_t operator()(const std::string& str) const { return H {}(str); }

		template <typename A, typename B>
		size_t operator()(const std::pair<A, B>& p) const
		{
			return std::hash<A> {}(p.first) ^ std::hash<B> {}(p.second);
		}
	};

	template <typename K, typename V>
	using hashmap = std::unordered_map<K, V, hasher, std::equal_to<>>;

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
}

inline void zst::error_and_exit(const char* str, size_t len)
{
	sap::internal_error("{}", zst::str_view(str, len));
}
