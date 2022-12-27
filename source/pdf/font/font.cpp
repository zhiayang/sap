// font.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId, GlyphId::notdef

#include "pdf/font.h"              // for Font, Font::ENCODING_CID, Font::E...
#include "pdf/misc.h"              // for error
#include "pdf/win_ansi_encoding.h" // for WIN_ANSI

#include "font/font.h"     // for CharacterMapping, GlyphMetrics
#include "font/features.h" // for SubstitutedGlyphString, GlyphAdju...

namespace pdf
{
	std::string Font::getFontResourceName() const
	{
		return this->font_resource_name;
	}


	font::FontMetrics Font::getFontMetrics() const
	{
		// TODO: metrics for the 14 built-in fonts
		// see https://stackoverflow.com/questions/6383511/
		assert(this->source_file);

		return this->source_file->metrics;
	}

	Size2d_YDown Font::getWordSize(zst::wstr_view text, Scalar font_size) const
	{
		Size2d_YDown size;
		size.y() = this->scaleMetricForFontSize(source_file->metrics.default_line_spacing, font_size)
		               .into<dim::units::pdf_typographic_unit_y_down>();
		auto glyphs = this->getGlyphInfosForString(text);
		for(auto& g : glyphs)
		{
			auto tmp = g.metrics.horz_advance + g.adjustments.horz_advance;
			size.x() += this->scaleMetricForFontSize(tmp, font_size.into<pdf::Scalar>())
			                .into<dim::units::pdf_typographic_unit_y_down>();
		}
		return size;
	}

	void Font::markGlyphAsUsed(GlyphId glyph) const
	{
		m_used_glyphs.insert(glyph);
	}


	font::GlyphMetrics Font::getMetricsForGlyph(GlyphId glyph) const
	{
		this->markGlyphAsUsed(glyph);

		if(!this->source_file)
			return {};

		else if(auto it = m_glyph_metrics.find(glyph); it != m_glyph_metrics.end())
			return it->second;

		auto metrics = this->source_file->getGlyphMetrics(glyph);
		m_glyph_metrics[glyph] = metrics;

		return metrics;
	}

	GlyphId Font::getGlyphIdFromCodepoint(char32_t codepoint) const
	{
		if(this->encoding_kind == ENCODING_WIN_ANSI)
		{
			return GlyphId { encoding::WIN_ANSI(codepoint) };
		}
		else if(this->encoding_kind == ENCODING_CID)
		{
			assert(this->source_file != nullptr);

			// also pre-load the metrics (because in all likelihood we'll need the metrics soon)
			auto gid = this->source_file->getGlyphIndexForCodepoint(codepoint);
			this->markGlyphAsUsed(gid);

			if(gid == GlyphId::notdef)
				sap::warn("font", "glyph for codepoint U+{04x} not found in font", codepoint);

			return gid;
		}
		else
		{
			pdf::error("unsupported encoding");
		}
	}

	const std::vector<font::GlyphInfo>& Font::getGlyphInfosForString(zst::wstr_view text) const
	{
		if(auto it = m_glyph_infos_cache.find(text); it != m_glyph_infos_cache.end())
		{
			return it->second;
		}

		// first, convert all codepoints to glyphs
		std::vector<GlyphId> glyphs {};
		for(char32_t cp : text)
			glyphs.push_back(this->getGlyphIdFromCodepoint(cp));

		using font::Tag;
		font::off::FeatureSet features {};

		// REMOVE: this is just for testing!
		features.script = Tag("cyrl");
		features.language = Tag("BGR ");
		features.enabled_features = { Tag("kern"), Tag("liga"), Tag("locl") };

		auto span = [](auto& foo) {
			return zst::span<GlyphId>(foo.data(), foo.size());
		};

		// next, use GSUB to perform substitutions.
		glyphs = this->performSubstitutionsForGlyphSequence(span(glyphs), features);

		// next, get base metrics for each glyph.
		std::vector<font::GlyphInfo> glyph_infos {};
		for(auto g : glyphs)
		{
			font::GlyphInfo info {};
			info.gid = g;
			info.metrics = this->getMetricsForGlyph(g);
			glyph_infos.push_back(std::move(info));
		}

		// finally, use GPOS
		auto adjustment_map = this->getPositioningAdjustmentsForGlyphSequence(span(glyphs), features);
		for(auto& [i, adj] : adjustment_map)
		{
			auto& info = glyph_infos[i];
			info.adjustments.horz_advance += adj.horz_advance;
			info.adjustments.vert_advance += adj.vert_advance;
			info.adjustments.horz_placement += adj.horz_placement;
			info.adjustments.vert_placement += adj.vert_placement;
		}

		auto res = m_glyph_infos_cache.emplace(text.str(), std::move(glyph_infos));

		return res.first->second;
	}

	void Font::addGlyphUnicodeMapping(GlyphId glyph, std::vector<char32_t> codepoints) const
	{
		if(auto it = m_extra_unicode_mappings.find(glyph); it != m_extra_unicode_mappings.end() && it->second != codepoints)
		{
			sap::warn("font", "conflicting unicode mapping for glyph '{}' (existing: {}, new: {})", glyph, it->second,
			    codepoints);
		}

		m_extra_unicode_mappings[glyph] = std::move(codepoints);
	}


	std::map<size_t, font::GlyphAdjustment> Font::getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const font::off::FeatureSet& features) const
	{
		if(!this->source_file)
			return {};

		return font::off::getPositioningAdjustmentsForGlyphSequence(this->source_file, glyphs, features);
	}

	std::vector<GlyphId> Font::performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const font::off::FeatureSet& features) const
	{
		if(!this->source_file)
			return std::vector<GlyphId>(glyphs.begin(), glyphs.end());

		auto subst = font::off::performSubstitutionsForGlyphSequence(this->source_file, glyphs, features);

		auto& cmap = this->source_file->character_mapping;
		for(auto& [out, in] : subst.mapping.replacements)
		{
			this->markGlyphAsUsed(out);

			// if the out is mapped, then we actually don't need to do anything special
			if(auto m = cmap.reverse.find(out); m == cmap.reverse.end())
			{
				// get the codepoint for the input
				if(auto it = cmap.reverse.find(in); it == cmap.reverse.end())
				{
					sap::warn("font/off", "could not find unicode codepoint for {}", in);
					continue;
				}
				else
				{
					auto in_cp = it->second;
					this->addGlyphUnicodeMapping(out, { in_cp });
				}
			}
		}

		auto find_codepoint_for_gid = [&cmap, this](GlyphId gid) -> std::vector<char32_t> {
			// if it is in the reverse cmap, all is well. if it is not, then we hope that
			// it was a single-replacement (eg. a ligature of replaced glyphs). otherwise,
			// that's a big oof.
			if(auto x = cmap.reverse.find(gid); x != cmap.reverse.end())
			{
				return { x->second };
			}
			else if(auto x = m_extra_unicode_mappings.find(gid); x != m_extra_unicode_mappings.end())
			{
				return x->second;
			}
			else
			{
				sap::warn("font", "could not find unicode codepoint for {}", gid);
				return { U'?' };
			}
		};

		for(auto& [out, ins] : subst.mapping.contractions)
		{
			this->markGlyphAsUsed(out);
			std::vector<char32_t> in_cps {};
			for(auto& in_gid : ins)
			{
				auto tmp = find_codepoint_for_gid(in_gid);
				in_cps.insert(in_cps.end(), tmp.begin(), tmp.end());
			}

			this->addGlyphUnicodeMapping(out, std::move(in_cps));
		}

		for(auto& g : subst.mapping.extra_glyphs)
		{
			this->markGlyphAsUsed(g);

			if(cmap.reverse.find(g) == cmap.reverse.end())
				this->addGlyphUnicodeMapping(g, find_codepoint_for_gid(g));
		}

		return subst.glyphs;
	}
}
