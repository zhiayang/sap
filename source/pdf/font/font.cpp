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


	font::GlyphMetrics Font::getMetricsForGlyph(GlyphId glyph) const
	{
		if(!this->source_file || this->glyph_metrics.find(glyph) != this->glyph_metrics.end())
			return { };

		return (this->glyph_metrics[glyph] = this->source_file->getGlyphMetrics(glyph));
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
			this->getMetricsForGlyph(gid);

			if(gid == GlyphId::notdef)
				zpr::println("warning: glyph for codepoint U+{04x} not found in font", codepoint);

			return gid;
		}
		else
		{
			pdf::error("unsupported encoding");
		}
	}

	std::map<size_t, font::GlyphAdjustment> Font::getPositioningAdjustmentsForGlyphSequence(
		zst::span<GlyphId> glyphs, const font::off::FeatureSet& features) const
	{
		if(!this->source_file)
			return {};

		return font::off::getPositioningAdjustmentsForGlyphSequence(this->source_file, glyphs, features);
	}

	font::off::SubstitutedGlyphString Font::performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
		const font::off::FeatureSet& features) const
	{
		if(!this->source_file)
		{
			font::off::SubstitutedGlyphString result {};
			result.glyphs = std::vector<GlyphId>(glyphs.begin(), glyphs.end());
			return result;
		}

		return font::off::performSubstitutionsForGlyphSequence(this->source_file, glyphs, features);
	}
}
