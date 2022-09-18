// font.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>

#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"
#include "pdf/document.h"
#include "pdf/win_ansi_encoding.h"

#include "font/font.h"

namespace pdf
{
	std::string Font::getFontResourceName() const
	{
		return this->font_resource_name;
	}

	Scalar Font::scaleMetricForFontSize(double metric, Scalar font_size) const
	{
		return Scalar((metric * font_size.value()) / this->getFontMetrics().units_per_em);
	}

	Scalar Font::scaleMetricForPDFTextSpace(double metric) const
	{
		return Scalar((metric * GLYPH_SPACE_UNITS) / this->getFontMetrics().units_per_em);
	}

	font::FontMetrics Font::getFontMetrics() const
	{
		// TODO: metrics for the 14 built-in fonts
		// see https://stackoverflow.com/questions/6383511/
		assert(this->source_file);

		return this->source_file->metrics;
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

	GlyphId Font::getGlyphIdFromCodepoint(Codepoint codepoint) const
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

	void Font::addGlyphUnicodeMapping(GlyphId glyph, std::vector<Codepoint> codepoints) const
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

		auto find_codepoint_for_gid = [&cmap, this](GlyphId gid) -> std::vector<Codepoint> {
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
				return { Codepoint { '?' } };
			}
		};

		for(auto& [out, ins] : subst.mapping.contractions)
		{
			this->markGlyphAsUsed(out);
			std::vector<Codepoint> in_cps {};
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
