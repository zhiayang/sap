// hyph.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <utf8proc/utf8proc.h>

#include "misc/hyphenator.h"

namespace sap::hyph
{
	template <typename Callable>
	static void iterateLines(Callable on_line, zst::wstr_view contents)
	{
		auto line_it = contents.begin();
		auto it = contents.begin();
		size_t len = 0;
		for(auto c : contents)
		{
			++it;
			if(c == '\n')
			{
				on_line(zst::wstr_view(line_it, len));
				line_it = it;
				len = 0;
			}
			else
			{
				++len;
			}
		}
		if(len > 0)
		{
			on_line(zst::wstr_view(line_it, len));
		}
	}

	Hyphenator::Pats Hyphenator::Pats::parse(zst::wstr_view contents)
	{
		Pats ret;

		auto parse_pat = [](zst::wstr_view line) {
			size_t num_chars = 0;
			util::ShortString<char32_t> pat {};

			HyphenationPoints hyphenation_points = { 0 };
			for(char32_t c : line)
			{
				if(c == U'.')
				{
					continue;
				}
				else if(U'0' <= c && c <= U'9')
				{
					if(num_chars >= 16)
					{
						sap::internal_error(
						    "hyphenation point is too far right, change code to handle bigger "
						    "patterns");
					}
					hyphenation_points[num_chars] = (uint8_t) (c - '0');
				}
				else
				{
					if(num_chars >= 16)
						sap::internal_error("pattern string is too long, change code to handle bigger patterns");

					pat.s[num_chars] = c;
					++num_chars;
				}
			}
			return std::make_pair(pat, hyphenation_points);
		};

		auto on_line = [&](zst::wstr_view line) {
			auto pats_ptr = &ret.m_mid_pats;
			if(line.front() == U'.')
				pats_ptr = &ret.m_front_pats;

			else if(line.back() == U'.')
				pats_ptr = &ret.m_front_pats;

			pats_ptr->emplace(parse_pat(line));
		};

		iterateLines(on_line, contents);

		return ret;
	}

	void Hyphenator::parseAndAddExceptions(zst::wstr_view contents)
	{
		auto on_line = [&](zst::wstr_view line) {
			std::vector<uint8_t> points;
			std::u32string word;

			for(char32_t c : line)
			{
				if(c == U'-')
				{
					points.push_back(5);
				}
				else
				{
					word.push_back(c);
					while(points.size() < word.size())
						points.push_back(0);
				}
			}

			if(points.size() <= word.size())
				points.push_back(0);

			m_hyphenation_cache.emplace(word, points);
		};

		iterateLines(on_line, contents);
	}

	Hyphenator Hyphenator::parseFromFile(const std::string& path)
	{
		auto [file, size] = util::readEntireFile(path);

		auto u32_string = unicode::u32StringFromUtf8({ file.get(), size });
		auto u32_contents = zst::wstr_view(u32_string);

		auto pats_start = u32_contents.find(U"\\patterns{");
		pats_start += u32_contents.drop(pats_start).find(U'\n');

		auto pats_size = u32_contents.drop(pats_start).find(U'}');

		auto exceptions_start = u32_contents.find(U"\\hyphenation{");
		exceptions_start += u32_contents.drop(pats_start).find(U'\n');

		auto exceptions_size = u32_contents.drop(exceptions_start).find(U'}');

		auto hyph = Hyphenator(Pats::parse(u32_contents.substr(pats_start, pats_size)));
		hyph.parseAndAddExceptions(u32_contents.substr(exceptions_start, exceptions_size));

		return hyph;
	}

	const std::vector<uint8_t>& Hyphenator::computeHyphenationPoints(zst::wstr_view word)
	{
		if(auto it = m_hyphenation_cache.find(word); it != m_hyphenation_cache.end())
			return it->second;

		auto ret = std::vector<uint8_t>(word.size() + 1, (uint8_t) 0);

		std::u32string lowercased;
		lowercased.reserve(word.size());

		for(size_t i = 0; i < word.size(); i++)
			lowercased.push_back((char32_t) utf8proc_tolower((int32_t) word[i]));

		for(size_t j = 1; j <= 16 && j < lowercased.size(); ++j)
		{
			auto snip = util::ShortString<char32_t>(lowercased.data(), j);
			if(auto hit = m_pats.m_front_pats.find(snip); hit != m_pats.m_front_pats.end())
			{
				const auto& points = hit->second;
				for(size_t k = 0; k < points.size(); ++k)
				{
					if(points[k] != 0)
						ret[k] = std::max(ret[k], points[k]);
				}
			}
		}

		for(size_t i = 0; i < lowercased.size() - 1; ++i)
		{
			for(size_t j = 1; j <= 16 && i + j < lowercased.size(); ++j)
			{
				auto snip = util::ShortString<char32_t>(lowercased.data() + i, j);

				if(auto hit = m_pats.m_mid_pats.find(snip); hit != m_pats.m_mid_pats.end())
				{
					const auto& points = hit->second;
					for(size_t k = 0; k < points.size(); ++k)
					{
						if(points[k] != 0)
							ret[i + k] = std::max(ret[i + k], points[k]);
					}
				}
			}
		}

		for(size_t j = 1; j <= 16 && j < lowercased.size(); ++j)
		{
			auto start_idx = lowercased.size() - j;
			auto snip = util::ShortString<char32_t>(lowercased.data() + start_idx, j);

			if(auto hit = m_pats.m_back_pats.find(snip); hit != m_pats.m_back_pats.end())
			{
				const auto& points = hit->second;
				for(size_t k = 0; k < points.size(); ++k)
				{
					if(points[k] != 0)
						ret[start_idx + k] = std::max(ret[start_idx + k], points[k]);
				}
			}
		}

		auto res = m_hyphenation_cache.emplace(std::move(lowercased), std::move(ret));
		return res.first->second;
	}
}
