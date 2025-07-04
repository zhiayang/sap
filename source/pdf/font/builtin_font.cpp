// builtin_fonts.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include <libdeflate/libdeflate.h>

#include <array>
#include <charconv>

#include "pdf/builtin_font.h"

namespace pdf
{
	extern std::pair<const uint8_t*, size_t> get_compressed_afm(BuiltinFont::Core14 font);
	extern char32_t get_codepoint_for_glyph_name(zst::str_view name);

	static std::pair<uint8_t*, size_t> decompress_afm(std::pair<const uint8_t*, size_t> p)
	{
		auto [buf, len] = p;

		// we know that all the AFMs are at most 128kB (decompressed)
		constexpr size_t OUTPUT_BUF_SIZE = 128 * 1024;

		auto output = new uint8_t[OUTPUT_BUF_SIZE];

		auto dc = libdeflate_alloc_decompressor();

		size_t real_size = 0;
		auto result = libdeflate_zlib_decompress(dc, buf, len, output, OUTPUT_BUF_SIZE, &real_size);

		if(result != LIBDEFLATE_SUCCESS)
			sap::internal_error("failed to decompress afm!");

		libdeflate_free_decompressor(dc);
		return { output, real_size };
	}


	std::unique_ptr<BuiltinFont> BuiltinFont::get(Core14 font)
	{
		return std::unique_ptr<BuiltinFont>(new BuiltinFont(font));
	}

	static zst::str_view trim(zst::str_view sv)
	{
		if(sv.empty())
			return sv;

		while(util::is_one_of(sv[0], ' ', '\t', '\r', '\n'))
			sv.remove_prefix(1);

		while(util::is_one_of(sv.back(), ' ', '\t', '\r', '\n'))
			sv.remove_suffix(1);

		return sv;
	}

	static std::vector<zst::str_view> split_by(zst::str_view sv, char c)
	{
		std::vector<zst::str_view> ret {};
		while(not sv.empty())
		{
			auto x = trim(sv.take_prefix(sv.find(c)));
			ret.push_back(x);

			sv = trim(sv);
			if(sv.starts_with(c))
				sv.remove_prefix(1);
		}

		return ret;
	}

	template <typename T>
	static T to(zst::str_view sv)
	{
		if constexpr(std::is_integral_v<T>)
		{
			T ret = 0;
			(void) std::from_chars(sv.begin(), sv.end(), ret);
			return ret;
		}
		else
		{
			return std::stod(sv.str());
		}
	}


	std::optional<zst::str_view> get_value_for_key(zst::str_view line, zst::str_view key)
	{
		if(line.starts_with(key) && line.drop(key.size()).starts_with(" "))
			return trim(line.drop(key.size()));

		return std::nullopt;
	}

	BuiltinFont::BuiltinFont(Core14 which)
	{
		m_kind = which;

		using font::FontScalar;

		auto [b, l] = decompress_afm(get_compressed_afm(which));
		auto afm_file = zst::byte_span(b, l).chars();

		std::vector<zst::str_view> lines {};
		while(afm_file.size() > 0)
		{
			while(util::is_one_of(afm_file[0], ' ', '\t', '\r', '\n'))
				afm_file.remove_prefix(1);

			auto line = afm_file.take_prefix(afm_file.find_first_of("\r\n"));
			lines.push_back(trim(line));
		}

		if(not lines[0].starts_with("StartFontMetrics"))
			sap::internal_error("malformed AFM file!");

		m_metrics = font::FontMetrics {};
		m_first_glyph_id = GlyphId(0);
		m_last_glyph_id = GlyphId(0);

		size_t expected_glyphs = 0;
		size_t expected_kerns = 0;

		util::hashmap<std::string, GlyphId> name_to_gid_map {};

		// we need to wait till we get all the glyph names before we do ligatures
		// (because there might be forward references)
		std::map<std::pair<GlyphId, std::string>, std::string> ligature_pairs {};

		for(auto line : lines)
		{
			auto get_value = [&line](zst::str_view key) { return get_value_for_key(line, key); };

			if(auto x0 = get_value("FontName"); x0.has_value())
			{
				m_name = x0->str();
			}
			else if(auto x1 = get_value("FontBBox"); x1.has_value())
			{
				auto bbox = split_by(*x1, ' ');
				m_metrics.xmin = to<int>(bbox[0]);
				m_metrics.ymin = to<int>(bbox[1]);
				m_metrics.xmax = to<int>(bbox[2]);
				m_metrics.ymax = to<int>(bbox[3]);
			}
			else if(auto x2 = get_value("ItalicAngle"); x2.has_value())
			{
				m_metrics.italic_angle = to<double>(*x2);
			}
			else if(auto x3 = get_value("CapHeight"); x3.has_value())
			{
				m_metrics.cap_height = FontScalar(to<int>(*x3));
			}
			else if(auto x4 = get_value("XHeight"); x4.has_value())
			{
				m_metrics.x_height = FontScalar(to<int>(*x4));
			}
			else if(auto x5 = get_value("Ascender"); x5.has_value())
			{
				m_metrics.typo_ascent = FontScalar(to<int>(*x5));
				m_metrics.hhea_ascent = m_metrics.typo_ascent;
			}
			else if(auto x6 = get_value("Descender"); x6.has_value())
			{
				m_metrics.typo_descent = FontScalar(to<int>(*x6));
				m_metrics.hhea_descent = m_metrics.typo_descent;
			}
			else if(auto x7 = get_value("StdVW"); x7.has_value())
			{
				m_metrics.stem_v = to<int>(*x7);
			}
			else if(auto x8 = get_value("StdHW"); x8.has_value())
			{
				m_metrics.stem_h = to<int>(*x8);
			}
			else if(auto x9 = get_value("StartCharMetrics"); x9.has_value())
			{
				expected_glyphs = to<size_t>(*x9);
			}
			else if(auto x10 = get_value("StartKernPairs"); x10.has_value())
			{
				expected_kerns = to<size_t>(*x10);
			}
			else if(auto x11 = get_value("C"); x11.has_value() && expected_glyphs > 0)
			{
				expected_glyphs--;

				auto parts = split_by(*x11, ';');

				// if this is not encoded, we can't do anything about it, so just bail
				auto gid = to<int>(parts[0]);
				if(gid == -1)
					continue;

				zst::str_view glyph_name {};
				font::GlyphMetrics metrics {};
				for(auto part : parts)
				{
					if(auto t0 = get_value_for_key(part, "B"); t0.has_value())
					{
						auto bbox = split_by(*t0, ' ');
						metrics.xmin = FontScalar(to<int>(bbox[0]));
						metrics.ymin = FontScalar(to<int>(bbox[1]));
						metrics.xmax = FontScalar(to<int>(bbox[2]));
						metrics.ymax = FontScalar(to<int>(bbox[3]));
					}
					else if(auto t1 = get_value_for_key(part, "WX"); t1.has_value())
					{
						metrics.horz_advance = FontScalar(to<int>(*t1));
					}
					else if(auto t2 = get_value_for_key(part, "WY"); t2.has_value())
					{
						metrics.vert_advance = FontScalar(to<int>(*t2));
					}
					else if(auto t3 = get_value_for_key(part, "N"); t3.has_value())
					{
						glyph_name = *t3;
					}
					else if(auto t4 = get_value_for_key(part, "L"); t4.has_value())
					{
						auto lig_parts = split_by(*t4, ' ');
						assert(lig_parts.size() == 2);

						ligature_pairs[{ GlyphId(gid), lig_parts[0].str() }] = lig_parts[1].str();
					}
				}

				// make sure the glyph has a name
				assert(not glyph_name.empty());
				auto cp = get_codepoint_for_glyph_name(glyph_name);

				auto ggid = GlyphId(gid);
				m_character_mapping.forward[cp] = ggid;
				m_character_mapping.reverse[ggid] = cp;

				m_glyph_metrics[ggid] = std::move(metrics);

				m_first_glyph_id = std::min(ggid, m_first_glyph_id);
				m_last_glyph_id = std::max(ggid, m_last_glyph_id);

				name_to_gid_map[glyph_name.str()] = ggid;
			}
			else if(auto x12 = get_value("KPX"); x12.has_value() && expected_kerns > 0)
			{
				--expected_kerns;

				auto parts = split_by(*x12, ' ');
				assert(parts.size() == 3);

				GlyphId lgid;
				GlyphId rgid;

				if(auto it = name_to_gid_map.find(parts[0]); it != name_to_gid_map.end())
					lgid = it->second;
				else
					continue;

				if(auto it = name_to_gid_map.find(parts[1]); it != name_to_gid_map.end())
					rgid = it->second;
				else
					continue;

				m_kerning_pairs[{ lgid, rgid }] = font::GlyphAdjustment {
					.horz_advance = FontScalar(to<int>(parts[2])),
				};
			}
		}

		// do up the ligatures now that we know all the names
		for(auto& [p, lig_name] : ligature_pairs)
		{
			auto& [lgid, rname] = p;

			GlyphId rgid;
			if(auto it = name_to_gid_map.find(rname); it != name_to_gid_map.end())
				rgid = it->second;
			else
				continue;

			GlyphId liggid;
			if(auto it = name_to_gid_map.find(lig_name); it != name_to_gid_map.end())
				liggid = it->second;
			else
				continue;

			m_ligature_pairs[{ lgid, rgid }] = liggid;
		}


		// see the comment in font/loader.cpp about this 1.2x
		m_metrics.units_per_em = 1000;
		m_metrics.typo_linegap = 0;

		// i don't know how the heck this works
		m_metrics.default_line_spacing = m_metrics.typo_ascent - m_metrics.typo_descent;

		// m_metrics.default_line_spacing = std::max(FontScalar(m_metrics.units_per_em * 12) / 10,
		//     m_metrics.typo_ascent - m_metrics.typo_descent);

		// zpr::println("font = {}", m_name);
		// zpr::println("asc = {}", m_metrics.typo_ascent);
		// zpr::println("dsc = {}", m_metrics.typo_descent);
		// zpr::println("spacing = {}", m_metrics.default_line_spacing);

		delete[] b;
	}


	util::hashmap<size_t, font::GlyphAdjustment> BuiltinFont::getPositioningAdjustmentsForGlyphSequence( //
	    zst::span<GlyphId> glyphs,
	    const font::FeatureSet& features) const
	{
		if(glyphs.size() < 2)
			return {};

		util::hashmap<size_t, font::GlyphAdjustment> kerns {};
		for(size_t i = 0; i < glyphs.size() - 1; i++)
		{
			if(auto k = m_kerning_pairs.find({ glyphs[i], glyphs[i + 1] }); k != m_kerning_pairs.end())
				kerns[i] = k->second;
		}

		return kerns;
	}

	std::optional<font::SubstitutedGlyphString> BuiltinFont::
	    performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs, const font::FeatureSet& features) const
	{
		if(glyphs.size() < 2)
			return std::nullopt;

		std::optional<font::SubstitutedGlyphString> ret = std::nullopt;

		bool dont_add_last = false;
		for(size_t i = 0; i < glyphs.size() - 1; i++)
		{
			if(auto lig = m_ligature_pairs.find({ glyphs[i], glyphs[i + 1] }); lig != m_ligature_pairs.end())
			{
				if(not ret.has_value())
				{
					ret = font::SubstitutedGlyphString {};
					ret->glyphs.reserve(glyphs.size());

					// copy the old glyphs
					ret->glyphs.insert(ret->glyphs.end(), glyphs.begin(), glyphs.begin() + i);
				}

				// also skip the next glyph
				ret->glyphs.push_back(lig->second);
				ret->mapping.contractions[lig->second] = { glyphs[i], glyphs[i + 1] };
				i++;

				if(i + 1 == glyphs.size())
					dont_add_last = true;
			}
			else
			{
				if(ret.has_value())
					ret->glyphs.push_back(glyphs[i]);
			}
		}

		if(ret.has_value() && not dont_add_last)
			ret->glyphs.push_back(glyphs.back());

		return ret;
	}

	font::GlyphMetrics BuiltinFont::get_glyph_metrics_impl(GlyphId glyphid) const
	{
		return {};
	}
}
