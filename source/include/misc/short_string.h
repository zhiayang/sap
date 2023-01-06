// short_string.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace util
{
	template <typename T>
	struct ShortString
	{
		using value_type = T;
		using iterator = T*;
		using const_iterator = const T*;

		T s[16] = { 0 };

		ShortString() = default;

		ShortString(const T* bytes, size_t len)
		{
			if(len > 16)
				sap::internal_error("short string not so short eh?");

			std::copy(bytes, bytes + len, s);
		}

		ShortString(zst::impl::str_view<T> sv)
		{
			if(sv.length() > 16)
				sap::internal_error("short string not so short eh?");

			std::copy(sv.begin(), sv.end(), s);
		}

		T* data() { return s; }
		const T* data() const { return s; }

		T* begin() { return s; }
		const T* begin() const { return s; }

		T* end() { return s + size(); }
		const T* end() const { return s + size(); }

		ShortString& operator+=(zst::impl::str_view<T> sv)
		{
			this->append(sv);
			return *this;
		}

		ShortString operator+(zst::impl::str_view<T> sv) const
		{
			ShortString copy = *this;
			copy.append(sv);
			return copy;
		}

		void append(zst::impl::str_view<T> sv)
		{
			if(size() + sv.size() > 16)
				sap::internal_error("short string would be too long");

			std::copy(sv.begin(), sv.end(), end());
		}

		void push_back(T ch)
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

		operator std::basic_string<T>() const { return std::basic_string<T>(s, this->size()); }
		operator zst::impl::str_view<T>() const { return zst::impl::str_view<T>(s, this->size()); }
		operator std::basic_string_view<T>() const { return std::basic_string_view<T>(s, this->size()); }

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

}
