// util.h
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bit> // for endian, endian::big, endian::native
#include <numeric>
#include <variant>
#include <concepts>
#include <string_view> // for hash, string_view, u32string_view

#include "zst.h" // for str_view, byte_span, wstr_view, Result, span
#include "types.h"
#include "endian.h"

namespace util
{
	template <typename T>
	using big_endian_span = zst::span<T, std::endian::big>;

	template <typename It>
	struct subrange
	{
		using value_type = typename std::iterator_traits<It>::value_type;
		It m_begin;
		It m_end;
		subrange(It begin, It end) : m_begin(begin), m_end(end) { }
		It begin() const { return m_begin; }
		It end() const { return m_end; }
	};

	template <typename... Xs>
	struct overloaded : Xs...
	{
		using Xs::operator()...;
	};

	template <typename... Xs>
	overloaded(Xs...) -> overloaded<Xs...>;




	template <typename T, std::same_as<T>... Ts>
	static constexpr bool is_one_of(T foo, Ts... foos)
	{
		return (false || ... || (foo == foos));
	}

	std::pair<uint8_t*, size_t> readEntireFile(const std::string& path);

	uint16_t convertBEU16(uint16_t x);
	uint32_t convertBEU32(uint32_t x);

	template <std::integral From>
	std::make_unsigned_t<From> to_unsigned(From f)
	{
		return static_cast<std::make_unsigned_t<From>>(f);
	}

	template <std::integral To, std::integral From>
	requires requires()
	{
		requires sizeof(To) < sizeof(From);
	}
	To checked_cast(From f)
	{
		if constexpr(std::signed_integral<To> && std::unsigned_integral<From>)
		{
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		else if constexpr(std::unsigned_integral<To> && std::signed_integral<From>)
		{
			assert(f >= 0);
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		else
		{
			// same signedness
			assert(f >= static_cast<From>(std::numeric_limits<To>::min()));
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		return static_cast<To>(f);
	}

	template <std::integral To, std::integral From>
	requires requires()
	{
		requires sizeof(To) == sizeof(From);
	}
	To checked_cast(From f)
	{
		if constexpr(std::signed_integral<To> && std::unsigned_integral<From>)
		{
			assert(f <= static_cast<From>(std::numeric_limits<To>::max()));
		}
		else if constexpr(std::unsigned_integral<To> && std::signed_integral<From>)
		{
			assert(f >= 0);
		}
		else
		{
			// same signedness and same size, no checks required
		}
		return static_cast<To>(f);
	}

	template <std::integral To, std::integral From>
	requires requires()
	{
		requires sizeof(To) > sizeof(From);
	}
	To checked_cast(From f)
	{
		if constexpr(std::signed_integral<To> && std::unsigned_integral<From>)
		{
			// To bigger signed, no check required
		}
		else if constexpr(std::unsigned_integral<To> && std::signed_integral<From>)
		{
			assert(f >= 0);
		}
		else
		{
			// same signedness and casting to bigger size, no check required
		}
		return static_cast<To>(f);
	}


	// Convert type of `shared_ptr`, via `dynamic_cast`
	// This exists because libstdc++ is a dum dum
	template <typename To, typename From>
	inline std::shared_ptr<To> dynamic_pointer_cast(const std::shared_ptr<From>& from) //
	    noexcept requires std::derived_from<To, From>
	{
		if(auto* to = dynamic_cast<typename std::shared_ptr<To>::element_type*>(from.get()))
			return std::shared_ptr<To>(from, to);
		return std::shared_ptr<To>();
	}

	template <typename To, typename From>
	inline std::unique_ptr<To> static_pointer_cast(std::unique_ptr<From>&& from) //
	    noexcept requires std::derived_from<To, From>
	{
		return std::unique_ptr<To>(static_cast<To*>(from.release()));
	}


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
		using WH = std::hash<std::u32string_view>;

		size_t operator()(const char* str) const { return H {}(str); }
		size_t operator()(zst::str_view str) const { return H {}(str.sv()); }
		size_t operator()(std::string_view str) const { return H {}(str); }
		size_t operator()(const std::string& str) const { return H {}(str); }

		size_t operator()(zst::wstr_view str) const { return WH {}(str.sv()); }
		size_t operator()(std::u32string_view str) const { return WH {}(str); }
		size_t operator()(const std::u32string& str) const { return WH {}(str); }

		size_t operator()(const has_hash_method auto& a) const { return a.hash(); }
	};

	template <typename K, typename V, typename H = hasher>
	using hashmap = std::unordered_map<K, V, H, std::equal_to<>>;

	template <typename T>
	using hashset = std::unordered_set<T, hasher, std::equal_to<>>;

	namespace impl
	{
		template <typename T, typename E>
		struct extract_value_or_return_void
		{
			T extract(zst::Result<T, E>& result) { return std::move(result.unwrap()); }
		};

		template <typename E>
		struct extract_value_or_return_void<void, E>
		{
			void extract([[maybe_unused]] zst::Result<void, E>& result) { }
		};
	}

	template <typename Callback>
	struct Defer
	{
		Defer(Callback cb) : m_cb(std::move(cb)), m_cancel(false) { }
		~Defer()
		{
			if(not m_cancel)
				m_cb();
		}

		void cancel() { m_cancel = true; }

		Defer(const Defer&) = delete;
		Defer& operator=(const Defer&) = delete;

	private:
		Callback m_cb;
		bool m_cancel;
	};

	struct ShortString
	{
		using value_type = char;
		using iterator = char*;
		using const_iterator = const char*;

		char s[16] = { 0 };
		ShortString() = default;
		ShortString(const char* bytes, size_t len)
		{
			if(len > 16)
				sap::internal_error("short string not so short eh?");

			std::copy(bytes, bytes + len, s);
		}
		ShortString(zst::str_view sv)
		{
			if(sv.length() > 16)
				sap::internal_error("short string not so short eh?");

			std::copy(sv.begin(), sv.end(), s);
		}

		char* data() { return s; }
		const char* data() const { return s; }
		char* begin() { return s; }
		const char* begin() const { return s; }
		char* end() { return s + size(); }
		const char* end() const { return s + size(); }

		ShortString& operator+=(zst::str_view sv)
		{
			append(sv);
			return *this;
		}

		ShortString operator+(zst::str_view sv) const
		{
			ShortString copy = *this;
			copy.append(sv);
			return copy;
		}

		void append(zst::str_view sv)
		{
			if(size() + sv.size() > 16)
				sap::internal_error("short string would be too long");

			std::copy(sv.begin(), sv.end(), end());
		}

		void push_back(char ch)
		{
			if(size() >= 16)
				sap::internal_error("short string would be too long");

			s[size()] = ch;
		}

		void pop_back()
		{
			if(size() == 0)
				sap::internal_error("short string already empty");

			s[size() - 1] = 0;
		}

		size_t length() const { return size(); }
		size_t size() const
		{
			return s[0] == 0  ? 0
			     : s[1] == 0  ? 1
			     : s[2] == 0  ? 2
			     : s[3] == 0  ? 3
			     : s[4] == 0  ? 4
			     : s[5] == 0  ? 5
			     : s[6] == 0  ? 6
			     : s[7] == 0  ? 7
			     : s[8] == 0  ? 8
			     : s[9] == 0  ? 9
			     : s[10] == 0 ? 10
			     : s[11] == 0 ? 11
			     : s[12] == 0 ? 12
			     : s[13] == 0 ? 13
			     : s[14] == 0 ? 14
			     : s[15] == 0 ? 15
			                  : 16;
		}

		operator zst::str_view() const { return zst::str_view(s, this->size()); }
		operator std::string_view() const { return std::string_view(s, this->size()); }
		operator std::string() const { return std::string(s, this->size()); }

		std::strong_ordering operator<=>(const ShortString&) const = default;

		size_t hash() const
		{
			size_t hinum;
			size_t lonum;
			std::copy(s, s + 8, (char*) &lonum);
			std::copy(s + 8, s + 16, (char*) &hinum);
			// pack bits,
			// shift by 4 because every other 4 bytes in ascii contain entropy,
			// the other half is constant.
			// e.g. a = 01100000
			return lonum ^ (hinum >> 4);
		}
	};


	template <typename T, std::same_as<T>... Ts>
	std::vector<T> vectorOf(T&& x, Ts&&... xs)
	{
		std::vector<T> ret {};
		ret.push_back(static_cast<T&&>(x));
		(ret.push_back(static_cast<Ts&&>(xs)), ...);

		return ret;
	}
}

namespace unicode
{
	std::string utf8FromUtf16(zst::span<uint16_t> utf16);
	std::string utf8FromUtf16BigEndianBytes(zst::byte_span bytes);

	std::string utf8FromCodepoint(char32_t cp);

	char32_t consumeCodepointFromUtf8(zst::byte_span& utf8);

	std::u32string u32StringFromUtf8(zst::byte_span sv);
	std::u32string u32StringFromUtf8(zst::str_view sv);

	std::string stringFromU32String(zst::wstr_view sv);

	// high-order surrogate first.
	std::pair<uint16_t, uint16_t> codepointToSurrogatePair(char32_t codepoint);
}

namespace zpr
{
	template <>
	struct print_formatter<std::u32string>
	{
		template <typename Cb>
		void print(const std::u32string& x, Cb&& cb, format_args args)
		{
			auto s = unicode::stringFromU32String(x);
			detail::print_one(static_cast<Cb&&>(cb), std::move(args), std::move(s));
		}
	};
}
