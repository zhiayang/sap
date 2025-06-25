// parser.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include <charconv>
#include <utf8proc/utf8proc.h>

#include "sap/config.h"

namespace sap::config
{
	static bool skip_one_ws(zst::str_view& sv)
	{
		if(sv.empty())
			return false;

		bool ret = false;
		while(not sv.empty() && util::is_one_of(sv[0], ' ', '\t', '\n', '\r'))
			ret = true, sv.remove_prefix(1);

		if(not sv.empty() && sv[0] == '#')
			ret = true, sv = sv.drop_until('\n');

		return ret;
	}

	static void skip_whitespace_and_comments(zst::str_view& sv)
	{
		while(skip_one_ws(sv))
			;
	}

	static void skip_line_space(zst::str_view& sv)
	{
		while(not sv.empty() && util::is_one_of(sv[0], ' ', '\t'))
			sv.remove_prefix(1);
	}



	static StrErrorOr<void> expect(zst::str_view& sv, zst::str_view kw)
	{
		skip_whitespace_and_comments(sv);
		if(not sv.starts_with(kw))
			return ErrFmt("expected '{}', got '{}'", kw, sv.take(kw.size()));

		sv.remove_prefix(kw.size());
		return Ok();
	}

	static zst::str_view consume_token(zst::str_view& sv)
	{
		skip_whitespace_and_comments(sv);
		return sv.take_prefix(sv.find_first_of(" \t\r\n"));
	}

	static zst::str_view peek_token(zst::str_view& sv)
	{
		skip_whitespace_and_comments(sv);
		return sv.take(sv.find_first_of(" \t\r\n"));
	}

	static zst::str_view consume_line(zst::str_view& sv)
	{
		skip_line_space(sv);
		size_t i = 0;
		while((i = sv.find_first_of("#\r\n", i)) != std::string::npos)
		{
			if(sv[i] == '#')
			{
				if(i == 0 || sv[i - 1] != '\\')
					break;

				i += 1;
			}
			else
			{
				break;
			}
		}

		// trim the line of whitespace
		auto line = sv.take_prefix(i);
		while(line.ends_with(' ') || line.ends_with('\t'))
			line.remove_suffix(1);

		return line;
	}


	static std::vector<zst::str_view> split_by_whitespace(zst::str_view sv)
	{
		std::vector<zst::str_view> ret {};
		while(not sv.empty())
		{
			auto x = sv.take_prefix(sv.find_first_of(" \t"));
			ret.push_back(x.trim_whitespace());

			sv = sv.trim_whitespace();
		}

		return ret;
	}

	static StrErrorOr<char32_t> parse_charcode(zst::str_view sv)
	{
		assert(not sv.empty());

		static auto parse_hex = [](zst::str_view s, size_t num) -> StrErrorOr<uint32_t> {
			uint32_t hex = 0;
			for(size_t i = 0; i < num; i++)
			{
				char ch = s[i];
				if(not(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F')))
					return ErrFmt("invalid hex digit '{}'", ch);

				hex = (0x10 * hex)
				    + uint32_t('0' <= ch && ch <= '9'
				                   ? ch - '0'
				                   : ('a' <= ch && ch <= 'f' //
				                             ? 10 + ch - 'a'
				                             : 10 + ch - 'A'));
			}

			return Ok(hex);
		};

		if(sv[0] == '\\')
		{
			if(sv.size() < 2 || not util::is_one_of(sv[1], '#', '\\', 'u', 'U'))
				return ErrFmt("expected one of '#', '\\', or 'uXXXX' after '\\'");

			if(sv[1] == '\\')
			{
				return Ok(U'\\');
			}
			else if(sv[1] == '#')
			{
				return Ok(U'#');
			}
			else if(sv[1] == 'u')
			{
				sv.remove_prefix(2);
				if(sv.size() < 4)
					return ErrFmt("expected 4 hex digits after '\\u'");

				return Ok(char32_t(TRY(parse_hex(sv, 4))));
			}
			else
			{
				assert(sv[1] == 'U');
				sv.remove_prefix(2);
				if(sv.size() < 8)
					return ErrFmt("expected 8 hex digits after '\\U'");

				return Ok(char32_t(TRY(parse_hex(sv, 8))));
			}
		}
		else
		{
			auto bytes = sv.bytes();
			std::vector<int32_t> codepoints {};

			while(not bytes.empty())
				codepoints.push_back(static_cast<int32_t>(unicode::consumeCodepointFromUtf8(bytes)));

			auto len = utf8proc_normalize_utf32(codepoints.data(), static_cast<utf8proc_ssize_t>(codepoints.size()),
			    UTF8PROC_COMPOSE);

			if(len < 0)
				return ErrFmt("failed to parse unicode: {}", utf8proc_errmsg(len));

			if(len != 1)
				return ErrFmt("too many codepoints (expected one, got {})", len);

			return Ok(static_cast<char32_t>(codepoints[0]));
		}
	}

	static StrErrorOr<double> parse_number(zst::str_view sv)
	{
		auto copy = sv.str();

		char* end_ptr = 0;
		auto ret = strtod(copy.c_str(), &end_ptr);

		if(end_ptr != copy.c_str() + copy.size())
			return ErrFmt("garbage at end of number: '{}'", end_ptr);

		return Ok(ret);
	}




	static StrErrorOr<void> parse_protrusion(zst::str_view& sv,
	    util::hashmap<char32_t, CharacterProtrusion>& protrusions)
	{
		auto line = consume_line(sv);
		auto parts = split_by_whitespace(line);

		if(parts.size() < 3)
			return ErrFmt("protrusion line format: <CHAR> <LEFT> <RIGHT> [aliases]...");

		assert(not parts[0].empty() && not parts[1].empty() && not parts[2].empty());

		auto char_code = TRY(parse_charcode(parts[0]));
		auto left = TRY(parse_number(parts[1]));
		auto right = TRY(parse_number(parts[2]));

		protrusions[char_code] = { left, right };
		for(size_t i = 3; i < parts.size(); i++)
			protrusions[TRY(parse_charcode(parts[i]))] = { left, right };

		return Ok();
	}

	StrErrorOr<MicrotypeConfig> parseMicrotypeConfig(zst::str_view& sv)
	{
		MicrotypeConfig config {};

		TRY(expect(sv, "microtype"));
		TRY(expect(sv, "match"));
		TRY(expect(sv, "["));

		while(not sv.empty() && peek_token(sv) != "]")
			config.matched_fonts.insert(consume_token(sv).str());

		TRY(expect(sv, "]"));
		TRY(expect(sv, "begin"));

		while(not sv.empty() && peek_token(sv) != "end")
		{
			auto section = consume_token(sv);
			if(section == "protrusions")
			{
				TRY(expect(sv, "begin"));

				while(not sv.empty() && peek_token(sv) != "end")
					TRY(parse_protrusion(sv, config.protrusions));

				TRY(expect(sv, "end"));
			}
			else if(section == "enable_if_feature")
			{
				TRY(expect(sv, "["));

				util::hashset<std::string> features {};
				while(not sv.empty() && peek_token(sv) != "]")
				{
					auto tok = consume_token(sv);
					if(tok.ends_with(']'))
						tok.transfer_suffix(sv, 1);

					features.insert(tok.str());
				}

				config.required_features.push_back(std::move(features));
				TRY(expect(sv, "]"));
			}
			else if(section == "enable_if_italic")
			{
				config.enable_if_italic = true;
			}
			else
			{
				return ErrFmt("unexpected subsection '{}'", section);
			}
		}

		TRY(expect(sv, "end"));

		// consume trailing whitespace to prepare for the next.
		skip_whitespace_and_comments(sv);

		return Ok(config);
	}
}
