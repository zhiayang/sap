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
	Version 1.4.1
	=============



	Documentation
	=============

	Just a collection of various common utility classes/functions that aren't big enough
	to justify their own header library.

	Macros:

	- ZST_USE_STD
		this is *TRUE* by default. controls whether or not STL type interfaces are used; with it,
		str_view gains implicit constructors accepting std::string and std::string_view, as well
		as methods to convert to them (sv() and str()).

	- ZST_FREESTANDING
		this is *FALSE by default; controls whether or not a standard library implementation is
		available. if not, then memset(), memmove(), memcmp(), strlen(), and strncmp() are forward declared
		according to the C library specifications, but not defined.

	- ZST_HAVE_BUFFER
		this is *TRUE* by default. controls whether the zst::buffer<T> type is available. If you do not have
		operator new[] and want to avoid link errors, then set this to false.


	Note that ZST_FREESTANDING implies ZST_USE_STD = 0.

	Also note that buffer<T>, while it is itself a RAII type, it does *NOT* correctly RAII its contents.
	notably, no copy or move constructors will be called on any elements, only destructors. Thus, the
	element type should be limited to POD types.

	This may change in the future.


	Version history is at the bottom of the file.
*/

#pragma once

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
	#include <string_view>
#endif

#undef ZST_DO_EXPAND
#undef ZST_EXPAND


// forward declare the zpr namespace...
namespace zpr
{
	// as well as the print_formatter.
	template <typename, typename>
	struct print_formatter;
}

namespace zst
{
	// we need a forward-declaration of error_and_exit for expect().
	[[noreturn]] void error_and_exit(const char* str, size_t n);

	// we really just need a very small subset of <type_traits>, so don't include the whole thing.
	namespace detail
	{
		template <typename T> T min(T a, T b) { return a < b ? a : b;}

		template <typename, typename>   struct is_same { static constexpr bool value = false; };
		template <typename T>           struct is_same<T, T> { static constexpr bool value = true; };


		template <bool B, typename T = void>    struct enable_if { };
		template <typename T>                   struct enable_if<true, T> { using type = T; };

		struct true_type  { static constexpr bool value = true; };
		struct false_type { static constexpr bool value = false; };

		template <typename T, T v>
		struct integral_constant
		{
			static constexpr T value = v;
			using value_type = T;
			using type = integral_constant; // using injected-class-name
			constexpr operator value_type() const noexcept { return value; }
			constexpr value_type operator()() const noexcept { return value; }
		};

		template <typename B> true_type  test_pre_ptr_convertible(const volatile B*);
		template <typename>   false_type test_pre_ptr_convertible(const volatile void*);

		template <typename, typename>
		auto test_pre_is_base_of(...) -> true_type;

		template <typename B, typename D>
		auto test_pre_is_base_of(int) -> decltype(test_pre_ptr_convertible<B>(static_cast<D*>(nullptr)));

		template <typename Base, typename Derived>
		struct is_base_of : integral_constant<bool, __is_class(Base) && __is_class(Derived)
			&& decltype(test_pre_is_base_of<Base, Derived>(0))::value
		>
		{ };

		template <typename T>
		struct __stop_declval_eval { static constexpr bool __stop = false; };

		template <typename T, typename U = T&&>
		U __declval(int);

		template <typename T>
		T __declval(long);

		template <typename T>
		auto declval() -> decltype(__declval<T>(0))
		{
			static_assert(__stop_declval_eval<T>::__stop, "declval() must not be used!");
			return __stop_declval_eval<T>::__unknown();
		}
	}
}


namespace zst
{
	namespace impl
	{
		template <typename CharType>
		struct str_view
		{
			using value_type = CharType;

			constexpr str_view() : ptr(nullptr), len(0) { }
			constexpr str_view(const value_type* p, size_t l) : ptr(p), len(l) { }

			template <size_t N>
			constexpr str_view(const value_type (&s)[N]) : ptr(s), len(N - 1) { }

			template <typename T, typename = typename detail::enable_if<
				detail::is_same<char, value_type>::value &&
				detail::is_same<const char*, T>::value
			>::type>
			constexpr str_view(T s) : ptr(s), len(strlen(s)) { }

			constexpr str_view(str_view&&) = default;
			constexpr str_view(const str_view&) = default;
			constexpr str_view& operator= (str_view&&) = default;
			constexpr str_view& operator= (const str_view&) = default;

			constexpr inline bool operator== (const str_view& other) const
			{
				return (this->len == other.len) &&
					(this->ptr == other.ptr || (memcmp(this->ptr, other.ptr, detail::min(this->len, other.len)) == 0)
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

			inline str_view<uint8_t> bytes() const { return str_view<uint8_t>(reinterpret_cast<const uint8_t*>(this->ptr), this->len); }

			value_type front() const { return this->ptr[0]; }
			value_type back() const { return this->ptr[this->len - 1]; }

			inline void clear() { this->ptr = 0; this->len = 0; }

			inline value_type operator[] (size_t n) const { return this->ptr[n]; }

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

			inline size_t find_first_of(str_view sv)
			{
				for(size_t i = 0; i < this->len; i++)
				{
					for(value_type k : sv)
					{
						if(this->ptr[i] == k)
							return i;
					}
				}

				return static_cast<size_t>(-1);
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
				else
					assert(other.ptr + other.len == this->ptr);

				n = detail::min(n, this->len);
				other.len += n;

				return this->remove_prefix(n);
			}


		#if ZST_USE_STD
			str_view(const std::basic_string<value_type>& str) : ptr(str.data()), len(str.size()) { }
			str_view(const std::basic_string_view<value_type>& sv) : ptr(sv.data()), len(sv.size()) { }

			inline std::basic_string_view<value_type> sv() const
			{
				return std::basic_string_view<value_type>(this->data(), this->size());
			}

			inline std::basic_string<value_type> str() const
			{
				return std::basic_string<value_type>(this->data(), this->size());
			}
		#endif // ZST_USE_STD


			// this must appear inside the class body... see the comment below about these leaky macros.
		#if defined(ZPR_USE_STD) || defined(ZPR_FREESTANDING)

			template <typename T = value_type, typename = typename detail::enable_if<detail::is_same<T, char>::value>::type>
			operator zpr::tt::str_view() const
			{
				return zpr::tt::str_view(this->ptr, this->len);
			}

		#endif

			// a little hacky, but a useful feature.
			template <typename A = value_type, typename = typename detail::enable_if<detail::is_same<A, uint8_t>::value>::type>
			str_view<char> chars() const
			{
				return str_view<char>(reinterpret_cast<const char*>(this->ptr), this->len);
			}

			template <typename U>
			str_view<U> cast() const
			{
				static_assert(sizeof(value_type) % sizeof(U) == 0 || sizeof(U) % sizeof(value_type) == 0, "types are not castable");
				return str_view<U>(reinterpret_cast<const U*>(this->ptr), (sizeof(value_type) * this->len) / sizeof(U));
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
		#endif // ZST_USE_STD

		template <typename C> inline const C* begin(const str_view<C>& sv) { return sv.begin(); }
		template <typename C> inline const C* end(const str_view<C>& sv) { return sv.end(); }
	}

	using str_view = impl::str_view<char>;
	using wstr_view = impl::str_view<char32_t>;

	template <typename T>
	using span = impl::str_view<T>;

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

			inline const value_type* begin() const { return this->ptr; }
			inline const value_type* end() const { return this->ptr + this->len; }

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

			template <typename T, typename A = value_type, typename = typename detail::enable_if<detail::is_same<A, uint8_t>::value>::type>
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
	namespace impl
	{
		template <typename... Args>
		[[noreturn]] void error_wrapper(const char* fmt, Args&&... args);
	}

	template <typename, typename>
	struct Result;

	namespace detail
	{
		template <typename T>
		struct is_result : detail::false_type { };

		template <typename T, typename U>
		struct is_result<Result<T, U>> : detail::true_type { };
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
		Result(Ok<T1>&& ok) : Result(tag_ok(), static_cast<T1&&>(ok.m_value)) { }

		template <typename E1 = E>
		Result(Err<E1>&& err) : Result(tag_err(), static_cast<E1&&>(err.m_error)) { }

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

		operator bool() const { return this->state == STATE_VAL; }
		bool ok() const { return this->state == STATE_VAL; }
		bool is_err() const { return this->state == STATE_ERR; }

		const T& unwrap() const { this->assert_has_value(); return this->val; }
		const E& error() const { this->assert_is_error(); return this->err; }

		T& unwrap() { this->assert_has_value(); return this->val; }
		E& error() { this->assert_is_error(); return this->err; }

		// enable implicit upcast to a base type
		template <typename U, typename = std::enable_if_t<
			std::is_pointer_v<T> && std::is_pointer_v<U>
			&& std::is_base_of_v<std::remove_pointer_t<U>, std::remove_pointer_t<T>>
		>>
		operator Result<U, E> () const
		{
			using R = Result<U, E>;
			if(state == STATE_VAL)  return R(typename R::tag_ok{}, this->val);
			if(state == STATE_ERR)  return R(typename R::tag_err{}, this->err);

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
		auto map(Fn&& fn) const -> Result<decltype(fn(detail::declval<T>())), E>
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
		auto flatmap(Fn&& fn) const -> decltype(fn(detail::declval<T>()))
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
		Result(Err<E1>&& err) : state(STATE_ERR), err(static_cast<E1&&>(err.m_error)) { }

		Result(tag_ok _) : state(STATE_VAL) { }
		Result(tag_err _, const E& err) : state(STATE_ERR), err(err) { }


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

		operator bool() const { return this->state == STATE_VAL; }
		bool ok() const { return this->state == STATE_VAL; }
		bool is_err() const { return this->state == STATE_ERR; }

		const E& error() const { this->assert_is_error(); return this->err; }
		E& error() { this->assert_is_error(); return this->err; }

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
}




// we rely on zpr.h defining some macros that "leak" globally. zpr.h always "force-defines" these macros,
// so we can be sure that if they are defined, then zpr.h is included.

#if defined(ZPR_USE_STD) || defined(ZPR_FREESTANDING)

namespace zst
{
	template <typename... Args>
	Err<std::string> ErrFmt(const char* fmt, Args&&... args)
	{
		return Err<std::string>(zpr::sprint(fmt, static_cast<Args&&>(args)...));
	}

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
	struct print_formatter<zst::Ok<T>, typename zst::detail::enable_if<detail::has_formatter_v<T>>::type>
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
	struct print_formatter<zst::Err<E>, typename zst::detail::enable_if<detail::has_formatter_v<E>>::type>
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
		typename zst::detail::enable_if<detail::has_formatter_v<T> && detail::has_formatter_v<E>>::type
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
	[[noreturn]] void error_wrapper(const char* fmt, Args&&... args)
	{
		constexpr const char msg[] = "internal error (no zpr, cannot elaborate)";
		zst::error_and_exit(msg, sizeof(msg) - 1);
	}
}

#endif



/*
	Version History
	===============

	1.4.2 - 12/08/2022
	------------------
	- fix wrong placement of const in `map` and `flatmap`
	- add `add_value` function to Result<void>
	- add `remove_value` function to Result<T>
	- add Failable type alias


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
