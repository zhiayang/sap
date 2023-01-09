// hasher.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace util
{
	// clang-format off
	template <typename A>
	concept has_hash_method = requires(A a)
	{
		{ a.hash() } -> std::same_as<size_t>;
	};

	template <typename A>
	concept has_hash_specialisation = requires(A a)
	{
		{ std::hash<A>{}(a) } -> std::same_as<size_t>;
	};
	// clang-format on

	// https://en.cppreference.com/w/cpp/container/unordered_map/find
	// stupid language
	struct hasher
	{
		using is_transparent = void;
		using H = std::hash<std::string_view>;
		using WH = std::hash<std::u32string_view>;

		size_t operator()(const char* str) const { return H {}(str); }
		size_t operator()(zst::str_view str) const { return H {}(str.sv()); }
		size_t operator()(std::string_view str) const { return H {}(str); }
		size_t operator()(const std::string& str) const { return H {}(str); }

		size_t operator()(zst::wstr_view str) const { return WH {}(str.sv()); }
		size_t operator()(std::u32string_view str) const { return WH {}(str); }
		size_t operator()(const std::u32string& str) const { return WH {}(str); }

		template <has_hash_method T>
		requires(not has_hash_specialisation<T>) size_t operator()(const T& x) const
		{
			return x.hash();
		}

		template <has_hash_specialisation T>
		size_t operator()(const T& a) const
		{
			return std::hash<std::remove_cvref_t<decltype(a)>>()(a);
		}
	};

}
