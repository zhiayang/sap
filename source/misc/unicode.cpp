// unicode.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <string_view>

#include "util.h"
#include "error.h"

#include <utf8proc/utf8proc.h>

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

			cp = (high << 10) | (low << 0);
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
			uint16_t fst = ((uint16_t) bytes[i + 0] << 8)
						 | ((uint16_t) bytes[i + 1] << 0);

			uint16_t snd = i + 3 < bytes.size()
				? ((uint16_t) bytes[i + 2] << 8) | ((uint16_t) bytes[i + 3] << 0)
				: 0;

			bool surrogate = convert_one_utf16(ret, fst, snd);
			if(surrogate)
				i += 2;
		}

		return ret;
	}

	uint32_t codepointFromUtf8(zst::byte_span& utf8)
	{
		int32_t codepoint = 0;
		auto read = utf8proc_iterate(utf8.data(), utf8.size(), &codepoint);
		if(codepoint == -1)
			sap::internal_error("utf8 conversion error");

		utf8.remove_prefix(read);
		return static_cast<uint32_t>(codepoint);
	}
}
