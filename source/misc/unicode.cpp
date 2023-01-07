// unicode.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h> // for utf8proc_encode_char, utf8proc_iterate

#include "util.h" // for checked_cast, u32StringFromUtf8, code...

namespace unicode
{
	static bool convert_one_utf16(std::string& out, uint16_t fst, uint16_t snd)
	{
		bool surrogate = false;

		int32_t cp = 0;
		if(fst <= 0xD7FF || fst >= 0xE000)
		{
			cp = fst;
		}
		else
		{
			int32_t high = fst & 0x3FF;
			int32_t low = snd & 0x3FF;

			if(snd <= 0xD7FF || snd >= 0xE000)
				sap::internal_error("missing second surrogate in pair");

			cp = 0x10'0000 + (high << 10) | (low << 0);
			surrogate = true;
		}

		auto did = utf8proc_reencode(&cp, 1, (utf8proc_option_t) (UTF8PROC_STRIPCC | UTF8PROC_COMPOSE));
		if(did < 0)
			sap::internal_error("failed to convert codepoint to utf-8 (error = {})", (int) did);

		out += std::string_view(reinterpret_cast<const char*>(&cp), static_cast<size_t>(did));
		return surrogate;
	}

	std::string utf8FromUtf16BE(zst::span<uint16_t> us)
	{
		std::string ret;
		for(size_t i = 0; i < us.size(); i++)
		{
			bool surrogate = convert_one_utf16(ret, us[i], i + 1 == us.size() ? 0 : us[i + 1]);
			if(surrogate)
				i += 1;
		}

		return ret;
	}

	std::string utf8FromUtf16BigEndianBytes(zst::byte_span bytes)
	{
		std::string ret;
		for(size_t i = 0; i < bytes.size(); i += 2)
		{
			uint16_t fst = (uint16_t) (((uint16_t) bytes[i + 0] << 8) | ((uint16_t) bytes[i + 1] << 0));

			uint16_t
			    snd = i + 3 < bytes.size() ? (uint16_t) (((uint16_t) bytes[i + 2] << 8) | ((uint16_t) bytes[i + 3] << 0)) : 0;

			bool surrogate = convert_one_utf16(ret, fst, snd);
			if(surrogate)
				i += 2;
		}

		return ret;
	}

	char32_t consumeCodepointFromUtf8(zst::byte_span& utf8)
	{
		int32_t codepoint = 0;
		auto read = utf8proc_iterate(utf8.data(), util::checked_cast<ptrdiff_t>(utf8.size()), &codepoint);
		if(codepoint == -1)
			sap::internal_error("utf8 conversion error");

		utf8.remove_prefix(util::checked_cast<size_t>(read));
		return static_cast<char32_t>(codepoint);
	}

	std::string utf8FromCodepoint(char32_t cp)
	{
		uint8_t buf[4] {};
		auto n = utf8proc_encode_char(static_cast<int32_t>(cp), &buf[0]);

		return std::string(reinterpret_cast<char*>(&buf[0]), static_cast<size_t>(n));
	}

	std::pair<uint16_t, uint16_t> codepointToSurrogatePair(char32_t codepoint)
	{
		auto cp = static_cast<uint32_t>(codepoint);
		assert(cp <= 0x10FFFF);

		auto high = ((cp - 0x10'0000) & 0xFFC000) >> 10;
		auto low = ((cp - 0x10'0000) & 0x0003FF) >> 0;

		return { static_cast<uint16_t>(high + 0xD800), static_cast<uint16_t>(low + 0xDC00) };
	}

	std::u32string u32StringFromUtf8(zst::byte_span sv)
	{
		std::u32string str {};
		str.reserve(sv.size());

		while(sv.size() > 0)
			str += consumeCodepointFromUtf8(sv);

		return str;
	}

	std::u32string u32StringFromUtf8(std::string_view sv)
	{
		return u32StringFromUtf8(zst::byte_span((uint8_t*) sv.data(), sv.size()));
	}

	std::string stringFromU32String(std::u32string_view sv)
	{
		std::string s;
		for(auto c : sv)
		{
			s += utf8FromCodepoint(c);
		}
		return s;
	}
}
