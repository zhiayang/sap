/*
    zst.h
    Copyright 2020 - 2021, zhiayang

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
    Version 2.0.4
    =============



    Documentation
    =============

    Just a collection of various common utility classes/functions that aren't
    big enough to justify their own header library.

    Macros:

    - ZST_USE_STD
        this is *TRUE* by default. controls whether or not STL type interfaces
        are used; with it, str_view gains implicit constructors accepting
   std::string and std::string_view, as well as methods to convert to them (sv()
   and str()).

    - ZST_FREESTANDING
        this is *FALSE by default; controls whether or not a standard library
        implementation is available. if not, then memset(), memmove(), memcmp(),
        strlen(), and strncmp() are forward declared according to the C library
        specifications, but not defined.

    - ZST_HAVE_BUFFER
        this is *TRUE* by default. controls whether the zst::buffer<T> type is
        available. If you do not have operator new[] and want to avoid link
   errors, then set this to false.


    Note that ZST_FREESTANDING implies ZST_USE_STD = 0.

    Also note that buffer<T>, while it is itself a RAII type, it does *NOT*
    correctly RAII its contents. notably, no copy or move constructors will be
    called on any elements, only destructors. Thus, the element type should be
    limited to POD types.

    This may change in the future.


    Version history is at the bottom of the file.
*/
// clang-format off
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <concepts>
#include <type_traits>

#define ZST_DO_EXPAND(VAL)  VAL ## 1
#define ZST_EXPAND(VAL)     ZST_DO_EXPAND(VAL)

#if !defined(ZST_USE_STD)
	#define ZST_USE_STD 1
#elif (ZST_EXPAND(ZST_USE_STD) == 1)
	#undef ZST_USE_STD
	#define ZST_USE_STD 1
#endif

#if !defined(ZST_FREESTANDING)
	#define ZST_FREESTANDING 0
#elif (ZST_EXPAND(ZST_FREESTANDING) == 1)
	#undef ZST_FREESTANDING
	#define ZST_FREESTANDING 1
#endif

#if !defined(ZST_HAVE_BUFFER)
	#define ZST_HAVE_BUFFER 1
#elif (ZST_EXPAND(ZST_HAVE_BUFFER) == 1)
	#undef ZST_HAVE_BUFFER
	#define ZST_HAVE_BUFFER 1
#endif


#if !ZST_FREESTANDING
	#include <cstring>
#else
	#undef ZST_USE_STD
	#define ZST_USE_STD 0

	extern "C" void* memset(void* s, int c, size_t n);
	extern "C" int memcmp(const void* s1, const void* s2, size_t n);
	extern "C" void* memmove(void* dest, const void* src, size_t n);

	extern "C" size_t strlen(const char* s);
	extern "C" int strncmp(const char* s1, const char* s2, size_t n);
#endif

#if !ZST_FREESTANDING && ZST_USE_STD
	#include <string>
	#include <vector>
	#include <string_view>

	#include <memory>
#else

// if we DONT have std, then make a shitty unique_ptr that will allow SFINAE
namespace std
{
	template <int>
	class unique_ptr;
}

#endif



#undef ZST_DO_EXPAND
#undef ZST_EXPAND


// forward declare the zpr namespace...
namespace zpr
{
	// if zpr was included before us, then we don't forward declare.
#if !defined(ZPR_USE_STD) && !defined(ZPR_FREESTANDING)
	// as well as the print_formatter.
	template <typename, typename>
	struct print_formatter;
#endif

}

namespace zst
{
	// we need a forward-declaration of error_and_exit for expect().
	[[noreturn]] void error_and_exit(const char* str, size_t n);

	// we really just need a very small subset of <type_traits>, so don't include the whole thing.
	namespace detail
	{
		template <typename T> T min(T a, T b) { return a < b ? a : b;}

		template <typename T, typename... Ts>
		using is_same_as_any = std::disjunction<std::is_same<T, Ts>...>;

		template <typename T, typename... Ts>
		constexpr bool is_same_as_any_v = is_same_as_any<T, Ts...>::value;
	}

	namespace impl
	{
		template <typename... Args>
		[[noreturn]] void error_wrapper(const char* fmt, Args&&... args);
	}
}

#if (__cplusplus >= 202000L)
#include <bit>
#endif

namespace zst
{
	template <typename T, typename std::enable_if_t<std::is_signed_v<T>, int> = 0>
	inline T byteswap(T x)
	{
		return byteswap<std::make_unsigned_t<T>>(static_cast<std::make_unsigned_t<T>>(x));
	}

	inline uint16_t byteswap(uint16_t x)
	{
#if(_MSC_VER)
		return _byteswap_ushort(value);
#else
		// Compiles to bswap since gcc 4.5.3 and clang 3.4.1
		return (uint16_t) (((uint16_t) (x & 0x00ff) << 8) | ((uint16_t) (x & 0xff00) >> 8));
#endif
	}

	inline uint32_t byteswap(uint32_t x)
	{
#if(_MSC_VER)
		return _byteswap_ulong(value);
#else
		// Compiles to bswap since gcc 4.5.3 and clang 3.4.1
		return ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
#endif
	}

	inline uint64_t byteswap(uint64_t value)
	{
#if(_MSC_VER)
		return _byteswap_uint64(value);
#else
		// Compiles to bswap since gcc 4.5.3 and clang 3.4.1
		return ((value & 0xFF00000000000000u) >> 56u) | ((value & 0x00FF000000000000u) >> 40u)
		     | ((value & 0x0000FF0000000000u) >> 24u) | ((value & 0x000000FF00000000u) >> 8u)
		     | ((value & 0x00000000FF000000u) << 8u) | ((value & 0x0000000000FF0000u) << 24u)
		     | ((value & 0x000000000000FF00u) << 40u) | ((value & 0x00000000000000FFu) << 56u);
#endif
	}

	namespace impl
	{
		#if (__cplusplus >= 202000L)
			using endian = std::endian;
		#else
			// copied from cppref
			enum class endian
			{
			#if defined(_WIN32)
				little = 0,
			    big    = 1,
			    native = little,
		    #else
				little = __ORDER_LITTLE_ENDIAN__,
				big    = __ORDER_BIG_ENDIAN__,
				native = __BYTE_ORDER__,
			#endif
			};
		#endif

		template <typename CharType, impl::endian Endian = impl::endian::native>
		struct str_view
		{
			using value_type = CharType;

			constexpr str_view() : ptr(nullptr), len(0) { }
			constexpr str_view(const value_type* p, size_t l) : ptr(p), len(l) { }

			template <size_t N>
			constexpr str_view(const value_type (&s)[N]) : ptr(s), len(N - (std::is_same_v<char, value_type> ? 1 : 0)) { }

			template <typename T, typename std::enable_if_t<
				(std::is_same_v<value_type, char> || std::is_same_v<value_type, const char>) &&
				(std::is_same_v<char*, T> || std::is_same_v<const char*, T>), int> = 0>
			constexpr str_view(T s) : ptr(s), len(strlen(s)) { }

			constexpr str_view(str_view&&) = default;
			constexpr str_view(const str_view&) = default;
			constexpr str_view& operator= (str_view&&) = default;
			constexpr str_view& operator= (const str_view&) = default;

			constexpr inline bool operator== (const value_type* other) const requires(std::same_as<value_type, char>) {
				return *this == str_view(other);
			}

			constexpr inline bool operator== (const str_view& other) const
			{
				return (this->len == other.len) &&
					(this->ptr == other.ptr || (memcmp(this->ptr, other.ptr, this->len * sizeof(CharType)) == 0)
				);
			}

			constexpr inline bool operator!= (const str_view& other) const
			{
				return !(*this == other);
			}

			constexpr inline const value_type* begin() const { return this->ptr; }
			constexpr inline const value_type* end() const { return this->ptr + len; }

			constexpr inline size_t length() const { return this->len; }
			constexpr inline size_t size() const { return this->len; }
			constexpr inline bool empty() const { return this->len == 0; }
			constexpr inline const value_type* data() const { return this->ptr; }

			constexpr inline str_view<uint8_t> bytes() const
			{
				auto byte_ptr = static_cast<const void*>(this->ptr);
				return str_view<uint8_t>(static_cast<const uint8_t*>(byte_ptr), this->len);
			}

			value_type front() const { return this->ptr[0]; }
			value_type back() const { return this->ptr[this->len - 1]; }

			inline void clear() { this->ptr = 0; this->len = 0; }

			template <auto e = Endian, typename std::enable_if_t<e == impl::endian::native, int> = 0>
			inline value_type operator[] (size_t n) const { return this->ptr[n]; }

			template <auto e = Endian, typename std::enable_if_t<
				(e != impl::endian::native) && std::is_integral_v<value_type>,
			int> = 0>
			inline value_type operator[] (size_t n) const
			{
				return zst::byteswap(this->ptr[n]);
			}

			inline size_t find(value_type c) const { return this->find(str_view(&c, 1)); }
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

			inline size_t rfind(value_type c) const { return this->rfind(str_view(&c, 1)); }
			inline size_t rfind(str_view sv) const
			{
				if(sv.size() > this->size())
					return static_cast<size_t>(-1);

				else if(sv.empty())
					return this->size() - 1;

				for(size_t i = 1 + this->size() - sv.size(); i-- > 0;)
				{
					if(this->take(1 + i).take_last(sv.size()) == sv)
						return i;
				}

				return static_cast<size_t>(-1);
			}

			inline size_t find_first_of(str_view sv, size_t start = 0)
			{
				for(size_t i = start; i < this->len; i++)
				{
					for(value_type k : sv)
					{
						if(this->ptr[i] == k)
							return i;
					}
				}
				return static_cast<size_t>(-1);
			}

			inline size_t find_first_not_of(value_type k)
			{
				for(size_t i = 0; i < this->len; i++)
				{
					if(this->ptr[i] != k)
						return i;
				}
				return static_cast<size_t>(-1);
			}

			[[nodiscard]] str_view trim_whitespace() const
			{
				auto copy = *this;
				while(not copy.empty() && (copy[0] == value_type(' ') || copy[0] == value_type('\t') || copy[0] == value_type('\n') || copy[0] == value_type('\r')))
				{
					copy.remove_prefix(1);
				}

				while(not copy.empty() && (copy.back() == value_type(' ') || copy.back() == value_type('\t') || copy.back() == value_type('\n') || copy.back() == value_type('\r')))
				{
					copy.remove_suffix(1);
				}

				return copy;
			}

			[[nodiscard]] inline str_view drop(size_t n) const
			{ return (this->size() >= n ? this->substr(n, this->size() - n) : str_view { }); }

			[[nodiscard]] inline str_view take(size_t n) const
			{ return (this->size() >= n ? this->substr(0, n) : *this); }

			[[nodiscard]] inline str_view take_last(size_t n) const
			{ return (this->size() >= n ? this->substr(this->size() - n, n) : *this); }

			[[nodiscard]] inline str_view drop_last(size_t n) const
			{ return (this->size() >= n ? this->substr(0, this->size() - n) : *this); }

			[[nodiscard]] inline str_view substr(size_t pos = 0, size_t cnt = static_cast<size_t>(-1)) const
			{
				return str_view(this->ptr + pos, cnt == static_cast<size_t>(-1)
					? (this->len > pos ? this->len - pos : 0)
					: cnt);
			}

			[[nodiscard]] inline str_view drop_until(value_type c) const
			{
				auto n = this->find(c);
				if(n == size_t(-1))
					return *this;
				return this->drop(n);
			}

			[[nodiscard]] inline str_view take_until(value_type c) const
			{
				auto n = this->find(c);
				if(n == size_t(-1))
					return *this;
				return this->take(n);
			}

			inline bool starts_with(str_view sv) const { return this->take(sv.size()) == sv; }
			inline bool ends_with(str_view sv) const   { return this->take_last(sv.size()) == sv; }

			inline bool starts_with(value_type c) const { return this->size() > 0 && this->take(1)[0] == c; }
			inline bool ends_with(value_type c) const { return this->size() > 0 && this->take_last(1)[0] == c; }

			inline str_view& remove_prefix(size_t n) { return (*this = this->drop(n)); }
			inline str_view& remove_suffix(size_t n) { return (*this = this->drop_last(n)); }

			[[nodiscard]] inline str_view take_prefix(size_t n)
			{
				auto ret = this->take(n);
				this->remove_prefix(n);
				return ret;
			}

			[[nodiscard]] inline str_view take_suffix(size_t n)
			{
				auto ret = this->take_last(n);
				this->remove_suffix(n);
				return ret;
			}

			// these function is... potentially unsafe.
			inline str_view& transfer_prefix(str_view& other, size_t n)
			{
				if(&other == this)
					return *this;

				if(other.ptr == nullptr)
					other.ptr = this->ptr;
				else if(other.ptr + other.len != this->ptr)
					impl::error_wrapper("invalid use of transfer_prefix -- views are not contiguous");

				n = detail::min(n, this->len);
				other.len += n;

				return this->remove_prefix(n);
			}

			inline str_view& transfer_suffix(str_view& other, size_t n)
			{
				if(&other == this)
					return *this;

				if(other.ptr == nullptr)
					other.ptr = this->ptr + this->len;
				else if (this->ptr + this->len != other.ptr)
					impl::error_wrapper("invalid use of transfer_suffix -- views are not contiguous");

				n = detail::min(n, this->len);

				other.len += n;
				other.ptr -= n;

				return this->remove_suffix(n);
			}

		#if ZST_USE_STD
			template <typename T = value_type, std::enable_if_t<std::is_trivial_v<T> && std::is_same_v<T, value_type> && detail::is_same_as_any_v<T, char, char8_t, char16_t, char32_t>, int> = 0>
			str_view(const std::basic_string<T>& s) : ptr(s.data()), len(s.size()) { }

			template <typename T = value_type, std::enable_if_t<std::is_trivial_v<T> && std::is_same_v<T, value_type> && detail::is_same_as_any_v<T, char, char8_t, char16_t, char32_t>, int> = 0>
			str_view(std::basic_string_view<T> sv) : ptr(sv.data()), len(sv.size()) { }

			template <typename T = value_type, std::enable_if_t<std::is_trivial_v<T> && std::is_same_v<T, value_type> && detail::is_same_as_any_v<T, char, char8_t, char16_t, char32_t>, int> = 0>
			inline std::basic_string_view<T> sv() const
			{
				return std::basic_string_view<value_type>(this->data(), this->size());
			}

			template <typename T = value_type, std::enable_if_t<std::is_trivial_v<T> && std::is_same_v<T, value_type> && detail::is_same_as_any_v<T, char, char8_t, char16_t, char32_t>, int> = 0>
			inline std::basic_string<T> str() const
			{
				return std::basic_string<value_type>(this->data(), this->size());
			}
		#endif // ZST_USE_STD


			// this must appear inside the class body... see the comment below about these leaky macros.
		#if defined(ZPR_USE_STD) || defined(ZPR_FREESTANDING)

			template <typename T = value_type, typename = typename std::enable_if_t<std::is_same_v<T, char>>>
			operator zpr::tt::str_view() const
			{
				return zpr::tt::str_view(this->ptr, this->len);
			}

		#endif

			// a little hacky, but a useful feature.
			template <typename A = value_type, typename = typename std::enable_if_t<std::is_same_v<A, uint8_t>>>
			str_view<char> chars() const
			{
				return str_view<char>(reinterpret_cast<const char*>(this->ptr), this->len);
			}

			template <typename U, impl::endian E = Endian>
			str_view<U, E> cast() const
			{
				static_assert(sizeof(value_type) % sizeof(U) == 0 || sizeof(U) % sizeof(value_type) == 0, "types are not castable");
				return str_view<U, E>(reinterpret_cast<const U*>(this->ptr), (sizeof(value_type) * this->len) / sizeof(U));
			}

		private:
			const value_type* ptr;
			size_t len;
		};

		#if ZST_USE_STD
			template <typename C>
			inline bool operator== (const std::basic_string<C>& other, str_view<C> sv) { return sv == str_view<C>(other); }

			template <typename C>
			inline bool operator== (str_view<C> sv, const std::basic_string<C>& other) { return sv == str_view<C>(other); }

			template <typename C>
			inline bool operator== (const std::basic_string_view<C>& other, str_view<C> sv) { return sv == str_view<C>(other); }

			template <typename C>
			inline bool operator== (str_view<C> sv, const std::basic_string_view<C>& other) { return sv == str_view<C>(other); }

			template <typename C>
			inline bool operator!= (const std::basic_string<C>& other, str_view<C> sv) { return sv != str_view<C>(other); }

			template <typename C>
			inline bool operator!= (str_view<C> sv, const std::basic_string<C>& other) { return sv != str_view<C>(other); }

			template <typename C>
			inline bool operator!= (std::basic_string_view<C> other, str_view<C> sv) { return sv != str_view<C>(other); }

			template <typename C>
			inline bool operator!= (str_view<C> sv, std::basic_string_view<C> other) { return sv != str_view<C>(other); }


			// for vector
			template <typename T, auto E>
			inline bool operator== (str_view<T,E> sv, const std::vector<T>& other)
			{
				return sv == str_view<T>(other.data(), other.size());
			}

			template <typename T, auto E>
			inline bool operator== (const std::vector<T>& other, str_view<T,E> sv)
			{
				return sv == str_view<T>(other.data(), other.size());
			}

			template <typename T, auto E>
			inline bool operator!= (str_view<T,E> sv, const std::vector<T>& other)
			{
				return sv != str_view<T>(other.data(), other.size());
			}
			template <typename T, auto E>
			inline bool operator!= (const std::vector<T>& other, str_view<T,E> sv)
			{
				return sv != str_view<T>(other.data(), other.size());
			}

		#endif // ZST_USE_STD

		template <typename C> inline const C* begin(const str_view<C>& sv) { return sv.begin(); }
		template <typename C> inline const C* end(const str_view<C>& sv) { return sv.end(); }
	}

	using str_view = impl::str_view<char>;
	using wstr_view = impl::str_view<char32_t>;

	template <typename T, impl::endian E = impl::endian::native>
	using span = impl::str_view<T, E>;

	using byte_span = span<uint8_t>;
}


#if ZST_HAVE_BUFFER
namespace zst
{
	namespace impl
	{
		template <typename value_type>
		struct buffer
		{
			buffer(const buffer& b) = delete;
			buffer& operator= (const buffer& b) = delete;


			buffer() : buffer(64) { }
			buffer(size_t capacity) : mem(new value_type[capacity]), cap(capacity), len(0) { }

			~buffer()
			{
				if(this->mem != nullptr)
					delete[] this->mem;

				this->mem = 0;
				this->len = 0;
				this->cap = 0;
			}

			buffer(buffer&& b) : mem(b.mem), cap(b.cap), len(b.len)
			{
				b.mem = nullptr;
				b.cap = 0;
				b.len = 0;
			}

			buffer& operator= (buffer&& b)
			{
				if(this != &b)
				{
					if(this->mem)
						delete[] this->mem;

					this->mem = b.mem;  b.mem = nullptr;
					this->len = b.len;  b.len = 0;
					this->cap = b.cap;  b.cap = 0;
				}
				return *this;
			}


			inline buffer clone() const
			{
				auto ret = buffer(this->cap);
				ret.append(*this);
				return ret;
			}

			inline bool operator== (const buffer& b) const
			{
				return this->len == b.len
					&& memcmp(this->mem, b.mem, sizeof(value_type) * this->len) == 0;
			}

			inline bool operator!= (const buffer& b) const
			{
				return !(*this == b);
			}

			inline bool empty() const { return this->len == 0; }
			inline size_t size() const { return this->len; }
			inline size_t length() const { return this->len; }

			inline size_t capacity() const { return this->cap; }

			inline value_type* data() { return this->mem; }
			inline const value_type* data() const { return this->mem; }

			inline const value_type* begin() const { return this->mem; }
			inline const value_type* end() const { return this->mem + this->len; }

			inline byte_span bytes() const
			{
				return byte_span(reinterpret_cast<const uint8_t*>(this->mem),
					this->len * sizeof(value_type));
			}

			inline zst::span<value_type> span() const
			{
				return zst::span<value_type>(this->mem, this->len);
			}

			inline void clear()
			{
				memset(this->mem, 0, sizeof(value_type) * this->len);
				this->len = 0;
			}

			inline buffer& append(value_type x)
			{
				this->expand(1);
				this->mem[this->len++] = static_cast<value_type&&>(x);
				return *this;
			}

			inline buffer& append(zst::span<value_type> span)
			{
				this->expand(span.size());
				for(auto& x : span)
					this->mem[this->len++] = x;

				return *this;
			}

			inline buffer& append(const buffer<value_type>& buf)
			{
				this->expand(buf.size());
				for(auto& x : buf)
					this->mem[this->len++] = x;

				return *this;
			}

			inline buffer& append(const value_type* arr, size_t num)
			{
				this->expand(num);
				memmove(&this->mem[this->len], arr, sizeof(value_type) * num);
				this->len += num;

				return *this;
			}

			template <typename T, typename A = value_type, typename = typename std::enable_if_t<std::is_same_v<A, uint8_t>>>
			inline buffer& append_bytes(const T& value)
			{
				this->expand(sizeof(T));
				memmove(&this->mem[this->len], &value, sizeof(T));
				this->len += sizeof(T);
				return *this;
			}


		private:
			inline void expand(size_t extra)
			{
				if(this->len + extra <= this->cap)
					return;

				auto newcap = (this->len * 3) / 2;
				if(this->len + extra > newcap)
					newcap = this->len + extra;

				auto newmem = new value_type[newcap];
				memmove(newmem, this->mem, sizeof(value_type) * this->len);

				delete[] this->mem;
				this->mem = newmem;
				this->cap = newcap;
			}


			value_type* mem = 0;
			size_t cap = 0;
			size_t len = 0;
		};
	}

	template <typename C> inline const C* begin(const impl::buffer<C>& sv) { return sv.begin(); }
	template <typename C> inline const C* end(const impl::buffer<C>& sv) { return sv.end(); }

	template <typename T>
	using buffer = impl::buffer<T>;

	using byte_buffer = impl::buffer<uint8_t>;
}
#endif



namespace zst
{
	#if (__cplusplus >= 202000L)
		#define ZST_NO_UNIQUE_ADDRESS [[no_unique_address]]
	#else
		#define ZST_NO_UNIQUE_ADDRESS
	#endif

	namespace impl
	{
		struct default_deleter
		{
			default_deleter() = default;
			explicit default_deleter(void (*user_deleter)(const void*, size_t)) : m_user_deleter(user_deleter) { }

			template <typename T>
			void operator()(const T* ptr, size_t n)
			{
				if(m_user_deleter != nullptr)
					m_user_deleter(ptr, n);
				else
					delete[] ptr;
			}

			void (*m_user_deleter)(const void*, size_t) = nullptr;
		};
	}

	template <typename _ArrayType, typename _Deleter = impl::default_deleter,
		typename std::enable_if_t<std::is_array_v<std::remove_reference_t<std::remove_cv_t<_ArrayType>>>, int> = 0>
	struct unique_span
	{
		using _TPtr = std::decay_t<std::remove_reference_t<std::remove_cv_t<_ArrayType>>>;

		static_assert(std::is_pointer_v<_TPtr>);
		using _T = std::remove_pointer_t<_TPtr>;

		template <typename D = _Deleter, typename std::enable_if_t<std::is_default_constructible_v<D>, int> = 0>
		explicit unique_span(D del = _Deleter{})
			: m_ptr(nullptr)
			, m_size(0)
			, m_deleter(static_cast<D&&>(del)) { }

		template <typename D = _Deleter, typename std::enable_if_t<std::is_default_constructible_v<D>, int> = 0>
		explicit unique_span(_T* ptr, size_t size, D del = _Deleter{})
			: m_ptr(ptr)
			, m_size(size)
			, m_deleter(static_cast<D&&>(del)) { }

		unique_span(const unique_span&) = delete;
		unique_span& operator=(const unique_span&) = delete;

		unique_span(unique_span&& other)
			: m_ptr(other.m_ptr)
			, m_size(other.m_size)
			, m_deleter(static_cast<_Deleter&&>(other.m_deleter))
		{
			other.m_ptr = nullptr;
			other.m_size = 0;
		}

		unique_span& operator=(unique_span&& other)
		{
			if(this == &other)
				return *this;

			m_ptr = other.m_ptr; other.m_ptr = nullptr;
			m_size = other.m_size; other.m_size = 0;
			m_deleter = static_cast<_Deleter&&>(other.m_deleter);
			return *this;
		}

		~unique_span()
		{
			if(m_ptr != nullptr)
				m_deleter(m_ptr, m_size);

			m_ptr = nullptr;
			m_size = 0;
		}

		bool operator==(decltype(nullptr) _) const { return m_ptr == nullptr; }
		bool operator!=(decltype(nullptr) _) const { return m_ptr != nullptr; }

		_T* get() { return m_ptr; }
		const _T* get() const { return m_ptr; }

		size_t size() const { return m_size; }
		bool empty() const { return m_size == 0; }

		_T& operator[](size_t n) { return m_ptr[n]; }
		const _T& operator[](size_t n) const { return m_ptr[n]; }

		zst::span<_T> span() const { return zst::span<_T>(m_ptr, m_size); }

		_T* release()
		{
			auto ret = m_ptr;
			m_ptr = nullptr;
			return ret;
		}

	private:
		_T* m_ptr;
		size_t m_size;
		ZST_NO_UNIQUE_ADDRESS _Deleter m_deleter;
	};
}



namespace zst
{
	template <typename, typename>
	struct Result;

	namespace detail
	{
		template <typename ToUniquePtr, typename FromUniquePtr>
		concept UniquePtrConvertible = requires(FromUniquePtr f)
		{
			typename FromUniquePtr::pointer;
			typename FromUniquePtr::element_type;
			typename FromUniquePtr::deleter_type;

			typename ToUniquePtr::pointer;
			typename ToUniquePtr::element_type;
			typename ToUniquePtr::deleter_type;

			requires std::derived_from<
				std::remove_pointer_t<typename FromUniquePtr::pointer>,
				std::remove_pointer_t<typename ToUniquePtr::pointer>
			>;

			{ f.release() } -> std::same_as<typename FromUniquePtr::pointer>;
		};

		template <typename ToSharedPtr, typename FromSharedPtr>
		concept SharedPtrConvertible = requires(FromSharedPtr f)
		{
			typename FromSharedPtr::element_type;
			typename ToSharedPtr::element_type;

			requires std::derived_from<
				typename FromSharedPtr::element_type,
				typename ToSharedPtr::element_type
			>;

			{ f.get() } -> std::same_as<typename FromSharedPtr::element_type*>;
			{ f.use_count() } -> std::same_as<size_t>;
		};
	}

	template <typename T>
	struct Ok
	{
		Ok() = default;
		~Ok() = default;

		Ok(Ok&&) = delete;
		Ok(const Ok&) = delete;

		Ok(const T& value) : m_value(value) { }
		Ok(T&& value) : m_value(static_cast<T&&>(value)) { }

		template <typename... Args>
		Ok(Args&&... args) : m_value(static_cast<Args&&>(args)...) { }

		template <typename, typename> friend struct Result;

	private:
		T m_value;
	};

	template <>
	struct Ok<void>
	{
		Ok() = default;
		~Ok() = default;

		Ok(Ok&&) = delete;
		Ok(const Ok&) = delete;

		template <typename, typename> friend struct Result;
		template <typename, typename> friend struct zpr::print_formatter;
	};

	// deduction guide
	Ok() -> Ok<void>;


	template <typename E>
	struct Err
	{
		Err() = default;
		~Err() = default;

		Err(Err&&) = delete;
		Err(const Err&) = delete;

		Err(const E& error) : m_error(error) { }
		Err(E&& error) : m_error(static_cast<E&&>(error)) { }

		template <typename... Args>
		Err(Args&&... args) : m_error(static_cast<Args&&>(args)...) { }

		template <typename, typename> friend struct Result;
		template <typename, typename> friend struct zpr::print_formatter;

	private:
		E m_error;
	};

	template <typename T, typename E>
	struct Result
	{
	private:
		static constexpr int STATE_NONE = 0;
		static constexpr int STATE_VAL  = 1;
		static constexpr int STATE_ERR  = 2;

		struct tag_ok { };
		struct tag_err { };

		Result(tag_ok _, const T& x)  : state(STATE_VAL), val(x) { (void) _; }
		Result(tag_ok _, T&& x)       : state(STATE_VAL), val(static_cast<T&&>(x)) { (void) _; }

		Result(tag_err _, const E& e) : state(STATE_ERR), err(e) { (void) _; }
		Result(tag_err _, E&& e)      : state(STATE_ERR), err(static_cast<E&&>(e)) { (void) _; }

	public:
		using value_type = T;
		using error_type = E;

		~Result()
		{
			if(state == STATE_VAL) this->val.~T();
			if(state == STATE_ERR) this->err.~E();
		}

		template <typename T1 = T>
		Result(Ok<T1>&& ok_) : Result(tag_ok(), static_cast<T1&&>(ok_.m_value)) { }

		template <typename E1 = E>
		Result(Err<E1>&& err_) : Result(tag_err(), static_cast<E1&&>(err_.m_error)) { }

		Result(const Result& other)
		{
			this->state = other.state;
			if(this->state == STATE_VAL) new(&this->val) T(other.val);
			if(this->state == STATE_ERR) new(&this->err) E(other.err);
		}

		Result(Result&& other)
		{
			this->state = other.state;
			other.state = STATE_NONE;

			if(this->state == STATE_VAL) new(&this->val) T(static_cast<T&&>(other.val));
			if(this->state == STATE_ERR) new(&this->err) E(static_cast<E&&>(other.err));
		}

		Result& operator=(const Result& other)
		{
			if(this != &other)
			{
				if(this->state == STATE_VAL) this->val.~T();
				if(this->state == STATE_ERR) this->err.~E();

				this->state = other.state;
				if(this->state == STATE_VAL) new(&this->val) T(other.val);
				if(this->state == STATE_ERR) new(&this->err) E(other.err);
			}
			return *this;
		}

		Result& operator=(Result&& other)
		{
			if(this != &other)
			{
				if(this->state == STATE_VAL) this->val.~T();
				if(this->state == STATE_ERR) this->err.~E();

				this->state = other.state;
				other.state = STATE_NONE;

				if(this->state == STATE_VAL) new(&this->val) T(static_cast<T&&>(other.val));
				if(this->state == STATE_ERR) new(&this->err) E(static_cast<E&&>(other.err));
			}
			return *this;
		}

		T* operator -> () { this->assert_has_value(); return &this->val; }
		const T* operator -> () const { this->assert_has_value(); return &this->val; }

		T& operator* () { this->assert_has_value(); return this->val; }
		const T& operator* () const  { this->assert_has_value(); return this->val; }

		explicit operator bool() const { return this->state == STATE_VAL; }
		bool ok() const { return this->state == STATE_VAL; }
		bool is_err() const { return this->state == STATE_ERR; }

		const T& unwrap() const { this->assert_has_value(); return this->val; }
		const E& error() const { this->assert_is_error(); return this->err; }

		T& unwrap() { this->assert_has_value(); return this->val; }
		E& error() { this->assert_is_error(); return this->err; }

		// enable implicit upcast to a base type
		template <typename U> requires(std::is_pointer_v<T>
			&& std::is_pointer_v<U>
			&& std::is_base_of_v<std::remove_pointer_t<U>, std::remove_pointer_t<T>>
		)
		operator Result<U, E> () const
		{
			using R = Result<U, E>;
			if(state == STATE_VAL)  return R(typename R::tag_ok{}, this->val);
			if(state == STATE_ERR)  return R(typename R::tag_err{}, this->err);

			impl::error_wrapper("invalid state of Result");
		}

		template <detail::UniquePtrConvertible<T> U>
		operator Result<U, E>() &&
		{
			using R = Result<U, E>;
			if(state == STATE_VAL)  return R(typename R::tag_ok{}, U(this->val.release()));
			if(state == STATE_ERR)  return R(typename R::tag_err{}, static_cast<E&&>(this->err));

			impl::error_wrapper("invalid state of Result");
		}

		template <detail::SharedPtrConvertible<T> U>
		operator Result<U, E>() &&
		{
			using R = Result<U, E>;
			if(state == STATE_VAL)  return R(typename R::tag_ok{}, U(static_cast<T&&>(this->val)));
			if(state == STATE_ERR)  return R(typename R::tag_err{}, static_cast<E&&>(this->err));

			impl::error_wrapper("invalid state of Result");
		}



		const T& expect(str_view msg) const
		{
			if(this->ok())  return this->unwrap();
			else            impl::error_wrapper("{}: {}", msg, this->error());
		}

		const T& or_else(const T& default_value) const
		{
			if(this->ok())  return this->unwrap();
			else            return default_value;
		}

		template <typename Fn>
		auto map(Fn&& fn) const -> Result<decltype(fn(std::declval<T>())), E>
		{
			using Res = Result<decltype(fn(this->val)), E>;
			if constexpr (std::is_same_v<decltype(fn(this->val)), void>)
			{
				if(this->ok())  return Res();
				else            return Err(this->err);
			}
			else
			{
				if(this->ok())  return Res(typename Res::tag_ok{}, fn(this->val));
				else            return Res(typename Res::tag_err{}, this->err);
			}
		}

		template <typename Fn>
		auto flatmap(Fn&& fn) const -> decltype(fn(std::declval<T>()))
		{
			using Res = decltype(fn(this->val));

			if(this->ok())  return fn(this->val);
			else            return Res(typename Res::tag_err{}, this->err);
		}

		Result<void, E> remove_value() const
		{
			using Res = Result<void, E>;
			if(this->ok())  return Res(typename Res::tag_ok{});
			else            return Res(typename Res::tag_err{}, this->err);
		}

		Err<E> to_err() { this->assert_is_error(); this->state = STATE_NONE; return Err(static_cast<E&&>(this->err)); }

		E take_error() { this->assert_is_error(); this->state = STATE_NONE; return static_cast<E&&>(this->err); }
		T take_value() { this->assert_has_value(); this->state = STATE_NONE; return static_cast<T&&>(this->val); }

	private:
		inline void assert_has_value() const
		{
			if(this->state != STATE_VAL)
				impl::error_wrapper("unwrapping result of Err: {}", this->error());
		}

		inline void assert_is_error() const
		{
			if(this->state != STATE_ERR)
				impl::error_wrapper("result is not an Err");
		}


		// 0 = schrodinger -- no error, no value.
		// 1 = valid
		// 2 = error
		int state = 0;
		union {
			T val;
			E err;
		};

		// befriend all results
		template <typename, typename> friend struct Result;
	};


	template <typename E>
	struct Result<void, E>
	{
	private:
		static constexpr int STATE_NONE = 0;
		static constexpr int STATE_VAL  = 1;
		static constexpr int STATE_ERR  = 2;

	public:
		using value_type = void;
		using error_type = E;

		struct tag_ok { };
		struct tag_err { };

		Result() : state(STATE_VAL) { }

		Result(Ok<void>&& ok) : Result() { (void) ok; }

		template <typename E1 = E>
		Result(Err<E1>&& err_) : state(STATE_ERR), err(static_cast<E1&&>(err_.m_error)) { }

		Result([[maybe_unused]] tag_ok _) : state(STATE_VAL) { }
		Result([[maybe_unused]] tag_err _, const E& err_) : state(STATE_ERR), err(err_) { }


		Result(const Result& other)
		{
			this->state = other.state;
			if(this->state == STATE_ERR)
				this->err = other.err;
		}

		Result(Result&& other)
		{
			this->state = other.state;
			other.state = STATE_NONE;

			if(this->state == STATE_ERR)
				this->err = static_cast<E&&>(other.err);
		}

		Result& operator=(const Result& other)
		{
			if(this != &other)
			{
				this->state = other.state;
				if(this->state == STATE_ERR)
					this->err = other.err;
			}
			return *this;
		}

		Result& operator=(Result&& other)
		{
			if(this != &other)
			{
				this->state = other.state;
				other.state = STATE_NONE;

				if(this->state == STATE_ERR)
					this->err = static_cast<E&&>(other.err);
			}
			return *this;
		}

		explicit operator bool() const { return this->state == STATE_VAL; }
		bool ok() const { return this->state == STATE_VAL; }
		bool is_err() const { return this->state == STATE_ERR; }

		const E& error() const { this->assert_is_error(); return this->err; }
		E& error() { this->assert_is_error(); return this->err; }

		Err<E> to_err() { this->assert_is_error(); return Err(static_cast<E&&>(this->err)); }
		E take_error() { this->assert_is_error(); this->state = STATE_NONE; return static_cast<E&&>(this->err); }

		void expect(zst::str_view msg) const
		{
			if(!this->ok())
				impl::error_wrapper("{}: {}", msg, this->error());
		}

		static Result of_value()
		{
			return Result<void, E>();
		}

		template <typename E1 = E>
		static Result of_error(E1&& err)
		{
			return Result<void, E>(E(static_cast<E1&&>(err)));
		}

		template <typename... Args>
		static Result of_error(Args&&... xs)
		{
			return Result<void, E>(E(static_cast<Args&&>(xs)...));
		}

		template <typename Fn>
		auto map(Fn&& fn) const -> Result<decltype(fn()), E>
		{
			using Res = Result<decltype(fn()), E>;
			if(this->ok())  return Res(typename Res::tag_ok{}, fn());
			else            return Res(typename Res::tag_err{}, this->err);
		}

		template <typename Fn>
		auto flatmap(Fn&& fn) const -> decltype(fn())
		{
			using Res = decltype(fn());

			if(this->ok())  return fn();
			else            return Res(typename Res::tag_err{}, this->err);
		}

		template <typename T>
		Result<T, E> add_value(T&& value) const
		{
			using Res = Result<T, E>;
			if(this->ok())  return Res(typename Res::tag_ok{}, static_cast<T&&>(value));
			else            return Res(typename Res::tag_err{}, this->err);
		}

	private:
		inline void assert_is_error() const
		{
			if(this->state != STATE_ERR)
				impl::error_wrapper("result is not an Err");
		}

		int state = 0;
		E err;
	};

	template <typename E>
	using Failable = Result<void, E>;





	// TODO: see if we can share code between Either and Result
	template <typename T>
	struct Left
	{
		Left() = default;
		~Left() = default;
		Left(Left&&) = default;
		Left(const Left&) = default;

		Left(const T& value) : m_value(value) { }
		Left(T&& value) : m_value(static_cast<T&&>(value)) { }

		template <typename... Args>
		Left(Args&&... args) : m_value(static_cast<Args&&>(args)...) { }

		template <typename, typename> friend struct Either;

	private:
		T m_value;
	};

	template <typename T>
	struct Right
	{
		Right() = default;
		~Right() = default;
		Right(Right&&) = default;
		Right(const Right&) = default;

		Right(const T& value) : m_value(value) { }
		Right(T&& value) : m_value(static_cast<T&&>(value)) { }

		template <typename... Args>
		Right(Args&&... args) : m_value(static_cast<Args&&>(args)...) { }

		template <typename, typename> friend struct Either;

	private:
		T m_value;
	};

	template <typename LT, typename RT>
	struct Either
	{
	private:
		static constexpr int STATE_NONE  = 0;
		static constexpr int STATE_LEFT  = 1;
		static constexpr int STATE_RIGHT = 2;

		struct tag_left {};
		struct tag_right {};

		Either(tag_left _, const LT& x)  : m_state(STATE_LEFT), m_left(x) { (void) _; }
		Either(tag_left _, LT&& x)       : m_state(STATE_LEFT), m_left(static_cast<LT&&>(x)) { (void) _; }

		Either(tag_right _, const RT& x) : m_state(STATE_RIGHT), m_right(x) { (void) _; }
		Either(tag_right _, RT&& x)      : m_state(STATE_RIGHT), m_right(static_cast<RT&&>(x)) { (void) _; }

	public:
		using left_type = LT;
		using right_type = RT;

		~Either()
		{
			if(m_state == STATE_LEFT)   m_left.~LT();
			if(m_state == STATE_RIGHT)  m_right.~RT();
		}

		template <typename T1 = LT>
		Either(Left<T1>&& x) : Either(tag_left(), static_cast<T1&&>(x.m_value)) { }

		template <typename T1 = RT>
		Either(Right<T1>&& x) : Either(tag_right(), static_cast<T1&&>(x.m_value)) { }

		Either(const Either& other)
		{
			m_state = other.m_state;
			if(m_state == STATE_LEFT)  new(&m_left) LT(other.m_left);
			if(m_state == STATE_RIGHT) new(&m_right) RT(other.m_right);
		}

		Either(Either&& other)
		{
			m_state = other.m_state;
			other.m_state = STATE_NONE;
			if(m_state == STATE_LEFT)  new(&m_left) LT(static_cast<LT&&>(other.m_left));
			if(m_state == STATE_RIGHT) new(&m_right) RT(static_cast<RT&&>(other.m_right));
		}

		Either& operator=(const Either& other)
		{
			if(this != &other)
			{
				if(m_state == STATE_LEFT)  m_left.~LT();
				if(m_state == STATE_RIGHT) m_right.~RT();

				m_state = other.m_state;
				if(m_state == STATE_LEFT) new(&m_left) LT(other.m_left);
				if(m_state == STATE_RIGHT) new(&m_right) RT(other.m_right);
			}
			return *this;
		}

		Either& operator=(Either&& other)
		{
			if(this != &other)
			{
				if(m_state == STATE_LEFT) m_left.~LT();
				if(m_state == STATE_RIGHT) m_right.~RT();

				m_state = other.m_state;
				other.m_state = STATE_NONE;
				if(m_state == STATE_LEFT) new(&m_left) LT(static_cast<LT&&>(other.m_left));
				if(m_state == STATE_RIGHT) new(&m_right) RT(static_cast<RT&&>(other.m_right));
			}
			return *this;
		}

		bool is_left() const { return m_state == STATE_LEFT; }
		bool is_right() const { return m_state == STATE_RIGHT; }

		const LT& left() const { this->assert_is_left(); return m_left; }
		const RT& right() const { this->assert_is_right(); return m_right; }

		LT& left()  { this->assert_is_left(); return m_left; }
		RT& right() { this->assert_is_right(); return m_right; }

		LT take_left() { this->assert_is_left(); m_state = STATE_NONE; return static_cast<LT&&>(m_left); }
		RT take_right() { this->assert_is_right(); m_state = STATE_NONE; return static_cast<RT&&>(m_right); }

		const LT* maybe_left() const { if(this->is_left()) return &m_left; else return nullptr; }
		const RT* maybe_right() const { if(this->is_right()) return &m_right; else return nullptr; }

	private:
		inline void assert_is_left() const
		{
			if(m_state != STATE_LEFT)
				impl::error_wrapper("expected Left, was Right!");
		}

		inline void assert_is_right() const
		{
			if(m_state != STATE_RIGHT)
				impl::error_wrapper("expected Right, was Left!");
		}

		int m_state = 0;
		union {
			LT m_left;
			RT m_right;
		};

		template <typename, typename> friend struct Either;
	};

}




// we rely on zpr.h defining some macros that "leak" globally. zpr.h always "force-defines" these macros,
// so we can be sure that if they are defined, then zpr.h is included.

#if defined(ZPR_USE_STD) || defined(ZPR_FREESTANDING)

namespace zst
{
	#if (ZST_USE_STD == 1) && (ZST_FREESTANDING == 0)
	template <typename... Args>
	Err<std::string> ErrFmt(const char* fmt, Args&&... args)
	{
		return Err<std::string>(zpr::sprint(fmt, static_cast<Args&&>(args)...));
	}
	#endif

	namespace impl
	{
		template <typename... Args>
		[[noreturn]] void error_wrapper(const char* fmt, Args&&... args)
		{
			char buf[1024] { };
			auto n = zpr::sprint(1024, buf, fmt, static_cast<Args&&>(args)...);
			zst::error_and_exit(buf, n);
		}
	}
}

namespace zpr
{
	// make sure that these are only defined if print_formatter is a complete type.
	template <typename T>
	struct print_formatter<zst::Ok<T>, typename std::enable_if_t<detail::has_formatter_v<T>>>
	{
		template <typename Cb>
		void print(const zst::Ok<T>& ok, Cb&& cb, format_args args)
		{
			cb("Ok(");
			detail::print_one(cb, static_cast<format_args&&>(args), ok.m_value);
			cb(")");
		}
	};

	template <typename E>
	struct print_formatter<zst::Err<E>, typename std::enable_if_t<detail::has_formatter_v<E>>>
	{
		template <typename Cb>
		void print(const zst::Err<E>& err, Cb&& cb, format_args args)
		{
			cb("Err(");
			detail::print_one(cb, static_cast<format_args&&>(args), err.m_error);
			cb(")");
		}
	};

	template <typename E, typename T>
	struct print_formatter<zst::Result<T, E>,
		typename std::enable_if_t<detail::has_formatter_v<T> && detail::has_formatter_v<E>>
	>
	{
		template <typename Cb>
		void print(const zst::Result<T, E>& res, Cb&& cb, format_args args)
		{
			if(res.ok())
			{
				cb("Ok(");
				detail::print_one(cb, static_cast<format_args&&>(args), res.unwrap());
				cb(")");
			}
			else
			{
				cb("Err(");
				detail::print_one(cb, static_cast<format_args&&>(args), res.error());
				cb(")");
			}
		}
	};
}
#else

// we need something here...

namespace zst::impl
{
	template <typename... Args>
	[[noreturn]] void error_wrapper([[maybe_unused]] const char* fmt, [[maybe_unused]] Args&&... args)
	{
		constexpr const char msg[] = "internal error (no zpr, cannot elaborate)";
		zst::error_and_exit(msg, sizeof(msg) - 1);
	}
}

#endif

constexpr inline zst::str_view operator""_sv(const char* s, size_t n)
{
	return zst::str_view(s, n);
}

constexpr inline zst::byte_span operator""_bs(const char* s, size_t n)
{
	return zst::str_view(s, n).bytes();
}



/*
    Version History
    ===============

    2.0.4 - 06/02/2025
    ------------------
    - Fix more str_view brokenness


    2.0.3 - 04/02/2025
    ------------------
    - Fix Wparentheses warning


    2.0.2 - 24/12/2024
    ------------------
    - Fix erroneous N-1 length accounting for reference-to-array constructor for str_view for non-char cases


    2.0.1 - 31/07/2023
    ------------------
    - Add missing <vector> include


    2.0.0 - 26/04/2023
    ------------------
    - switch to C++20, partial conversion to concepts
    - add automatic derived -> base casting for Result<std::unique_ptr<T>>
    - add Either<L, R>



    1.4.3 - 18/01/2023
    ------------------
    - add endianness support to str_view
    - add transfer_suffix
    - add unique_span
    - add _bs and _sv literals
    - add `drop_until` and `take_until` for str_view


    1.4.2 - 26/09/2022
    ------------------
    - fix wrong placement of const in `map` and `flatmap`
    - add `add_value` function to Result<void>
    - add `remove_value` function to Result<T>
    - add Failable type alias
    - fix missing includes, and only define `ErrFmt` in non-freestanding mode


    1.4.1 - 10/08/2022
    ------------------
    - fix implicit conversion warnings



    1.4.0 - 08/09/2021
    ------------------
    - add map() and flatmap() to Result



    1.3.3 - 31/08/2021
    ------------------
    - Fix implicit cast for inherited classes in Result
    - Rearrange how the error function is called. It's now a normal function
      that just takes a string+length, because obviously we can't instantiate all possible
      templates ahead of time.



    1.3.2 - 23/08/2021
    ------------------
    Fix forward declaration of zpr::print_formatter.



    1.3.1 - 08/08/2021
    ------------------
    Fix a bug in buffer<T>::append_bytes being SFINAE-ed wrongly



    1.3.0 - 03/08/2021
    ------------------
    Add buffer<T>, which is essentially a lightweight std::vector. requires operator new[];

    Add chars() for str_view where T = uint8_t, which returns a normal str_view
    Add span<T> (== str_view<T>) and byte_span (== str_view<uint8_t>)



    1.2.1 - 03/08/2021
    ------------------
    Make str_view::operator[] const



    1.2.0 - 05/06/2021
    ------------------
    Add `starts_with` and `ends_with` for str_view



    1.1.0 - 28/05/2021
    ------------------
    Various changes...



    1.0.0 - 24/05/2021
    ------------------
    Initial Release
*/
