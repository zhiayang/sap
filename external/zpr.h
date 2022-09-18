/*
	zpr.h
	Copyright 2020 - 2022, zhiayang

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.



	detail::print_floating and detail::print_exponent are adapted from _ftoa and _etoa
	from https://github.com/mpaland/printf, which is licensed under the MIT license,
	reproduced below:

	Copyright Marco Paland (info@paland.com), 2014-2019, PALANDesign Hannover, Germany

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/


/*
	Version 2.7.3
	=============



	Documentation
	=============

	This printing library functions as a lightweight alternative to
	std::format in C++20 (or fmtlib), with the following (non)features:

	1. no exceptions
	2. short and simple implementation without tons of templates
	3. no support for positional arguments (for now)

	Otherwise, it should support most of the "typical use-case" features that std::format provides. In essence it is a type-safe
	alternative to printf supporting custom type printers. The usage is as such:

	zpr::println("{<spec>}", argument);

	where `<spec>` is exactly a `printf`-style format specifier (note: there is no leading colon unlike the fmtlib/python style),
	and where the final type specifier (eg. `s`, `d`) is optional. Floating point values will print as if `g` was used. Size
	specifiers (eg. `lld`) are not supported. Variable width and precision specifiers (eg. `%.*s`) are not supported.

	The currently supported builtin formatters are:
	- integral types            (signed/unsigned char/short/int/long/long long) (but not 'char')
	- floating point types      (float, double, long double)
	- strings                   (char*, const char*, anything container value_type == 'char')
	- booleans                  (prints as 'true'/'false')
	- void*, const void*        (prints with %p)
	- chars                     (char)
	- enums                     (will print as their integer representation)
	- std::pair                 (prints as "{ first, second }")
	- all iterable containers   (with begin(x) and end(x) available -- prints as "[ a, b, ..., c ]")

	For non-constant widths and precisions, there are 3 functions: w(int), p(int), and wp(int, int). these
	functions return a function object, which you should then call with the actual argument value. usage:

	old: zpr::println("{.*}", 3, M_PI);
	new: zpr::println("{}", zpr::p(3)(M_PI));

	for wp(), the width is the first argument, and the precision the second argument. Note that you
	could use this everywhere and not put the widths in the format string at all.

	There are optional #define macros to control behaviour; defining them without a value constitues
	a TRUE -- for FALSE, either `#undef` them, or explicitly define them as 0. For values that are TRUE
	by default, you *must* explicitly `#define` them as 0.

	- ZPR_USE_STD
		this is *TRUE* by default. controls whether or not STL type interfaces are used; with it,
		you get the std::pair printer, and the sprint() overload that returns std::string. that's
		about it. iterator-based container printing is not affected by this flag.

	- ZPR_HEX_0X_RESPECTS_UPPERCASE
		this is *FALSE* by default. basically, if you use '{X}', this setting determines whether
		you'll get '0xDEADBEEF' or '0XDEADBEEF'. i think the capital 'X' looks ugly as heck, so this
		is OFF by default.

	- ZPR_DECIMAL_LOOKUP_TABLE
		this is *TRUE* by default. controls whether we use a lookup table to increase the speed of
		decimal printing. this uses 201 bytes.

	- ZPR_HEXADECIMAL_LOOKUP_TABLE
		this is *TRUE* by default. controls whether we use a lookup table to increase the speed of
		hex printing. this uses 1025 bytes.

	- ZPR_FREESTANDING
		this is *FALSE by default; controls whether or not a standard library implementation is
		available. if not, then the following changes are made:
		(a) the print() family of functions that would print to stdout, and the fprint() family that
			would print to a FILE* are no longer available.

		(b) memset(), memcpy(), memmove(), strlen(), and strncpy() are forward declared according to
			the C library specifications, but they are not defined. they are expected to be defined
			elsewhere in the program.


	Custom Formatters
	-----------------

	To format custom types, specialise the print_formatter struct. An example of how it should be done can be seen from
	the builtin formatters, taking note to follow the signatures.

	A key point to note is that you can specialise both for the 'raw' type -- ie. you can specialise for T&, and also
	for T; the non-decayed specialisation is preferred if both exist.

	(NB: the reason for this is so that we can support reference to array of char, ie. char (&)[N])



	Function List
	-------------

	the type 'tt::str_view' is an simplified version of std::string_view, and has
	implicit constructors from 'const char*' as well as const char (&)[N] (which means we don't
	call strlen() on string literals).

	for user-defined string-like types, feel free to implement operator tt::str_view() for
	an implicit conversion to be able to use those as a format string.

	* print to a callback function, given by the templated parameter 'callback'.
	* it should be a function object defining "operator() (const char*, size_t)",
	* and should print the string pointed to by the first argument, with length
	* specified by the second argument.
	size_t cprint(const _CallbackFn& callback, tt::str_view fmt, Args&&... args);

	* print to a buffer, given by the pointer 'buf', with maximum size 'len'.
	* no NULL terminator will be appended to the buffer, including in the situation
	* where the buffer is maximally filled; thus you might want to reserve space for a NULL
	* byte if necessary.
	size_t sprint(char* buf, size_t len, tt::str_view fmt, Args&&... args);

	* only available if ZPR_USE_STD == 1: print to a std::string
	std::string sprint(tt::str_view fmt, Args&&... args);

	* only available if ZPR_FREESTANDING != 0: print to stdout.
	size_t print(tt::str_view fmt, Args&&... args);

	* only available if ZPR_FREESTANDING != 0: print to stdout, followed by a newline '\n'.
	size_t println(tt::str_view fmt, Args&&... args);

	* only available if ZPR_FREESTANDING != 0: print to the specified FILE*
	size_t fprint(FILE* file, tt::str_view fmt, Args&&... args);

	* only available if ZPR_FREESTANDING != 0: print to the specified FILE*, followed by a newline '\n'.
	size_t fprintln(FILE* file, tt::str_view fmt, Args&&... args);

	* similar to sprint(), but it is meant to be used as an argument to an "outer" call to another
	* print function; the purpose is to avoid an unnecessary round-trip through a std::string.
	auto fwd(tt::str_view fmt, Args&&... args);

	* instead of having to specialise a print_formatter<T>, use this to specify the formatter for a
	* certain type using a lambda. The formatter given must return some sort of string-like object.
	auto zpr::with(T value, auto&& formatter);


	Type-erased API
	---------------

	Most of the formatting functions above are also available with a 'v' prefix, which is the type-erased
	version. The aim of this is to reduce the number of variadic template instantiatons, limiting it to the
	top-level entry-point (the `vprintX` function itself).

	This slightly increases the runtime cost (since there is now an indirect call through a function pointer),
	but now, there is one instantiation per callback type (`file_appender`, `buffer_appender`, etc.), and one
	for each combination of (callback_type, value_type).

	Of course, there will still be one template for each combination of argument types for the top-level function,
	but that is unavoidable --- we try to keep those functions small so code-size-explosion is reduced.


	----
	Version history has been moved to the bottom of the file.
*/

#pragma once

#include <cfloat>
#include <cstddef>
#include <cstdint>

#define ZPR_DO_EXPAND(VAL)  VAL ## 1
#define ZPR_EXPAND(VAL)     ZPR_DO_EXPAND(VAL)

// this weird macro soup is necessary to defend against the case where you just do
// #define ZPR_FOO_BAR, but without a value -- we want to treat that as a TRUE.
#if !defined(ZPR_HEX_0X_RESPECTS_UPPERCASE)
	#define ZPR_HEX_0X_RESPECTS_UPPERCASE 0
#elif (ZPR_EXPAND(ZPR_HEX_0X_RESPECTS_UPPERCASE) == 1)
	#undef ZPR_HEX_0X_RESPECTS_UPPERCASE
	#define ZPR_HEX_0X_RESPECTS_UPPERCASE 1
#endif

#if !defined(ZPR_FREESTANDING)
	#define ZPR_FREESTANDING 0
#elif (ZPR_EXPAND(ZPR_FREESTANDING) == 1)
	#undef ZPR_FREESTANDING
	#define ZPR_FREESTANDING 1
#endif

// it needs to be like this so we don't throw a redefinition warning.
#if !defined(ZPR_DECIMAL_LOOKUP_TABLE)
	#define ZPR_DECIMAL_LOOKUP_TABLE 1
#elif (ZPR_EXPAND(ZPR_DECIMAL_LOOKUP_TABLE) == 1)
	#undef ZPR_DECIMAL_LOOKUP_TABLE
	#define ZPR_DECIMAL_LOOKUP_TABLE 1
#endif

#if !defined(ZPR_HEXADECIMAL_LOOKUP_TABLE)
	#define ZPR_HEXADECIMAL_LOOKUP_TABLE 1
#elif (ZPR_EXPAND(ZPR_HEXADECIMAL_LOOKUP_TABLE) == 1)
	#undef ZPR_HEXADECIMAL_LOOKUP_TABLE
	#define ZPR_HEXADECIMAL_LOOKUP_TABLE 1
#endif

#if !defined(ZPR_USE_STD)
	#define ZPR_USE_STD 1
#elif (ZPR_EXPAND(ZPR_USE_STD) == 1)
	#undef ZPR_USE_STD
	#define ZPR_USE_STD 1
#endif


#if !ZPR_FREESTANDING
	#include <cstdio>
	#include <cstring>
#else
	#if defined(ZPR_USE_STD)
		#undef ZPR_USE_STD
	#endif

	#define ZPR_USE_STD 0

	extern "C" void* memset(void* s, int c, size_t n);
	extern "C" void* memcpy(void* dest, const void* src, size_t n);
	extern "C" void* memmove(void* dest, const void* src, size_t n);

	extern "C" size_t strlen(const char* s);
	extern "C" int strncmp(const char* s1, const char* s2, size_t n);

#endif

#if !ZPR_FREESTANDING && ZPR_USE_STD
	#include <string>
	#include <string_view>
#endif


#undef ZPR_DO_EXPAND
#undef ZPR_EXPAND


// we can't really use [[likely]]/[[unlikely]] since (a) they go in a different place
// and (b) I want to keep the minimum c++ version as c++17.
#if defined(_MSC_VER)
	#define ZPR_ALWAYS_INLINE __forceinline inline
	#define ZPR_UNLIKELY(x)   (x)
	#define ZPR_LIKELY(x)     (x)
#elif defined(__GNUC__)
	#define ZPR_ALWAYS_INLINE __attribute__((always_inline)) inline
	#define ZPR_UNLIKELY(x)   __builtin_expect((x), 0)
	#define ZPR_LIKELY(x)     __builtin_expect((x), 1)
#else
	#define ZPR_ALWAYS_INLINE
	#define ZPR_UNLIKELY(x)   (x)
	#define ZPR_LIKELY(x)     (x)
#endif




#include <type_traits>


namespace zpr::tt
{
	// just use <type_traits> since it's a freestanding thing
	using namespace std;


	struct __invalid {};

	template <typename _Type> _Type _Minimum(const _Type& a, const _Type& b) { return a < b ? a : b; }
	template <typename _Type> _Type _Maximum(const _Type& a, const _Type& b) { return a > b ? a : b; }
	template <typename _Type> _Type _Absolute(const _Type& x) { return x < 0 ? -x : x; }

	template <typename _Type>
	void swap(_Type& t1, _Type& t2)
	{
		_Type temp = static_cast<_Type&&>(t1);
		t1 = static_cast<_Type&&>(t2);
		t2 = static_cast<_Type&&>(temp);
	}


	// is_any<X, A, B, ... Z> -> is_same<X, A> || is_same<X, B> || ...
	template <typename _Type, typename... _Ts>
	struct is_any : disjunction<is_same<_Type, _Ts>...> { };

	struct str_view
	{
		using value_type = char;

		str_view() : ptr(nullptr), len(0) { }
		str_view(const char* p, size_t l) : ptr(p), len(l) { }

		template <size_t _Number>
		str_view(const char (&s)[_Number]) : ptr(s), len(_Number - 1) { }

		template <typename _Type, typename = tt::enable_if_t<tt::is_same_v<const char*, _Type>>>
		str_view(_Type s) : ptr(s), len(strlen(s)) { }

		str_view(str_view&&) = default;
		str_view(const str_view&) = default;
		str_view& operator= (str_view&&) = default;
		str_view& operator= (const str_view&) = default;

		inline bool operator== (const str_view& other) const
		{
			return (this->ptr == other.ptr && this->len == other.len)
				|| (this->len == other.len && strncmp(this->ptr, other.ptr, this->len) == 0);
		}

		inline bool operator!= (const str_view& other) const
		{
			return !(*this == other);
		}

		inline const char* begin() const { return this->ptr; }
		inline const char* end() const { return this->ptr + len; }

		inline size_t size() const { return this->len; }
		inline bool empty() const { return this->len == 0; }
		inline const char* data() const { return this->ptr; }

		inline char operator[] (size_t n) { return this->ptr[n]; }

		inline str_view drop(size_t n) const { return (this->size() >= n ? this->substr(n, this->size() - n) : ""); }
		inline str_view take(size_t n) const { return (this->size() >= n ? this->substr(0, n) : *this); }
		inline str_view take_last(size_t n) const { return (this->size() >= n ? this->substr(this->size() - n, n) : *this); }
		inline str_view drop_last(size_t n) const { return (this->size() >= n ? this->substr(0, this->size() - n) : *this); }

		inline str_view& remove_prefix(size_t n) { return (*this = this->drop(n)); }
		inline str_view& remove_suffix(size_t n) { return (*this = this->drop_last(n)); }

		[[nodiscard]] inline str_view take_prefix(size_t n)
		{
			auto ret = this->take(n);
			this->remove_prefix(n);
			return ret;
		}

		inline size_t find(char c) const { return this->find(str_view(&c, 1)); }
		inline size_t find(str_view sv) const
		{
			if(sv.size() > this->size())
				return static_cast<size_t>(-1);

			else if(sv.empty())
				return 0;

			for(size_t i = 0; i < 1 + this->size() - sv.size(); i++)
			{
				if(this->drop(i).take(sv.size()) == sv)
					return i;
			}

			return static_cast<size_t>(-1);
		}

		inline str_view substr(size_t pos, size_t cnt) const { return str_view(this->ptr + pos, cnt); }


	#if ZPR_USE_STD

		str_view(const std::string& str) : ptr(str.data()), len(str.size()) { }
		str_view(const std::string_view& sv) : ptr(sv.data()), len(sv.size()) { }

		[[nodiscard]] inline std::string str() const { return std::string(this->ptr, this->len); }
	#endif


	private:
		const char* ptr;
		size_t len;
	};
}

namespace zpr
{
	constexpr uint8_t FMT_FLAG_ZERO_PAD         = 0x1;
	constexpr uint8_t FMT_FLAG_ALTERNATE        = 0x2;
	constexpr uint8_t FMT_FLAG_PREPEND_PLUS     = 0x4;
	constexpr uint8_t FMT_FLAG_PREPEND_SPACE    = 0x8;
	constexpr uint8_t FMT_FLAG_HAVE_WIDTH       = 0x10;
	constexpr uint8_t FMT_FLAG_HAVE_PRECISION   = 0x20;
	constexpr uint8_t FMT_FLAG_WIDTH_NEGATIVE   = 0x40;

	struct format_args
	{
		char specifier      = -1;
		uint8_t flags       = 0;

		int64_t width       = -1;
		int64_t length      = -1;
		int64_t precision   = -1;

		ZPR_ALWAYS_INLINE bool zero_pad() const       { return this->flags & FMT_FLAG_ZERO_PAD; }
		ZPR_ALWAYS_INLINE bool alternate() const      { return this->flags & FMT_FLAG_ALTERNATE; }
		ZPR_ALWAYS_INLINE bool have_width() const     { return this->flags & FMT_FLAG_HAVE_WIDTH; }
		ZPR_ALWAYS_INLINE bool have_precision() const { return this->flags & FMT_FLAG_HAVE_PRECISION; }
		ZPR_ALWAYS_INLINE bool prepend_plus() const   { return this->flags & FMT_FLAG_PREPEND_PLUS; }
		ZPR_ALWAYS_INLINE bool prepend_space() const  { return this->flags & FMT_FLAG_PREPEND_SPACE; }

		ZPR_ALWAYS_INLINE bool negative_width() const { return have_width() && (this->flags & FMT_FLAG_WIDTH_NEGATIVE); }
		ZPR_ALWAYS_INLINE bool positive_width() const { return have_width() && !negative_width(); }

		ZPR_ALWAYS_INLINE void set_precision(int64_t p)
		{
			this->precision = p;
			this->flags |= FMT_FLAG_HAVE_PRECISION;
		}

		ZPR_ALWAYS_INLINE void set_width(int64_t w)
		{
			this->width = w;
			this->flags |= FMT_FLAG_HAVE_WIDTH;

			if(w < 0)
			{
				this->width = -w;
				this->flags |= FMT_FLAG_WIDTH_NEGATIVE;
			}
		}
	};

	template <typename _Type, typename = void>
	struct print_formatter { };


	namespace detail
	{
		template <typename _Type>
		struct __fmtarg_w
		{
			__fmtarg_w(__fmtarg_w&&) = delete;
			__fmtarg_w(const __fmtarg_w&) = delete;
			__fmtarg_w& operator= (__fmtarg_w&&) = delete;
			__fmtarg_w& operator= (const __fmtarg_w&) = delete;

			__fmtarg_w(_Type&& x, int width) : arg(static_cast<_Type&&>(x)), width(width) { }

			_Type arg;
			int width;
		};

		template <typename _Type>
		struct __fmtarg_p
		{
			__fmtarg_p(__fmtarg_p&&) = delete;
			__fmtarg_p(const __fmtarg_p&) = delete;
			__fmtarg_p& operator= (__fmtarg_p&&) = delete;
			__fmtarg_p& operator= (const __fmtarg_p&) = delete;

			__fmtarg_p(_Type&& x, int prec) : arg(static_cast<_Type&&>(x)), prec(prec) { }

			_Type arg;
			int prec;
		};

		template <typename _Type>
		struct __fmtarg_wp
		{
			__fmtarg_wp(__fmtarg_wp&&) = delete;
			__fmtarg_wp(const __fmtarg_wp&) = delete;
			__fmtarg_wp& operator= (__fmtarg_wp&&) = delete;
			__fmtarg_wp& operator= (const __fmtarg_wp&) = delete;

			__fmtarg_wp(_Type&& x, int width, int prec) : arg(static_cast<_Type&&>(x)), prec(prec), width(width) { }

			_Type arg;
			int prec;
			int width;
		};

		struct __fmtarg_w_helper
		{
			__fmtarg_w_helper(int w) : width(w) { }

			template <typename _Type>
			ZPR_ALWAYS_INLINE __fmtarg_w<_Type&&> operator() (_Type&& val)
			{
				return __fmtarg_w<_Type&&>(static_cast<_Type&&>(val), this->width);
			}

			int width;
		};

		struct __fmtarg_p_helper
		{
			__fmtarg_p_helper(int p) : prec(p) { }

			template <typename _Type>
			ZPR_ALWAYS_INLINE __fmtarg_p<_Type&&> operator() (_Type&& val)
			{
				return __fmtarg_p<_Type&&>(static_cast<_Type&&>(val), this->prec);
			}

			int prec;
		};

		struct __fmtarg_wp_helper
		{
			__fmtarg_wp_helper(int w, int p) : width(w), prec(p) { }

			template <typename _Type>
			ZPR_ALWAYS_INLINE __fmtarg_wp<_Type&&> operator() (_Type&& val)
			{
				return __fmtarg_wp<_Type&&>(static_cast<_Type&&>(val), this->width, this->prec);
			}

			int width;
			int prec;
		};

		template <typename... _Types>
		struct __forward_helper
		{
			template <typename... Xs>
			ZPR_ALWAYS_INLINE __forward_helper(tt::str_view fmt, Xs&&... xs) : fmt(static_cast<tt::str_view&&>(fmt))
			{
				size_t idx = 0;
				((values[idx++] = &xs), ...);
			}

			tt::str_view fmt;

			// msvc doesn't like a 0-sized array here
			const size_t num_values = sizeof...(_Types);
			const void* values[sizeof...(_Types) == 0 ? 1 : sizeof...(_Types)] { };
		};

		struct dummy_appender
		{
			void operator() (char c);
			void operator() (tt::str_view sv);
			void operator() (char c, size_t n);
			void operator() (const char* begin, const char* end);
			void operator() (const char* begin, size_t len);
		};

		template <typename _Type, typename = void>
		struct has_formatter : tt::false_type { };

		template <typename _Type>
		struct has_formatter<_Type, tt::void_t<decltype(tt::declval<print_formatter<_Type>>()
			.print(tt::declval<_Type>(), dummy_appender(), format_args{}))>
		> : tt::true_type { };

		template <typename _Type>
		constexpr bool has_formatter_v = has_formatter<_Type>::value;


		template <typename _Type, typename = void>
		struct has_required_size_calc : tt::false_type { };

		template <typename _Type>
		struct has_required_size_calc<_Type, tt::enable_if_t<tt::is_integral_v<decltype(tt::declval<print_formatter<_Type>>()
			.length(tt::declval<_Type>(), format_args{}))>>
		> : tt::true_type { };

		template <typename _Type>
		constexpr bool has_required_size_calc_v = has_required_size_calc<_Type>::value;






		template <typename _Type, typename = void>
		struct is_iterable : tt::false_type { };

		template <typename _Type>
		struct is_iterable<_Type, tt::void_t<
			decltype(begin(tt::declval<_Type&>())), decltype(end(tt::declval<_Type&>()))
		>> : tt::true_type { };

		// a bit hacky, but force this to be iterable.
		template <>
		struct is_iterable<tt::str_view> : tt::true_type { };

		static inline format_args parse_fmt_spec(tt::str_view sv)
		{
			// remove the first and last (they are { and })
			sv = sv.drop(1).drop_last(1);

			format_args fmt_args = { };
			{
				while(sv.size() > 0)
				{
					switch(sv[0])
					{
						case '0':   fmt_args.flags |= FMT_FLAG_ZERO_PAD; sv.remove_prefix(1); continue;
						case '#':   fmt_args.flags |= FMT_FLAG_ALTERNATE; sv.remove_prefix(1); continue;
						case '-':   fmt_args.flags |= FMT_FLAG_WIDTH_NEGATIVE; sv.remove_prefix(1); continue;
						case '+':   fmt_args.flags |= FMT_FLAG_PREPEND_PLUS; sv.remove_prefix(1); continue;
						case ' ':   fmt_args.flags |= FMT_FLAG_PREPEND_SPACE; sv.remove_prefix(1); continue;
						default:    break;
					}

					break;
				}

				if(sv.empty())
					goto done;

				if('0' <= sv[0] && sv[0] <= '9')
				{
					fmt_args.flags |= FMT_FLAG_HAVE_WIDTH;

					size_t k = 0;
					fmt_args.width = 0;

					while(sv.size() > k && ('0' <= sv[k] && sv[k] <= '9'))
						(fmt_args.width = 10 * fmt_args.width + (sv[k] - '0')), k++;

					sv.remove_prefix(k);
				}

				if(sv.empty())
					goto done;

				if(sv.size() >= 2 && sv[0] == '.')
				{
					sv.remove_prefix(1);

					if(sv[0] == '-')
					{
						// just ignore negative precision i guess.
						size_t k = 1;
						while(sv.size() > k && ('0' <= sv[k] && sv[k] <= '9'))
							k++;

						sv.remove_prefix(k);
					}
					else if('0' <= sv[0] && sv[0] <= '9')
					{
						fmt_args.flags |= FMT_FLAG_HAVE_PRECISION;
						fmt_args.precision = 0;

						size_t k = 0;
						while(sv.size() > k && ('0' <= sv[k] && sv[k] <= '9'))
							(fmt_args.precision = 10 * fmt_args.precision + (sv[k] - '0')), k++;

						sv.remove_prefix(k);
					}
				}

				if(!sv.empty())
					fmt_args.specifier = sv[0];
			}

		done:
			return fmt_args;
		}

		template <typename _CallbackFn>
		ZPR_ALWAYS_INLINE size_t print_string(_CallbackFn& cb, const char* str, size_t len, format_args args)
		{
			int64_t string_length = 0;

			if(args.have_precision())   string_length = tt::_Minimum(args.precision, static_cast<int64_t>(len));
			else                        string_length = static_cast<int64_t>(len);

			size_t ret = static_cast<size_t>(string_length);
			auto padding_width = args.width - string_length;

			if(args.positive_width() && padding_width > 0)
			{
				cb(args.zero_pad() ? '0' : ' ', static_cast<size_t>(padding_width));
				ret += static_cast<size_t>(padding_width);
			}

			cb(str, static_cast<size_t>(string_length));

			if(args.negative_width() && padding_width > 0)
			{
				cb(args.zero_pad() ? '0' : ' ', static_cast<size_t>(padding_width));
				ret += static_cast<size_t>(padding_width);
			}

			return ret;
		}

		template <typename _CallbackFn>
		ZPR_ALWAYS_INLINE size_t print_special_floating(_CallbackFn& cb, double value, format_args args)
		{
			// uwu. apparently, `inf` and `nan` are never truncated.
			args.set_precision(999);

			if(value != value)
				return print_string(cb, "nan", 3, static_cast<format_args&&>(args));

			if(value < -DBL_MAX)
				return print_string(cb, "-inf", 4, static_cast<format_args&&>(args));

			if(value > DBL_MAX)
			{
				return print_string(cb, args.prepend_plus()
					? "+inf" : args.prepend_space()
					? " inf" : "inf",
					args.prepend_space() || args.prepend_plus() ? 4 : 3,
					static_cast<format_args&&>(args)
				);
			}

			return 0;
		}




		// forward declare these
		template <typename _CallbackFn>
		size_t print_floating(_CallbackFn& cb, double value, format_args args);

		template <typename _Type>
		char* print_decimal_integer(char* buf, size_t bufsz, _Type value);



		template <typename _CallbackFn>
		size_t print_exponent(_CallbackFn& cb, double value, format_args args)
		{
			constexpr int DEFAULT_PRECISION = 6;

			// check for NaN and special values
			if((value != value) || (value > DBL_MAX) || (value < -DBL_MAX))
				return print_special_floating(cb, value, static_cast<format_args&&>(args));

			int prec = (args.have_precision() ? static_cast<int>(args.precision) : DEFAULT_PRECISION);

			bool use_precision  = args.have_precision();
			bool use_zero_pad   = args.zero_pad() && args.positive_width();
			bool use_right_pad  = !use_zero_pad && args.negative_width();
			// bool use_left_pad   = !use_zero_pad && args.positive_width();

			// determine the sign
			const bool negative = (value < 0);
			if(negative)
				value = -value;

			// determine the decimal exponent
			// based on the algorithm by David Gay (https://www.ampl.com/netlib/fp/dtoa.c)
			union {
				uint64_t U;
				double F;
			} conv;

			conv.F = value;
			auto exp2 = static_cast<int64_t>((conv.U >> 52U) & 0x07FFU) - 1023; // effectively log2
			conv.U = (conv.U & ((1ULL << 52U) - 1U)) | (1023ULL << 52U);        // drop the exponent so conv.F is now in [1,2)

			// now approximate log10 from the log2 integer part and an expansion of ln around 1.5
			auto expval = static_cast<int64_t>(0.1760912590558 + exp2 * 0.301029995663981 + (conv.F - 1.5) * 0.289529654602168);

			// now we want to compute 10^expval but we want to be sure it won't overflow
			exp2 = static_cast<int64_t>(static_cast<double>(expval) * 3.321928094887362 + 0.5);

			const double z = static_cast<double>(expval) * 2.302585092994046
				- static_cast<double>(exp2) * 0.6931471805599453;
			const double z2 = z * z;

			conv.U = static_cast<uint64_t>(exp2 + 1023) << 52U;

			// compute exp(z) using continued fractions, see https://en.wikipedia.org/wiki/Exponential_function#Continued_fractions_for_ex
			conv.F *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));

			// correct for rounding errors
			if(value < conv.F)
			{
				expval--;
				conv.F /= 10;
			}

			// the exponent format is "%+02d" and largest value is "307", so set aside 4-5 characters (including the e+ part)
			int minwidth = (-100 < expval && expval < 100) ? 4U : 5U;

			// in "%g" mode, "prec" is the number of *significant figures* not decimals
			if(args.specifier == 'g' || args.specifier == 'G')
			{
				// do we want to fall-back to "%f" mode?
				if((value >= 1e-4) && (value < 1e6))
				{
					if(static_cast<int64_t>(prec) > expval)
						prec = static_cast<int>(static_cast<int64_t>(prec) - expval - 1);

					else
						prec = 0;

					args.precision = prec;

					// no characters in exponent
					minwidth = 0;
					expval = 0;
				}
				else
				{
					// we use one sigfig for the whole part
					if(prec > 0 && use_precision)
						prec -= 1;
				}
			}

			// will everything fit?
			auto fwidth = static_cast<uint64_t>(args.width);
			if(args.width > minwidth)
			{
				// we didn't fall-back so subtract the characters required for the exponent
				fwidth -= static_cast<uint64_t>(minwidth);
			}
			else
			{
				// not enough characters, so go back to default sizing
				fwidth = 0;
			}

			if(use_right_pad && minwidth)
			{
				// if we're padding on the right, DON'T pad the floating part
				fwidth = 0;
			}

			// rescale the float value
			if(expval)
				value /= conv.F;

			// output the floating part

			auto args_copy = args;
			args_copy.width = static_cast<int64_t>(fwidth);
			auto len = static_cast<int64_t>(print_floating(cb, negative ? -value : value, args_copy));

			// output the exponent part
			if(minwidth > 0)
			{
				len++;
				if(args.specifier & 0x20)   cb('e');
				else                        cb('E');

				// output the exponent value
				char digits_buf[8] = { };
				size_t digits_len = 0;

				auto buf = print_decimal_integer(digits_buf, 8, static_cast<int64_t>(tt::_Absolute(expval)));
				digits_len = 8 - static_cast<size_t>(buf - digits_buf);

				len += digits_len + 1;
				cb(expval < 0 ? '-' : '+');

				// zero-pad to minwidth - 2
				if(auto tmp = (minwidth - 2) - static_cast<int>(digits_len); tmp > 0)
					len += tmp, cb('0', tmp);

				cb(buf, digits_len);

				// might need to right-pad spaces
				if(use_right_pad && args.width > len)
					cb(' ', args.width - len), len = args.width;
			}

			return static_cast<size_t>(len);
		}


		template <typename _CallbackFn>
		size_t print_floating(_CallbackFn& cb, double value, format_args args)
		{
			constexpr int DEFAULT_PRECISION = 6;
			constexpr size_t MAX_BUFFER_LEN = 128;
			constexpr long double EXPONENTIAL_CUTOFF = 1e15;

			char buf[MAX_BUFFER_LEN] = { 0 };

			size_t len = 0;

			int prec = (args.have_precision() ? static_cast<int>(args.precision) : DEFAULT_PRECISION);

			bool use_zero_pad   = args.zero_pad() && args.positive_width();
			bool use_left_pad   = !use_zero_pad && args.positive_width();
			bool use_right_pad  = !use_zero_pad && args.negative_width();

			// powers of 10
			constexpr double pow10[] = {
				1,
				10,
				100,
				1000,
				10000,
				100000,
				1000000,
				10000000,
				100000000,
				1000000000,
				10000000000,
				100000000000,
				1000000000000,
				10000000000000,
				100000000000000,
				1000000000000000,
				10000000000000000,
			};

			// test for special values
			if((value != value) || (value > DBL_MAX) || (value < -DBL_MAX))
				return print_special_floating(cb, value, static_cast<format_args&&>(args));

			// switch to exponential for large values.
			if((value > EXPONENTIAL_CUTOFF) || (value < -EXPONENTIAL_CUTOFF))
				return print_exponent(cb, value, static_cast<format_args&&>(args));

			// default to g.
			if(args.specifier == -1)
				args.specifier = 'g';

			// test for negative
			const bool negative = (value < 0);
			if(value < 0)
				value = -value;

			// limit precision to 16, cause a prec >= 17 can lead to overflow errors
			while((len < MAX_BUFFER_LEN) && (prec > 16))
			{
				buf[len++] = '0';
				prec--;
			}

			auto whole = static_cast<int64_t>(value);
			auto tmp = (value - static_cast<double>(whole)) * pow10[prec];
			auto frac = static_cast<unsigned long>(tmp);

			double diff = tmp - static_cast<double>(frac);

			if(diff > 0.5)
			{
				frac += 1;

				// handle rollover, e.g. case 0.99 with prec 1 is 1.0
				if(frac >= static_cast<unsigned long>(pow10[prec]))
				{
					frac = 0;
					whole += 1;
				}
			}
			else if(diff < 0.5)
			{
				// ?
			}
			else if((frac == 0U) || (frac & 1U))
			{
				// if halfway, round up if odd OR if last digit is 0
				frac += 1;
			}

			if(prec == 0U)
			{
				diff = value - static_cast<double>(whole);
				if((!(diff < 0.5) || (diff > 0.5)) && (whole & 1))
				{
					// exactly 0.5 and ODD, then round up
					// 1.5 -> 2, but 2.5 -> 2
					whole += 1;
				}
			}
			else
			{
				auto count = prec;

				bool flag = (args.specifier == 'g' || args.specifier == 'G');
				// now do fractional part, as an unsigned number
				while(len < MAX_BUFFER_LEN)
				{
					if(flag && (frac % 10) == 0)
						goto skip;

					flag = false;
					buf[len++] = static_cast<char>('0' + (frac % 10));

				skip:
					count -= 1;
					if(!(frac /= 10))
						break;
				}

				// add extra 0s
				while((len < MAX_BUFFER_LEN) && (count-- > 0))
					buf[len++] = '0';

				// add decimal
				if(len < MAX_BUFFER_LEN)
					buf[len++] = '.';
			}

			// do whole part, number is reversed
			while(len < MAX_BUFFER_LEN)
			{
				buf[len++] = static_cast<char>('0' + (whole % 10));
				if(!(whole /= 10))
					break;
			}

			// pad leading zeros
			if(use_zero_pad)
			{
				auto width = args.width;

				if(args.have_width() != 0 && (negative || args.prepend_plus() || args.prepend_space()))
					width--;

				while((len < static_cast<size_t>(width)) && (len < MAX_BUFFER_LEN))
					buf[len++] = '0';
			}

			if(len < MAX_BUFFER_LEN)
			{
				if(negative)
					buf[len++] = '-';

				else if(args.prepend_plus())
					buf[len++] = '+'; // ignore the space if the '+' exists

				else if(args.prepend_space())
					buf[len++] = ' ';
			}

			// reverse it.
			for(size_t i = 0; i < len / 2; i++)
				tt::swap(buf[i], buf[len - i - 1]);

			auto padding_width = static_cast<size_t>(tt::_Maximum(int64_t(0), args.width - static_cast<int64_t>(len)));

			if(use_left_pad) cb(' ', padding_width);
			if(use_zero_pad) cb('0', padding_width);

			cb(buf, len);

			if(use_right_pad)
				cb(' ', padding_width);

			return len + ((use_left_pad || use_right_pad) ? padding_width : 0);
		}


		template <typename _Type>
		char* print_hex_integer(char* buf, size_t bufsz, _Type value)
		{
			static_assert(sizeof(_Type) <= 8);

		#if ZPR_HEXADECIMAL_LOOKUP_TABLE
			constexpr const char lookup_table[] =
				"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
				"202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f"
				"404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f"
				"606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f"
				"808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f"
				"a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
				"c0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
				"e0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";
		#endif

			constexpr auto hex_digit = [](int x) -> char {
				if(0 <= x && x <= 9)
					return static_cast<char>('0' + x);

				return static_cast<char>('a' + x - 10);
			};

			char* ptr = buf + bufsz;

			// if we have the lookup table, do two digits at a time.
		#if ZPR_HEXADECIMAL_LOOKUP_TABLE

			constexpr auto copy = [](char* dst, const char* src) {
				memcpy(dst, src, 2);
			};

			if constexpr (sizeof(_Type) > 1)
			{
				while(value >= 0x100)
				{
					copy((ptr -= 2), &lookup_table[(value & 0xFF) * 2]);
					value /= static_cast<_Type>(0x100);
				}
			}

			if(value < 0x10)
				*(--ptr) = hex_digit(static_cast<int>(value));

			else
				copy((ptr -= 2), &lookup_table[value * 2]);

		#else

			do {
				*(--ptr) = hex_digit(value & 0xF);
				value /= 0x10;

			} while(value > 0);

		#endif

			return ptr;
		}

		template <typename _Type>
		char* print_binary_integer(char* buf, size_t bufsz, _Type value)
		{
			char* ptr = buf + bufsz;

			do {
				*(--ptr) = ('0' + (value & 1));
				value >>= 1;

			} while(value > 0);

			return ptr;
		}

		template <typename _Type>
		char* print_decimal_integer(char* buf, size_t bufsz, _Type value)
		{
			static_assert(sizeof(_Type) <= 8);

		#if ZPR_DECIMAL_LOOKUP_TABLE
			constexpr const char lookup_table[] =
				"000102030405060708091011121314151617181920212223242526272829"
				"303132333435363738394041424344454647484950515253545556575859"
				"606162636465666768697071727374757677787980818283848586878889"
				"90919293949596979899";
		#endif

			bool neg = false;
			if constexpr (tt::is_signed_v<_Type>)
			{
				neg = (value < 0);
				if(neg)
					value = -value;
			}

			char* ptr = buf + bufsz;

			// if we have the lookup table, do two digits at a time.
		#if ZPR_DECIMAL_LOOKUP_TABLE

			constexpr auto copy = [](char* dst, const char* src) {
				memcpy(dst, src, 2);
			};

			while(value >= 100)
			{
				copy((ptr -= 2), &lookup_table[(value % 100) * 2]);
				value /= 100;
			}

			if(value < 10)
				*(--ptr) = static_cast<char>(value + '0');

			else
				copy((ptr -= 2), &lookup_table[value * 2]);

		#else

			do {
				*(--ptr) = ('0' + (value % 10));
				value /= 10;

			} while(value > 0);

		#endif

			if(neg)
				*(--ptr) = '-';

			return ptr;
		}

		template <typename _Type>
		ZPR_ALWAYS_INLINE char* print_integer(char* buf, size_t bufsz, _Type value, int base)
		{
			if(base == 2)       return print_binary_integer(buf, bufsz, value);
			else if(base == 16) return print_hex_integer(buf, bufsz, value);
			else                return print_decimal_integer(buf, bufsz, value);
		}






		struct __print_state_t
		{
			const char* beg;
			const char* end;
			const char* fmtend;
		};

		/*
			Print a single value using the callback function and the given format arguments. This function
			should be used in user-defined print_formatters, so that inner values (if any) are correctly
			printed as well. Naturally it also reduces the amount of code that is required.

			Arguments:
			`cb`        -- the callback function; this should be the same `cb` that is passed to print_formatter::print()
			`fmt_args`  -- the format specifiers to use
			`value`     -- the value to print
		*/
		template <typename _CallbackFn, typename _Type>
		ZPR_ALWAYS_INLINE void print_one(_CallbackFn& cb, format_args fmt_args, _Type&& value)
		{
			using Decayed_T = tt::decay_t<_Type>;

			static_assert(has_formatter_v<_Type> || has_formatter_v<Decayed_T>,
				"no formatter for type");

			if constexpr (has_formatter<_Type>::value)
			{
				print_formatter<_Type>().print(static_cast<_Type&&>(value),
					cb, static_cast<format_args&&>(fmt_args));
			}
			else
			{
				print_formatter<Decayed_T>().print(static_cast<_Type&&>(value),
					cb, static_cast<format_args&&>(fmt_args));
			}
		}

		template <typename _PrintOne, typename _CallbackFn>
		ZPR_ALWAYS_INLINE void skip_fmts_impl(__print_state_t* pst, _CallbackFn& cb, _PrintOne&& one_printer)
		{
			bool printed = false;
			while(pst->end < pst->fmtend)
			{
				if(*pst->end == '{')
				{
					if(printed)
						break;

					auto tmp = pst->end;

					// flush whatever we have first:
					cb(pst->beg, pst->end);
					if(pst->end[1] == '{')
					{
						cb('{');
						pst->end += 2;
						pst->beg = pst->end;
						continue;
					}

					while(pst->end[0] && pst->end[0] != '}')
						pst->end++;

					// owo
					if(!pst->end[0])
						return;

					pst->end++;

					printed = true;
					auto fmt_spec = parse_fmt_spec(tt::str_view(tmp, static_cast<size_t>(pst->end - tmp)));

					one_printer(static_cast<format_args&&>(fmt_spec));

					pst->beg = pst->end;
				}
				else if(*pst->end == '}')
				{
					cb(pst->beg, pst->end + 1);

					// well... we don't need to escape }, but for consistency, we accept either } or }} to print one }.
					if(pst->end[1] == '}')
						pst->end++;

					pst->end++;
					pst->beg = pst->end;
				}
				else
				{
					pst->end++;
				}
			}

			if(not printed)
				one_printer(format_args{});
		}



		template <typename _CallbackFn, typename _Type, bool TypeErased = false>
		ZPR_ALWAYS_INLINE void skip_fmts(__print_state_t* pst, _CallbackFn& cb,
			tt::conditional_t<TypeErased, const void*, _Type&&> value)
		{
			skip_fmts_impl(pst, cb, [&](format_args fmt_args) {
				if constexpr (TypeErased)
				{
					if(value != nullptr)
						print_one(cb, fmt_args, *reinterpret_cast<const typename tt::remove_reference<_Type>::type*>(value));
				}
				else
				{
					print_one(cb, fmt_args, static_cast<_Type&&>(value));
				}
			});
		}


		/*
			Print to the specified callback function. This should be used in user-defined print_formatters if
			there is a need for variadic printing interface (versus print_one). This function should be
			preferred over `zpr::cprint` since it avoids an additional wrapper struct -- provided that the
			callback function used is exactly the one passed to print_formatter::print().

			Arguments:
			`cb`    -- the callback function; this should be the same `cb` passed to print_formatter::print().
			`sv`    -- the format string
			`args`  -- the values to print
		*/
		template <typename _CallbackFn, typename... _Types>
		ZPR_ALWAYS_INLINE void print(_CallbackFn& cb, tt::str_view sv, _Types&&... args)
		{
			__print_state_t st {};
			st.beg = sv.data();
			st.end = sv.data();
			st.fmtend = sv.end();

			(skip_fmts<_CallbackFn, _Types&&>(&st, cb, static_cast<_Types&&>(args)), ...);

			// flush
			cb(st.beg, static_cast<size_t>(st.fmtend - st.beg));
		}




		template <typename _CallbackFn, typename... _Types>
		ZPR_ALWAYS_INLINE void print_erased(_CallbackFn& cb, tt::str_view sv, const void* const* args, size_t num_args)
		{
			__print_state_t st {};
			st.beg = sv.data();
			st.end = sv.data();
			st.fmtend = sv.end();

			size_t idx = 0;
			(skip_fmts<_CallbackFn, _Types&&, true>(&st, cb, idx < num_args ? args[idx++] : nullptr), ...);

			// flush
			cb(st.beg, static_cast<size_t>(st.fmtend - st.beg));
		}


		template <typename _Type>
		ZPR_ALWAYS_INLINE size_t compute_required_length(_Type&& value, const format_args& fmt)
		{
			using Decayed_T = tt::decay_t<_Type>;
			static_assert(has_formatter_v<_Type> || has_formatter_v<Decayed_T>,
				"no formatter for type");

			if constexpr (has_required_size_calc_v<_Type>)
				return print_formatter<_Type>().length(static_cast<_Type&&>(value), fmt);

			else if constexpr (has_required_size_calc_v<Decayed_T>)
				return print_formatter<Decayed_T>().length(static_cast<_Type&&>(value), fmt);

			else
				return 1;
		}

		template <typename... _Types>
		ZPR_ALWAYS_INLINE size_t compute_required_lengths(const _Types&... values)
		{
			format_args fmt {};
			return (0 + ... + compute_required_length(values, fmt));
		}


	#if ZPR_USE_STD
		template <typename _Type, typename = void>
		struct has_resize_default_init
		{
			ZPR_ALWAYS_INLINE constexpr static void resize_default_init(std::string& s, size_t n)
			{
				s.resize(n);
			}
		};

		template <typename _Type>
		struct has_resize_default_init<_Type, tt::void_t<decltype(tt::declval<_Type>().__resize_default_init(1))>>
		{
			ZPR_ALWAYS_INLINE constexpr static void resize_default_init(_Type& s, size_t n)
			{
				s.__resize_default_init(n);
			}
		};

		ZPR_ALWAYS_INLINE void resize_string_cheaply(std::string& str, size_t required_size)
		{
			has_resize_default_init<std::string>::resize_default_init(str, required_size);
		}
	#endif



	#if ZPR_USE_STD
		struct string_appender
		{
			string_appender(std::string& buf) : m_ptr(buf.data()), m_buf(buf) { }
			~string_appender()
			{
				m_buf.resize(static_cast<size_t>(m_ptr - m_buf.data()));
			}

			ZPR_ALWAYS_INLINE void operator() (char c)
			{
				if(ZPR_UNLIKELY(get_remaining() < 1))
					this->reserve(m_buf.size() + 1);

				*m_ptr++ = c;
			}

			ZPR_ALWAYS_INLINE void operator() (tt::str_view sv)
			{
				if(ZPR_UNLIKELY(get_remaining() < sv.size()))
					this->reserve(m_buf.size() + sv.size());

				memmove(m_ptr, sv.data(), sv.size());
				m_ptr += sv.size();
			}

			ZPR_ALWAYS_INLINE void operator() (char c, size_t n)
			{
				if(ZPR_UNLIKELY(get_remaining() < n))
					this->reserve(m_buf.size() + n);

				memset(m_ptr, c, n);
				m_ptr += n;
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, const char* end)
			{
				auto n = static_cast<size_t>(end - begin);
				if(ZPR_UNLIKELY(get_remaining() < n))
					this->reserve(m_buf.size() + n);

				memmove(m_ptr, begin, n);
				m_ptr += n;
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, size_t len)
			{
				if(ZPR_UNLIKELY(get_remaining() < len))
					this->reserve(m_buf.size() + len);

				memmove(m_ptr, begin, len);
				m_ptr += len;
			}

			ZPR_ALWAYS_INLINE void reserve(size_t amount)
			{
				auto old_size = m_buf.size() - get_remaining();

				resize_string_cheaply(m_buf, amount);
				m_ptr = m_buf.data() + old_size;
			}

			string_appender(string_appender&&) = delete;
			string_appender(const string_appender&) = delete;
			string_appender& operator= (string_appender&&) = delete;
			string_appender& operator= (const string_appender&) = delete;

		private:
			char* m_ptr = 0;
			std::string& m_buf;

			ZPR_ALWAYS_INLINE size_t get_remaining()
			{
				return static_cast<size_t>(m_buf.data() + m_buf.size() - m_ptr);
			}
		};
	#endif

	#if !ZPR_FREESTANDING
		template <size_t Limit, bool Newline>
		struct file_appender
		{
			file_appender(FILE* fd, size_t& written) : fd(fd), written(written) { buf[0] = '\n'; }
			~file_appender() { flush(true); }

			file_appender(file_appender&&) = delete;
			file_appender(const file_appender&) = delete;
			file_appender& operator= (file_appender&&) = delete;
			file_appender& operator= (const file_appender&) = delete;

			ZPR_ALWAYS_INLINE void operator() (char c) { *ptr++ = c; flush(); }

			ZPR_ALWAYS_INLINE void operator() (tt::str_view sv) { (*this)(sv.data(), sv.size()); }
			ZPR_ALWAYS_INLINE void operator() (const char* begin, const char* end) { (*this)(begin, static_cast<size_t>(end - begin)); }

			ZPR_ALWAYS_INLINE void operator() (char c, size_t n)
			{
				while(n > 0)
				{
					auto x = tt::_Minimum(n, remaining());
					memset(ptr, c, x);
					ptr += x;
					n -= x;
					flush();
				}
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, size_t len)
			{
				while(len > 0)
				{
					auto x = tt::_Minimum(len, remaining());
					memcpy(ptr, begin, x);
					ptr += x;
					begin += x;
					len -= x;

					flush();
				}
			}

			ZPR_ALWAYS_INLINE void reserve(size_t) {}

		private:
			inline size_t remaining()
			{
				return Limit - static_cast<size_t>(ptr - buf);
			}

			inline void flush(bool last = false)
			{
				if(!last && static_cast<size_t>(ptr - buf) < Limit)
				{
					if constexpr (Newline)
						this->buf[ptr - buf] = '\n';

					return;
				}

				if(!last || !Newline)
				{
					fwrite(buf, sizeof(char), static_cast<size_t>(ptr - buf), fd);
					written += static_cast<size_t>(ptr - buf);

					ptr = buf;
				}
				else if(last && Newline)
				{
					// here's a special trick -- write one extra, because we always ensure that
					// "one-past" the last character in our buffer is a newline.
					fwrite(buf, sizeof(char), static_cast<size_t>(ptr - buf + 1), fd);
					written += static_cast<size_t>(ptr - buf) + 1;

					ptr = buf;
				}
			}

			FILE* fd = 0;

			char buf[Limit + 1];
			char* ptr = &buf[0];
			size_t& written;
		};
	#endif

		template <typename _Fn>
		struct callback_appender
		{
			static_assert(tt::is_invocable<_Fn, const char*, size_t>::value,
				"incompatible callback passed (signature must be compatible with [const char*, size_t])");


			callback_appender(_Fn* callback, bool newline) : len(0), newline(newline), callback(callback) { }
			~callback_appender() { if(newline) { (*callback)("\n", 1); } }

			ZPR_ALWAYS_INLINE void reserve(size_t) {}

			ZPR_ALWAYS_INLINE void operator() (char c)
			{
				(*callback)(&c, 1);
				this->len += 1;
			}

			ZPR_ALWAYS_INLINE void operator() (tt::str_view sv)
			{
				(*callback)(sv.data(), sv.size());
				this->len += sv.size();
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, const char* end)
			{
				(*callback)(begin, end - begin);
				this->len += (end - begin);
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, size_t len) { (*callback)(begin, len); this->len += len; }
			ZPR_ALWAYS_INLINE void operator() (char c, size_t n)
			{
				for(size_t i = 0; i < n; i++)
					(*callback)(&c, 1);

				this->len += n;
			}

			callback_appender(callback_appender&&) = delete;
			callback_appender(const callback_appender&) = delete;
			callback_appender& operator= (callback_appender&&) = delete;
			callback_appender& operator= (const callback_appender&) = delete;

			ZPR_ALWAYS_INLINE size_t size() { return this->len; }

		private:
			size_t len;
			bool newline;
			_Fn* callback;
		};

		struct buffer_appender
		{
			buffer_appender(char* buf, size_t cap) : buf(buf), cap(cap), len(0) { }

			ZPR_ALWAYS_INLINE void reserve(size_t) {}

			ZPR_ALWAYS_INLINE void operator() (char c)
			{
				if(this->len < this->cap)
					this->buf[this->len++] = c;
			}

			ZPR_ALWAYS_INLINE void operator() (tt::str_view sv)
			{
				auto l = this->remaining(sv.size());
				memmove(&this->buf[this->len], sv.data(), l);
				this->len += l;
			}

			ZPR_ALWAYS_INLINE void operator() (char c, size_t n)
			{
				for(size_t i = 0; i < this->remaining(n); i++)
					this->buf[this->len++] = c;
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, const char* end)
			{
				auto l = this->remaining(static_cast<size_t>(end - begin));
				memmove(&this->buf[this->len], begin, l);
				this->len += l;
			}

			ZPR_ALWAYS_INLINE void operator() (const char* begin, size_t len)
			{
				auto l = this->remaining(len);
				memmove(&this->buf[this->len], begin, l);
				this->len += l;
			}

			buffer_appender(buffer_appender&&) = delete;
			buffer_appender(const buffer_appender&) = delete;
			buffer_appender& operator= (buffer_appender&&) = delete;
			buffer_appender& operator= (const buffer_appender&) = delete;

			ZPR_ALWAYS_INLINE size_t size() { return this->len; }

		private:
			ZPR_ALWAYS_INLINE size_t remaining(size_t n) { return tt::_Minimum(this->cap - this->len, n); }

			char* buf = 0;
			size_t cap = 0;
			size_t len = 0;
		};

		constexpr size_t STDIO_BUFFER_SIZE = 4096;
	}



	/*
		Print with a user-specified callback function.

		The callback function should have this signature, or something compatible:
		void callback(const char* str, size_t len)

		Arguments:
		`callback`  -- the callback function, with an appropriate signature as stated above
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename _CallbackFn, typename... _Types>
	size_t cprint(_CallbackFn&& callback, tt::str_view fmt, _Types&&... args)
	{
		size_t n = 0;
		{
			auto appender = detail::callback_appender(&callback, /* newline: */ false);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
			n = appender.size();
		}
		return n;
	}

	/*
		Print with a user-specified callback function, appending a newline at the end.

		The callback function should have this signature, or something compatible:
		void callback(const char* str, size_t len)

		Arguments:
		`callback`  -- the callback function, with an appropriate signature as stated above
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename _CallbackFn, typename... _Types>
	size_t cprintln(_CallbackFn&& callback, tt::str_view fmt, _Types&&... args)
	{
		size_t n = 0;
		{
			auto appender = detail::callback_appender(&callback, /* newline: */ true);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
			n = appender.size();
		}
		return n;
	}

	/*
		Print to a user-specified buffer `buf`, with size `len`. A NULL-terminator is *NOT* inserted.
		If the number of bytes to print exceeds the size of the buffer, the output is truncated. Again,
		there is *NO* NULL-terminator, regardless of whether the buffer is filled.

		Arguments:
		`len`       -- the capacity of the buffer pointed to by `buf`
		`buf`       -- the buffer into which characters should be printed
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename... _Types>
	size_t sprint(size_t len, char* buf, tt::str_view fmt, _Types&&... args)
	{
		size_t n = 0;
		{
			auto appender = detail::buffer_appender(buf, len);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
			n = appender.size();
		}
		return n;
	}

#if ZPR_USE_STD
	/*
		Prints to a std::string.

		Arguments:
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the std::string.
	*/
	template <typename... _Types>
	std::string sprint(tt::str_view fmt, _Types&&... args)
	{
		std::string buf {};
		{
			auto appender = detail::string_appender(buf);
			appender.reserve(fmt.size() + detail::compute_required_lengths(args...));
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
		}
		return buf;
	}
#endif


	/*
		Forward the provided format-string and arguments to another zpr printing function. The implementation
		of this mechanism creates and stores pointers to the argument values, so you should *NOT* store the
		return value of a call to zpr::fwd() (especially if the arguments contain rvalues) -- you should only
		pass it directly as an argument to a zpr::print function.

		zpr::fwd() calls can be nested, ie. you can pass one zpr::fwd() to another zpr::fwd().

		Example usage:
		zpr::fprintln(stderr, "foo: {}", zpr::fwd("this is: {}", 69));

		In this case, no additional allocation is incurred in creating a temporary buffer to store the result of
		the inner print, and everything goes straight to the stderr FILE.

		Arguments:
		`fmt`       -- the format string
		`args`      -- the values to print

		Again, do not store the return value of this function.
	*/
	template <typename... _Types>
	auto fwd(tt::str_view fmt, _Types&&... args)
	{
		return detail::__forward_helper<_Types&&...>(static_cast<tt::str_view&&>(fmt), static_cast<_Types&&>(args)...);
	}



// if we are freestanding, we don't have stdio, so we can't print to "stdout".
#if !ZPR_FREESTANDING

	/*
		Print to the standard output stream.

		Arguments:
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename... _Types>
	size_t print(tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, false>(stdout, ret);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
		}
		return ret;
	}

	/*
		Print to the standard output stream, appending a newline at the end.

		Arguments:
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename... _Types>
	size_t println(tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, true>(stdout, ret);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
		}
		return ret;
	}

	/*
		Prints to the specified FILE stream.

		Arguments:
		`file`      -- the stream to print to
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename... _Types>
	size_t fprint(FILE* file, tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, false>(file, ret);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
		}
		return ret;
	}

	/*
		Prints to the specified FILE stream, appending a newline at the end.

		Arguments:
		`file`      -- the stream to print to
		`fmt`       -- the format string
		`args`      -- the values to print

		Returns the number of bytes printed.
	*/
	template <typename... _Types>
	size_t fprintln(FILE* file, tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, true>(file, ret);
			detail::print(appender, fmt, static_cast<_Types&&>(args)...);
		}
		return ret;
	}
#endif


	/*
		Create a special wrapper struct that will print its argument with the given width specifier.
		Example: `zpr::println("foo: {}", zpr::w(10)(69))` will print '69' with a width of 10.

		It is safe to store the value of `zpr::w(10)`, but it is *NOT* safe to store the value of
		`zpr::w(10)(value)`, especially if `value` is an rvalue.

		Arguments:
		`width`     -- the width specifier

		Returns a wrapper struct that accepts the value to print.
	*/
	ZPR_ALWAYS_INLINE detail::__fmtarg_w_helper w(int width)
	{
		return detail::__fmtarg_w_helper(width);
	}

	/*
		Create a special wrapper struct that will print its argument with the given precision specifier.
		Example: `zpr::println("foo: {}", zpr::p(10)(6.9))` will print '6.9' with a precision of 10.

		It is safe to store the value of `zpr::p(10)`, but it is *NOT* safe to store the value of
		`zpr::p(10)(value)`, especially if `value` is an rvalue.

		Arguments:
		`prec`      -- the precision specifier

		Returns a wrapper struct that accepts the value to print.
	*/
	ZPR_ALWAYS_INLINE detail::__fmtarg_p_helper p(int prec)
	{
		return detail::__fmtarg_p_helper(prec);
	}

	/*
		Create a special wrapper struct that will print its argument with the given width and precision
		specifiers. Example: `zpr::println("foo: {}", zpr::wp(10, 5)(6.9))` will print '6.9' with a
		width of 10 and a precision of 5.

		It is safe to store the value of `zpr::wp(10, 5)`, but it is *NOT* safe to store the value of
		`zpr::wp(10, 5)(value)`, especially if `value` is an rvalue.

		Arguments:
		`width`     -- the width specifier
		`prec`      -- the precision specifier

		Returns a wrapper struct that accepts the value to print.
	*/
	ZPR_ALWAYS_INLINE detail::__fmtarg_wp_helper wp(int width, int prec)
	{
		return detail::__fmtarg_wp_helper(width, prec);
	}


	namespace detail
	{
		template <typename _Type>
		constexpr auto is_stringlike_helper(_Type&& t) -> decltype(tt::conjunction<
			tt::disjunction<
				tt::is_same<char*, tt::decay_t<decltype(tt::declval<_Type>().data())>>,
				tt::is_same<const char*, tt::decay_t<decltype(tt::declval<_Type>().data())>>
			>, tt::is_integral<tt::decay_t<decltype(tt::declval<_Type>().size())>>>::value
		);

		constexpr auto is_stringlike_helper(...) -> tt::__invalid;

		template <typename _Type>
		struct is_stringlike
		{
			static constexpr bool value = !tt::is_same_v<tt::__invalid, decltype(is_stringlike_helper(tt::declval<_Type>()))>;
		};

	#if ZPR_USE_STD
		template <>
		struct is_stringlike<std::string> : tt::true_type {};
	#endif

		template <typename _Type, bool _HasFormatArgs>
		struct __extracted_return_type_wrapper
		{
			static constexpr bool has_format_args = _HasFormatArgs;
			using type = _Type;
		};

		template <typename _Func, typename _Type>
		constexpr auto __extract_return_type(_Func&& fn, _Type&& x)
			-> __extracted_return_type_wrapper<decltype(tt::declval<_Func>()(tt::declval<_Type>())), false>;

		template <typename _Func, typename _Type>
		constexpr auto __extract_return_type(_Func&& fn, _Type&& x)
			-> __extracted_return_type_wrapper<decltype(tt::declval<_Func>()(tt::declval<_Type>(), format_args{})), true>;


		template <typename _Func, typename _Type>
		struct __with_helper_ret
		{
			using return_type = decltype(__extract_return_type(tt::declval<_Func>(), tt::declval<_Type>()));

			static constexpr bool has_format_args = return_type::has_format_args;
			static constexpr bool returns_value = tt::is_same_v<typename return_type::type, const char*>
				|| tt::is_same_v<typename return_type::type, char*>
				|| is_stringlike<typename return_type::type>::value;

			static_assert(returns_value, "'zpr::with' can only be used with callbacks returning a string-like value");

			template <typename __Func, typename __Type>
			__with_helper_ret(__Func&& printer, __Type&& value) :
				printer(static_cast<__Func&&>(printer)), value(static_cast<__Type&&>(value)) { }

			_Func&& printer;
			_Type&& value;
		};
	}

	template <typename _Func, typename _Type>
	struct print_formatter<detail::__with_helper_ret<_Func, _Type>>
	{
		template <typename _Helper, typename _Cb>
		ZPR_ALWAYS_INLINE void print(_Helper&& uwu, _Cb&& cb, format_args args)
		{
			if constexpr (_Helper::has_format_args)
				print_one(cb, {}, uwu.printer(static_cast<_Type&&>(uwu.value), static_cast<format_args&&>(args)));
			else
				print_one(cb, static_cast<format_args&&>(args), uwu.printer(static_cast<_Type&&>(uwu.value)));
		}
	};



	/*
		Create a special wrapper struct that prints the first argument using the functor in the second. This
		function can be used when making a specialisation of `print_formatter` is unnecessary, or too troublesome.

		Example usage: `zpr::println("{}", zpr::with(69, [](auto&&) { return "uwu"; }))`
		This always prints '69'. clearly, a more complex lambda/function can be used.

		The provided functor should have one of these signatures:
		- `StringLike (Value)`                      -- should return the value as a string-like object
		- `StringLike (Value, format_args)`         -- like above, but also takes a `format_args`
		- `void (Callback, Value)`                  -- should print the value to the provided callback
		- `void (Callback, Value, format_args)`     -- like above, but also takes a `format_args`

		`StringLike` refers to a type that is either `char*` or `const char*`, OR meets BOTH the following criteria:
		1. has a `.data()` method that returns either `char*` or `const char*`
		2. has a `.size()` method that returns an integral type

		Thus, `std::string` and other stringlike-containers will work. Note that the requirements are less
		restrictive than the built-in string-printing formatter.

		`Callback` is a functor a signature of `void (const char*, size_t)` (though you should not rely on the
		return type). This variant can be useful to avoid any memory allocations; you can call it as many
		times as necessary in your functor.

		For the variants that *do not* take `format_args`, any provided format arguments (eg. `{.3f}`) are
		passed to the string printer (ie. the string that you return will be formatted accordingly).

		For the variants that *do* take it, the format args are passed directly to your functor, and the
		string itself is printed with default formatting.

		Arguments:
		`value`     -- the value to print
		`printer`   -- the functor to print with

		Returns a wrapper struct that
	*/
	template <typename _Func, typename _Type>
	ZPR_ALWAYS_INLINE auto with(_Type&& value, _Func&& printer) -> detail::__with_helper_ret<_Func, _Type>
	{
		return detail::__with_helper_ret<_Func, _Type>(static_cast<_Func&&>(printer), static_cast<_Type&&>(value));
	}







	// formatters lie here.
	// because we use explicit template args for the vprint stuff, it can only come after we
	// specialise all the print_formatters for the builtin types.
	template <typename _Type>
	struct print_formatter<detail::__fmtarg_w<_Type>>
	{
		template <typename _Cb, typename _Func = detail::__fmtarg_w<_Type>>
		ZPR_ALWAYS_INLINE void print(_Func&& x, _Cb&& cb, format_args args)
		{
			args.set_width(x.width);
			detail::print_one(static_cast<_Cb&&>(cb), static_cast<format_args&&>(args), static_cast<_Type&&>(x.arg));
		}

		ZPR_ALWAYS_INLINE size_t length(const _Type& x, format_args args)
		{
			args.set_width(x.width);
			return compute_required_length(x.arg, args);
		}
	};

	template <typename _Type>
	struct print_formatter<detail::__fmtarg_p<_Type>>
	{
		template <typename _Cb, typename _Func = detail::__fmtarg_p<_Type>>
		ZPR_ALWAYS_INLINE void print(_Func&& x, _Cb&& cb, format_args args)
		{
			args.set_precision(x.prec);
			detail::print_one(static_cast<_Cb&&>(cb), static_cast<format_args&&>(args), static_cast<_Type&&>(x.arg));
		}

		ZPR_ALWAYS_INLINE size_t length(const _Type& x, format_args args)
		{
			args.set_precision(x.prec);
			return compute_required_length(x.arg, args);
		}
	};

	template <typename _Type>
	struct print_formatter<detail::__fmtarg_wp<_Type>>
	{
		template <typename _Cb, typename _Func = detail::__fmtarg_wp<_Type>>
		ZPR_ALWAYS_INLINE void print(_Func&& x, _Cb&& cb, format_args args)
		{
			args.set_width(x.width);
			args.set_precision(x.prec);
			detail::print_one(static_cast<_Cb&&>(cb), static_cast<format_args&&>(args), static_cast<_Type&&>(x.arg));
		}

		ZPR_ALWAYS_INLINE size_t length(const _Type& x, format_args args)
		{
			args.set_width(x.width);
			args.set_precision(x.prec);
			return compute_required_length(x.arg, args);
		}
	};

	template <typename... _Types>
	struct print_formatter<detail::__forward_helper<_Types...>>
	{
		template <typename _Cb, typename _Func = detail::__forward_helper<_Types...>>
		ZPR_ALWAYS_INLINE void print(_Func&& fwd, _Cb&& cb, format_args args)
		{
			(void) args;
			detail::print_erased<_Cb, _Types&&...>(static_cast<_Cb&&>(cb), fwd.fmt, &fwd.values[0], fwd.num_values);
		}
	};

	namespace detail
	{
		template <typename _Type>
		struct __int_formatter
		{
			using Decayed_T = tt::decay_t<_Type>;

			template <typename _Cb>
			void print(_Type x, _Cb&& cb, format_args args)
			{
				if(args.specifier == 'c')
				{
					// we can't access the print_formatter<char> specialisation here (because... declaration order)
					// so just print it as a string, which is what we do in the char formatter anyway.
					detail::print_string(static_cast<_Cb&&>(cb), reinterpret_cast<char*>(&x), 1,
						static_cast<format_args&&>(args));
					return;
				}

				int base = 10;
				if((args.specifier | 0x20) == 'x')  base = 16;
				else if(args.specifier == 'b')      base = 2;
				else if(args.specifier == 'p')
				{
					base = 16;
					args.specifier = 'x';
					args.flags |= FMT_FLAG_ALTERNATE;
				}

				// if we print base 2 we need 64 digits!
				constexpr size_t digits_buf_sz = 65;
				char digits_buf[digits_buf_sz] = { };

				char* digits = 0;
				size_t digits_len = 0;


				{
					if constexpr (tt::is_unsigned<Decayed_T>::value)
					{
						digits = detail::print_integer(digits_buf, digits_buf_sz, x, base);
						digits_len = digits_buf_sz - static_cast<size_t>(digits - digits_buf);
					}
					else
					{
						if(base == 16)
						{
							digits = detail::print_integer(digits_buf, digits_buf_sz,
								static_cast<tt::make_unsigned_t<Decayed_T>>(x), base);

							digits_len = digits_buf_sz - static_cast<size_t>(digits - digits_buf);
						}
						else
						{
							auto abs_val = tt::_Absolute(x);
							digits = detail::print_integer(digits_buf, digits_buf_sz, abs_val, base);

							digits_len = digits_buf_sz - static_cast<size_t>(digits - digits_buf);
						}
					}

					if('A' <= args.specifier && args.specifier <= 'Z')
						for(size_t i = 0; i < digits_len; i++)
							digits[i] = static_cast<char>(digits[i] - 0x20);
				}

				char prefix[4] = { 0 };
				int64_t prefix_len = 0;
				int64_t prefix_digits_length = 0;
				{
					char* pf = prefix;
					if(args.prepend_plus())
						prefix_len++, *pf++ = '+';

					else if(args.prepend_space())
						prefix_len++, *pf++ = ' ';

					else if(x < 0 && base == 10)
						prefix_len++, *pf++ = '-';

					if(base != 10 && args.alternate())
					{
						*pf++ = '0';
						*pf++ = (ZPR_HEX_0X_RESPECTS_UPPERCASE ? args.specifier : (args.specifier | 0x20));

						prefix_digits_length += 2;
						prefix_len += 2;
					}
				}

				int64_t output_length_with_precision = (args.have_precision()
					? tt::_Maximum(args.precision, static_cast<int64_t>(digits_len))
					: static_cast<int64_t>(digits_len)
				);

				int64_t total_digits_length = prefix_digits_length + static_cast<int64_t>(digits_len);
				int64_t normal_length = prefix_len + static_cast<int64_t>(digits_len);
				int64_t length_with_precision = prefix_len + output_length_with_precision;

				bool use_precision = args.have_precision();
				bool use_zero_pad  = args.zero_pad() && args.positive_width() && !use_precision;
				bool use_left_pad  = !use_zero_pad && args.positive_width();
				bool use_right_pad = !use_zero_pad && args.negative_width();

				int64_t padding_width = args.width - length_with_precision;
				int64_t zeropad_width = args.width - normal_length;
				int64_t precpad_width = args.precision - total_digits_length;

				if(padding_width <= 0) { use_left_pad = false; use_right_pad = false; }
				if(zeropad_width <= 0) { use_zero_pad = false; }
				if(precpad_width <= 0) { use_precision = false; }

				// pre-prefix
				if(use_left_pad) cb(' ', static_cast<size_t>(padding_width));

				cb(prefix, static_cast<size_t>(prefix_len));

				// post-prefix
				if(use_zero_pad) cb('0', static_cast<size_t>(zeropad_width));

				// prec-string
				if(use_precision) cb('0', static_cast<size_t>(precpad_width));

				cb(digits, static_cast<size_t>(digits_len));

				// postfix
				if(use_right_pad) cb(' ', static_cast<size_t>(padding_width));
			}
		};
	}

	template <> struct print_formatter<signed char> : detail::__int_formatter<signed char> { };
	template <> struct print_formatter<unsigned char> : detail::__int_formatter<unsigned char> { };
	template <> struct print_formatter<signed short> : detail::__int_formatter<signed short> { };
	template <> struct print_formatter<unsigned short> : detail::__int_formatter<unsigned short> { };
	template <> struct print_formatter<signed int> : detail::__int_formatter<signed int> { };
	template <> struct print_formatter<unsigned int> : detail::__int_formatter<unsigned int> { };
	template <> struct print_formatter<signed long> : detail::__int_formatter<signed long> { };
	template <> struct print_formatter<unsigned long> : detail::__int_formatter<unsigned long> { };
	template <> struct print_formatter<signed long long> : detail::__int_formatter<signed long long> { };
	template <> struct print_formatter<unsigned long long> : detail::__int_formatter<unsigned long long> { };

	template <typename _Type>
	struct print_formatter<_Type, typename tt::enable_if<(
		tt::is_enum_v<tt::decay_t<_Type>>
	)>::type>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(_Type x, _Cb&& cb, format_args args)
		{
			using underlying = tt::underlying_type_t<tt::decay_t<_Type>>;
			detail::print_one(static_cast<_Cb&&>(cb), static_cast<format_args&&>(args), static_cast<underlying>(x));
		}
	};

	template <>
	struct print_formatter<float>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(float x, _Cb&& cb, format_args args)
		{
			if(args.specifier == 'e' || args.specifier == 'E')
				print_exponent(static_cast<_Cb&&>(cb), x, static_cast<format_args&&>(args));

			else
				print_floating(static_cast<_Cb&&>(cb), x, static_cast<format_args&&>(args));
		}

		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(double x, _Cb&& cb, format_args args)
		{
			if(args.specifier == 'e' || args.specifier == 'E')
				print_exponent(static_cast<_Cb&&>(cb), x, static_cast<format_args&&>(args));

			else
				print_floating(static_cast<_Cb&&>(cb), x, static_cast<format_args&&>(args));
		}
	};

	template <size_t _Number>
	struct print_formatter<const char (&)[_Number]>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(const char (&x)[_Number], _Cb&& cb, format_args args)
		{
			detail::print_string(static_cast<_Cb&&>(cb), x, _Number - 1, static_cast<format_args&&>(args));
		}

		size_t length(...)
		{
			return _Number - 1;
		}
	};

	template <>
	struct print_formatter<const char*>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(const char* x, _Cb&& cb, format_args args)
		{
			detail::print_string(static_cast<_Cb&&>(cb), x, strlen(x), static_cast<format_args&&>(args));
		}
	};

	template <>
	struct print_formatter<char>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(char x, _Cb&& cb, format_args args)
		{
			if(args.specifier != -1 && args.specifier != 'c')
			{
				print_formatter<unsigned char>().print(static_cast<unsigned char>(x),
					static_cast<_Cb&&>(cb),
					static_cast<format_args&&>(args));
			}
			else
			{
				detail::print_string(static_cast<_Cb&&>(cb), &x, 1, static_cast<format_args&&>(args));
			}
		}

		size_t length(char x, format_args fmt)
		{
			return 1;
		}
	};

	template <>
	struct print_formatter<bool>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(bool x, _Cb&& cb, format_args args)
		{
			detail::print_string(static_cast<_Cb&&>(cb),
				x ? "true" : "false",
				x ? 4      : 5,
				static_cast<format_args&&>(args)
			);
		}

		size_t length(char x, format_args fmt)
		{
			return 5;
		}
	};

	template <>
	struct print_formatter<const void*>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(const void* x, _Cb&& cb, format_args args)
		{
			args.specifier = 'p';
			print_one(static_cast<_Cb&&>(cb), static_cast<format_args&&>(args), reinterpret_cast<uintptr_t>(x));
		}
	};

	template <typename _Type>
	struct print_formatter<_Type, typename tt::enable_if<(
		tt::conjunction<detail::is_iterable<_Type>, tt::is_same<tt::remove_cv_t<typename _Type::value_type>, char>>::value
	)>::type>
	{
		template <typename _Cb>
		ZPR_ALWAYS_INLINE void print(const _Type& x, _Cb&& cb, format_args args)
		{
			detail::print_string(static_cast<_Cb&&>(cb), x.data(), x.size(), static_cast<format_args&&>(args));
		}

		size_t length(const _Type& x, format_args fmt)
		{
			return x.size();
		}
	};

	// exclude strings and string_views
	template <typename _Type>
	struct print_formatter<_Type, typename tt::enable_if<(
		tt::conjunction<detail::is_iterable<_Type>,
			tt::negation<tt::is_same<typename _Type::value_type, char>>
		>::value
	)>::type>
	{
		template <typename _Cb>
		void print(const _Type& x, _Cb&& cb, format_args args)
		{
			if(begin(x) == end(x))
			{
				if(!args.alternate())
					cb("[ ]");
				return;
			}

			if(!args.alternate())
				cb("[");

			for(auto it = begin(x);;)
			{
				detail::print_one(static_cast<_Cb&&>(cb), args, *it);
				++it;

				if(it != end(x))
				{
					if(!args.alternate())
						cb(", ");
				}
				else
				{
					break;
				}
			}

			if(!args.alternate())
				cb("]");
		}
	};

	template <typename _Type, size_t _Number>
	struct print_formatter<_Type (&)[_Number]>
	{
		template <typename _Cb>
		void print(const _Type (&array)[_Number], _Cb&& cb, format_args args)
		{
			if(_Number == 0)
			{
				if(!args.alternate())
					cb("[ ]");
				return;
			}

			if(!args.alternate())
				cb("[");

			for(size_t i = 0; i < _Number; i++)
			{
				detail::print_one(static_cast<_Cb&&>(cb), args, array[i]);
				if(i + 1 != _Number)
				{
					if(!args.alternate())
						cb(", ");
				}
				else
				{
					break;
				}
			}

			if(!args.alternate())
				cb("]");
		}
	};




	template <>
	struct print_formatter<void*> : print_formatter<const void*> { };

	template <>
	struct print_formatter<char*> : print_formatter<const char*> { };

	template <>
	struct print_formatter<double> : print_formatter<float> { };

	template <size_t _Number>
	struct print_formatter<char (&)[_Number]> : print_formatter<const char (&)[_Number]> { };




#if ZPR_USE_STD
	template <typename _Type1, typename _Type2>
	struct print_formatter<std::pair<_Type1, _Type2>>
	{
		template <typename _Cb>
		void print(const std::pair<_Type1, _Type2>& x, _Cb&& cb, format_args args)
		{
			cb("{ ");
			detail::print_one(static_cast<_Cb&&>(cb), args, x.first);
			cb(", ");
			detail::print_one(static_cast<_Cb&&>(cb), args, x.second);
			cb(" }");
		}

		size_t length(const std::pair<_Type1, _Type2>& x, format_args fmt)
		{
			// 6 for the '{ ', ', ', and ' }'.
			return 6 + detail::compute_required_length(x.first, fmt) + detail::compute_required_length(x.second, fmt);
		}
	};
#endif




	// vprint stuff
	namespace detail
	{
		template <typename _CallbackFn>
		struct type_erased_arg
		{
			const void* data;
			void (*type_erased_printer)(const void*, _CallbackFn&, format_args);
		};

		template <typename _Type, typename _CallbackFn>
		type_erased_arg<_CallbackFn> erase_argument(_CallbackFn& callback, _Type&& arg)
		{
			using DecayedT = tt::decay_t<_Type>;

			type_erased_arg<_CallbackFn> erased {};
			erased.data = &arg;
			erased.type_erased_printer = [](const void* value, _CallbackFn& cb, format_args fa) {
				print_formatter<DecayedT>().print(*reinterpret_cast<const DecayedT*>(value), cb, fa);
			};

			return erased;
		}

		template <typename _CallbackFn>
		void vprint_impl(tt::str_view fmt, _CallbackFn& callback, const type_erased_arg<_CallbackFn>* args, size_t num_args)
		{
			__print_state_t st {};
			st.beg = fmt.data();
			st.end = fmt.data();
			st.fmtend = fmt.end();

			size_t i = 0;
			auto thingy = [&](format_args fmt_args) {
				args[i].type_erased_printer(args[i].data, callback, static_cast<format_args&&>(fmt_args));
			};

			for(; i < num_args; i++)
				skip_fmts_impl(&st, callback, thingy);

			// flush
			callback(st.beg, st.fmtend - st.beg);
		}
	}

	#define ERASE_ARGUMENT_LIST(name, num)                                          \
		size_t num = 0;                                                             \
		detail::type_erased_arg<decltype(appender)> name[sizeof...(_Types)] {};     \
		do { ((name[num++] = detail::erase_argument(appender, args)), ...); } while(0)


	template <typename _CallbackFn, typename... _Types>
	size_t vcprint(_CallbackFn&& callback, tt::str_view fmt, _Types&&... args)
	{
		size_t n = 0;
		{
			auto appender = detail::callback_appender(&callback, /* newline: */ false);

			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);

			n = appender.size();
		}
		return n;
	}

	template <typename _CallbackFn, typename... _Types>
	size_t vcprintln(_CallbackFn&& callback, tt::str_view fmt, _Types&&... args)
	{
		size_t n = 0;
		{
			auto appender = detail::callback_appender(&callback, /* newline: */ true);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);

			n = appender.size();
		}
		return n;
	}

	template <typename... _Types>
	size_t vsprint(size_t len, char* buf, tt::str_view fmt, _Types&&... args)
	{
		size_t n = 0;
		{
			auto appender = detail::buffer_appender(buf, len);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);

			n = appender.size();
		}
		return n;
	}




#if !ZPR_FREESTANDING
	template <typename... _Types>
	size_t vprint(tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, /* newline: */ false>(stdout, ret);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);
		}
		return ret;
	}

	template <typename... _Types>
	size_t vprintln(tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, /* newline: */ true>(stdout, ret);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);
		}
		return ret;
	}

	template <typename... _Types>
	size_t vfprint(FILE* file, tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, /* newline: */ false>(file, ret);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);
		}
		return ret;
	}

	template <typename... _Types>
	size_t vfprintln(FILE* file, tt::str_view fmt, _Types&&... args)
	{
		size_t ret = 0;
		{
			auto appender = detail::file_appender<detail::STDIO_BUFFER_SIZE, /* newline: */ true>(file, ret);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);
		}
		return ret;
	}
#endif

#if ZPR_USE_STD
	template <typename... _Types>
	std::string vsprint(tt::str_view fmt, _Types&&... args)
	{
		std::string ret {};
		{
			auto appender = detail::string_appender(ret);
			ERASE_ARGUMENT_LIST(erased_args, num);
			detail::vprint_impl(static_cast<tt::str_view&&>(fmt), appender, &erased_args[0], num);
		}
		return ret;
	}
#endif

	#undef ERASE_ARGUMENT_LIST
}






/*

	Version History
	===============

	2.7.3 - 10/08/2022
	------------------
	- Fix '}}' not actually printing '}'



	2.7.2 - 10/08/2022
	------------------
	- Fix a bunch of implicit sign conversion warnings



	2.7.1 - 10/08/2022
	------------------
	- Fix a memory bug with string_appender due to accounting errors



	2.7.0 - 30/04/2022
	------------------
	- Add vprint API, which are functions prefixed with `v`. This API path tries to reduce the number of template
		instantiations produced by type-erasing the arguments as far as possible in order to prevent code bloat.



	2.6.0 - 29/04/2022
	------------------
	- Add `zpr::with` that lets you print a value with a lambda/function directly, instead of having to specialise a
		`print_formatter`.

	- Remove our type traits implementation since <type_traits> is a freestanding header that should always be available.
	- Add a new `length()` member to print_formatter to allow `sprint()` to `std::string` to pre-reserve the string buffer
		so we pay less for constant append/resize.
		(this method is optional; if you don't implement it, then you don't get the speedup, but it's not any slower).



	2.5.7 - 26/11/2021
	------------------
	Bug fixes:
	- fix mishandling of negative widths (ie. left-align/right-pad) when using `zpr::w` or `zpr::wp`.



	2.5.6 - 18/10/2021
	------------------
	Bug fixes:
	- increase protection against macros with better naming



	2.5.5 - 18/10/2021
	------------------
	Bug fixes:
	- increase protection against macros with more underscores



	2.5.4 - 18/10/2021
	------------------
	Add improved error message (static_assert) if an incompatible callback function is passed to `cprint`.
	Bug fixes:
	- add more protection against havoc-wrecking macros (abs, B1...) by adding underscores



	2.5.3 - 15/09/2021
	------------------
	Add additional methods to `tt::str_view`, and fix broken operator== on it



	2.5.2 - 30/08/2021
	------------------
	Improve safety of zpr::fwd calls by adding range checking to the type-erased value array.



	2.5.1 - 27/08/2021
	------------------
	Fix implicit integer casting warnings



	2.5.0 - 07/08/2021
	------------------
	Add a printer for T[N] (ie. arrays of arbitrary type)



	2.4.6 - 04/07/2021
	------------------
	Bug fixes:
	- fix a warning on MSVC when printing unsigned char as hex


	2.4.5 - 27/05/2021
	------------------
	Bug fixes:
	- fix a regression in file_appender introduced in 2.2.1, where empty println("")s were
	  printing garbage



	2.4.4 - 26/05/2021
	------------------
	Bug fixes:
	- fix const correctness when using zpr::fwd()



	2.4.3 - 25/05/2021
	------------------
	Bug fixes:
	- fix ignored specifier when printing a `char`



	2.4.2 - 25/05/2021
	------------------
	Bug fixes:
	- fix broken std::string sprint()



	2.4.1 - 25/05/2021
	------------------
	Improve documentation



	2.4.0 - 25/05/2021
	------------------
	Add `zpr::fwd`, which acts like zpr::sprint(), but it's meant to be passed to another, "outer" call
	to another printer; for example: `zpr::fprintln(stderr, "hello, {}", zpr::fwd("asdf {}", 69))`. This
	avoids a round-trip and a string allocation (if you used `sprint`), and everything goes straight to
	the FILE* in this case.

	It is not safe to store the return value of `zpr::fwd`, especially if its arguments contain rvalues.



	2.3.1 - 01/05/2021
	------------------
	Bug fixes:
	- fix a bug where the #-printing of iterables only printed the first element



	2.3.0 - 01/05/2021
	------------------
	Bug fixes:
	- change the signature of zpr::sprint(char* buf, size_t sz, str_view fmt, ...);
	  new signature: to be zpr::sprint(size_t sz, char* buf, str_view fmt, ...)

	  this is necessary because msvc cannot differentiate between sprint(char*, size_t, str_view, ...)
	  and sprint(str_view, ...), and would complain about an ambiguous overload.

	  This is a potentially breaking change, hence the minor version bump.




	2.2.1 - 01/05/2021
	------------------
	Improve the `file_appender` so that newlines are written together with the last part of the buffer in
	a single call to fwrite() -- this hopefully mitigates lines being broken up right at the newline when
	printing in multithreaded scenarios.



	2.2.0 - 27/04/2021
	------------------
	Add 'alternate' flag for the iterable printers; if true, then the opening and closing brackets ('[' and ']') and
	the commas (',') between items are *not* printed. As a reminder, use '{#}' to specify alternate printing mode.



	2.1.13 - 23/04/2021
	-------------------
	Bug fixes:
	- fix implicit conversion warning on MSVC in number printing



	2.1.12 - 15/03/2021
	-------------------
	Bug fixes:
	- fix handling of control macros when they are defined without a value. Now they will not generate errors, and are
		treated as TRUE. Also update the documentation about this.



	2.1.11 - 15/03/2021
	-------------------
	Bug fixes:
	- fix broken drop() and take_last() for str_view... again



	2.1.10 - 02/01/2021
	-------------------
	Bug fixes:
	- fix truncated `inf` and `nan` when precision is specified.



	2.1.9 - 23/12/2020
	------------------
	Bug fixes:
	- fix unused variable warning when lookup tables were disabled
	- fix pointless assertion in integer printing (`sizeof(T) <= 64` -> `sizeof(T) <= 8`)



	2.1.8 - 14/12/2020
	------------------
	Bug fixes:
	- fix incorrect drop(), drop_last(), take(), and take_last() on tt::str_view



	2.1.7 - 21/11/2020
	------------------
	Bug fixes:
	- fix warning on member initialisation order for cprint



	2.1.6 - 20/11/2020
	------------------
	Bug fixes:
	- fix broken std::pair printing



	2.1.5 - 17/11/2020
	------------------
	Bug fixes:
	- fix broken binary integer printing



	2.1.4 - 13/11/2020
	------------------
	Bug fixes:
	- fix 'comparison between integers of different signs' (or whatever) warning



	2.1.3 - 04/10/2020
	------------------
	Bug fixes:
	- fix an issue preventing user-defined formatters from calling cprint() with the callback given to them



	2.1.2 - 01/10/2020
	------------------
	Bug fixes:
	- fix bool and char not respecting width specifier
	- fix warnings on -Wall -Wextra
	- revert back to memcpy due to alignment concerns



	2.1.1 - 29/09/2020
	------------------
	Remove tt::forward and tt::move to cut down on templates (they're just glorified static_cast<T&&>s anyway)

	Bug fixes:
	- fix potential template failure for signed and unsigned char



	2.1.0 - 28/09/2020
	------------------
	Remove redundant overloads of all the print functions taking const char (&)[N], since tt:str_view
	has an implicit constructor for that.

	Bug fixes:
	- fix the implicit constructor for str_view taking const char (&)[N]



	2.0.2 - 28/09/2020
	------------------
	Small performance improvements. Reduced enable_if usage in favour of template specialisation to improve
	compile times.



	2.0.1 - 28/09/2020
	------------------
	Add `ln` versions of the cprint() functions.



	2.0.0 - 27/09/2020
	------------------
	Completely rewrite the internals; now no longer tuple-based, and we're back to template packs. However,
	instead of using recursion, we are now expanding with fold expressions. This comes with one drawback, however;
	we now do not support in-stream width and precision specifiers (eg. {*.s}). instead, we expose 3 new functions,
	w(), p(), and wp(), which do what their names would suggest; see the documentation above for more info.

	Major version bump due to breaking api change.



	1.6.0 - 26/09/2020
	------------------
	Add the ZPR_FREESTANDING define; see documentation above for details. Also add cprint() and family, which
	allow using a callback of the form `void (*)(char*, size_t)` -- or a compatible template function object,
	to print.

	Bug fixes:
	- fix printing integer zeroes.
	- fix printing hex digits.



	1.5.0 - 10/09/2020
	------------------
	Change the print_formatter interface subtly. Now, you can choose to specialise the template for both the decayed
	type (eg. int, const char*), or for the un-decayed type (eg. const int&, const char (&)[N]). The compiler will
	choose the appropriate specialisation, preferring the latter (not-decayed) type, if it exists.

	Bug fixes:
	- fix issues where we would write NULL bytes into the output stream, due to the 'char(&)[N]' change.
	- fix right-padding and zero-padding behaviour



	1.4.0 - 10/09/2020
	------------------
	Completely remove dependency on STL types. sadly this includes lots of re-implemented type_traits, but an okay
	cost to pay, I suppose. Introduces ZPR_USE_STD define to control this.

	Bug fixes: lots of fixes on formatting correctness; we should now be correct wrt. printf.



	1.3.1 - 09/09/2020
	------------------
	Remove dependency on std::to_chars, to slowly wean off STL. Introduce two new #defines:
	ZPR_DECIMAL_LOOKUP_TABLE, and ZPR_HEXADECIMAL_LOOKUP_TABLE. see docs for info.



	1.3.0 - 08/09/2020
	------------------
	Add overloads for the user-facing print functions that accept std::string_view as the format string. This also
	works for std::string since string_view has an implicit conversion constructor for it.

	Removed user-facing overloads of print and friends that take 'const char*'; now they either take std::string_view
	(as above), or (const char&)[].

	Change the behaviour of string printers; now, any iterable type with a value_type typedef equal to 'char'
	(exactly char -- not signed char, not unsigned char) will print as a string. This lets us cover custom
	string types as well. A side effect of this is that std::vector<char> will print as a string, which might
	be unexpected.

	Bug fixes:
	- broken formatting for floating point numbers
	- '{}' now uses '%g' format for floating point numbers
	- '{p}' (ie. '%p') now works, including '{}' for void* and const void*.



	1.2.0 - 20/07/2020
	------------------
	Use floating-point printer from mpaland/printf, letting us actually beat printf in all cases.



	1.1.1 - 20/07/2020
	------------------
	Don't include unistd.h



	1.1.0 - 20/07/2020
	------------------
	Performance improvements: switched to tuple-based arguments instead of parameter-packed recursion. We are now (slightly)
	faster than printf (as least on two of my systems) as long as no floating-point is involved. (for now we are still forced
	to call snprintf to print floats... charconv pls)

	Bug fixes: fixed broken escaping of {{ and }}.



	1.0.0 - 19/07/2020
	------------------
	Initial release.
*/
