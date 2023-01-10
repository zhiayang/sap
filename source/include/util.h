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

#include "misc/hasher.h"

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

	zst::unique_span<uint8_t[]> readEntireFile(const std::string& path);



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

	struct Defer
	{
		static constexpr size_t STORAGE_SIZE = 16;
		struct Base
		{
			virtual ~Base() { }
			virtual void operator()() const = 0;
		};

		template <typename Callback>
		Defer(Callback cb) : m_cancel(false)
		{
			struct Derived : Base
			{
				Derived(Callback&& cb) : cb(std::move(cb)) { }
				virtual void operator()() const override { cb(); }
				Callback cb;
			};

			static_assert(sizeof(Derived) <= STORAGE_SIZE);
			new(&m_storage[0]) Derived(std::move(cb));
		}

		~Defer()
		{
			if(not m_cancel)
				(*(Base*) &m_storage[0])();
		}

		void cancel() { m_cancel = true; }

		Defer(const Defer&) = delete;
		Defer& operator=(const Defer&) = delete;

	private:
		bool m_cancel;
		uint8_t m_storage[STORAGE_SIZE];
	};




	struct DeferMemberFn
	{
		template <typename T>
		DeferMemberFn(const T* self, void (T::*member_fn)())
		    : m_self(self)
		    , m_method(member_fn)
		    , m_cancel(false)
		{
			m_callback = [](const void* self, const void* fn_) {
				auto fn = reinterpret_cast<void (T::*)()>(fn_);
				(static_cast<const T*>(self)->*fn)();
			};
		}

		~DeferMemberFn()
		{
			if(not m_cancel)
				m_callback(m_self, m_method);
			// m_cb();
		}

		void cancel() { m_cancel = true; }

		DeferMemberFn(const DeferMemberFn&) = delete;
		DeferMemberFn& operator=(const DeferMemberFn&) = delete;

	private:
		const void* m_self;
		const void* m_method;
		void (*m_callback)(const void*, const void*);
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

	inline uint16_t convertBEU16(uint16_t x)
	{
		// Untested cause who has big endian anyway
		if constexpr(std::endian::native == std::endian::big)
			return x;
		else
			return zst::byteswap(x);
	}

	inline uint32_t convertBEU32(uint32_t x)
	{
		// Untested cause who has big endian anyway
		if constexpr(std::endian::native == std::endian::big)
			return x;
		else
			return zst::byteswap(x);
	}

	inline uint64_t convertBEU64(uint64_t x)
	{
		// Untested cause who has big endian anyway
		if constexpr(std::endian::native == std::endian::big)
			return x;
		else
			return zst::byteswap(x);
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
