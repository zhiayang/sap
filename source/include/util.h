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

	namespace impl
	{
		struct mmap_deleter
		{
			mmap_deleter(size_t size) : m_size(size) { }
			void operator()(const void* ptr);

			size_t m_size;
		};
	}

	template <typename T>
	requires(std::is_array_v<T>) //
	    using mmap_ptr = std::unique_ptr<T, impl::mmap_deleter>;

	std::pair<mmap_ptr<uint8_t[]>, size_t> readEntireFile(const std::string& path);


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
	std::u32string u32StringFromUtf8(std::string_view sv);

	std::string stringFromU32String(std::u32string_view sv);

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

	template <>
	struct print_formatter<zst::wstr_view>
	{
		template <typename Cb>
		void print(zst::wstr_view x, Cb&& cb, format_args args)
		{
			detail::print(static_cast<Cb&&>(cb), "{}", unicode::stringFromU32String(x.sv()));
		}
	};
}
