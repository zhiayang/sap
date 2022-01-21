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



	void Font::markLigatureUsed(const font::GlyphLigature& lig) const
	{
		if(std::find(this->used_ligatures.begin(), this->used_ligatures.end(), lig) == this->used_ligatures.end())
			this->used_ligatures.push_back(lig);
	}

	std::optional<font::GlyphLigatureSet> Font::getLigaturesForGlyph(uint32_t glyph) const
	{
		if(auto it = this->glyph_ligatures.find(glyph); it != this->glyph_ligatures.end())
			return it->second;

		return { };
	}

	font::GlyphMetrics Font::getMetricsForGlyph(uint32_t glyph) const
	{
		this->loadMetricsForGlyph(glyph);
		return this->glyph_metrics[glyph];
	}

	void Font::loadMetricsForGlyph(uint32_t glyph) const
	{
		if(!this->source_file || this->glyph_metrics.find(glyph) != this->glyph_metrics.end())
			return;

		// get and cache the glyph widths as well.
		this->glyph_metrics[glyph] = this->source_file->getGlyphMetrics(glyph);
	}

	uint32_t Font::getGlyphIdFromCodepoint(uint32_t codepoint) const
	{
		if(this->encoding_kind == ENCODING_WIN_ANSI)
		{
			return encoding::WIN_ANSI(codepoint);
		}
		else if(this->encoding_kind == ENCODING_CID)
		{
			assert(this->source_file != nullptr);
			if(auto it = this->cmap_cache.find(codepoint); it != this->cmap_cache.end())
				return it->second;

			auto gid = this->source_file->getGlyphIndexForCodepoint(codepoint);

			this->cmap_cache[codepoint] = gid;
			this->reverse_cmap[gid] = codepoint;

			this->loadMetricsForGlyph(gid);

			if(gid == 0)
				zpr::println("warning: glyph for codepoint U+{04x} not found in font", codepoint);

			return gid;
		}
		else
		{
			pdf::error("unsupported encoding");
		}
	}

	std::optional<font::KerningPair> Font::getKerningForGlyphs(uint32_t glyph1, uint32_t glyph2) const
	{
		auto pair = std::make_pair(glyph1, glyph2);
		if(auto it = this->kerning_pairs.find(pair); it != this->kerning_pairs.end())
			return it->second;

		if(this->source_file != nullptr)
		{
			auto kp = this->source_file->getGlyphPairAdjustments(glyph1, glyph2);
			if(kp.has_value())
				return *kp;
		}

		return { };
	}

	std::map<size_t, font::GlyphAdjustment> Font::getPositioningAdjustmentsForGlyphSequence(
		zst::span<uint32_t> glyphs, const font::off::FeatureSet& features) const
	{
		if(!this->source_file)
			return {};

		return font::off::getPositioningAdjustmentsForGlyphSequence(this->source_file, glyphs, features);
	}
}
