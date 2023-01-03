#include "hyph.h"

namespace sap::hyph
{
	template <typename Callable>
	static void iterateLines(Callable on_line, zst::str_view contents)
	{
		auto line_it = contents.begin();
		auto it = contents.begin();
		size_t len = 0;
		for(char c : contents)
		{
			++it;
			if(c == '\n')
			{
				on_line(zst::str_view(line_it, len));
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
			on_line(zst::str_view(line_it, len));
		}
	}

	AsciiHyph::Pats AsciiHyph::Pats::parse(zst::str_view contents)
	{
		Pats ret;

		auto parse_pat = [](zst::str_view line) {
			size_t num_chars = 0;
			util::ShortString pat;
			HyphenationPoints hyphenation_points = { 0 };
			for(char c : line)
			{
				if(c == '.')
				{
					continue;
				}
				else if('0' <= c && c <= '9')
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
					{
						sap::internal_error("pattern string is too long, change code to handle bigger patterns");
					}
					pat.s[num_chars] = c;
					++num_chars;
				}
			}
			return std::make_pair(pat, hyphenation_points);
		};

		auto on_line = [&](zst::str_view line) {
			auto pats_ptr = &ret.m_mid_pats;
			if(line.front() == '.')
			{
				pats_ptr = &ret.m_front_pats;
			}
			else if(line.back() == '.')
			{
				pats_ptr = &ret.m_front_pats;
			}
			pats_ptr->emplace(parse_pat(line));
		};

		iterateLines(on_line, contents);

		return ret;
	}

	void AsciiHyph::parseAndAddExceptions(zst::str_view contents)
	{
		auto on_line = [&](zst::str_view line) {
			std::string word;
			std::vector<uint8_t> points;
			for(char c : line)
			{
				if(c == '-')
				{
					points.push_back(5);
				}
				else
				{
					word.push_back(c);
					while(points.size() < word.size())
					{
						points.push_back(0);
					}
				}
			}
			if(points.size() <= word.size())
			{
				points.push_back(0);
			}
			m_hyphenation_cache.emplace(word, points);
		};

		iterateLines(on_line, contents);
	}

	AsciiHyph AsciiHyph::parseFromFile(const std::string& path)
	{
		auto [ptr, size] = util::readEntireFile(path);
		auto contents = zst::str_view((char*) ptr, size);
		auto pats_start = contents.find("\\patterns{");
		pats_start += contents.substr(pats_start).find('\n');
		auto pats_size = contents.substr(pats_start).find('}');
		auto exceptions_start = contents.find("\\hyphenation{");
		exceptions_start += contents.substr(pats_start).find('\n');
		auto exceptions_size = contents.substr(exceptions_start).find('}');

		auto hyph = AsciiHyph(Pats::parse(contents.substr(pats_start, pats_size)));
		hyph.parseAndAddExceptions(contents.substr(exceptions_start, exceptions_size));
		return hyph;
	}

	const std::vector<uint8_t>& AsciiHyph::computeHyphenationPointsForLowercaseAlphabeticalWord(zst::str_view word)
	{
		if(auto it = m_hyphenation_cache.find(word); it != m_hyphenation_cache.end())
			return it->second;

		std::vector<uint8_t> s(word.size() + 1, (uint8_t) 0);

		for(size_t i = 0; i < word.size() - 1; ++i)
		{
			for(size_t j = 1; j <= 16 && i + j < word.size(); ++j)
			{
				auto snip = util::ShortString(word.begin() + i, j);
				if(auto hit = m_pats.m_mid_pats.find(snip); hit != m_pats.m_mid_pats.end())
				{
					const auto& hyphenation_points = hit->second;
					for(size_t k = 0; k < hyphenation_points.size(); ++k)
					{
						s[i + k] = std::max(s[i + k], hyphenation_points[k]);
					}
				}
			}
		}

		auto res = m_hyphenation_cache.emplace(word.str(), std::move(s));
		return res.first->second;
	}
}
