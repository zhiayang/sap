// font_file.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "font/cff.h"
#include "font/truetype.h"
#include "font/font_file.h"

namespace font
{
	FontFile::~FontFile()
	{
	}

	GlyphId FontSource::getGlyphIndexForCodepoint(char32_t codepoint) const
	{
		if(auto it = m_character_mapping.forward.find(codepoint); it != m_character_mapping.forward.end())
		{
			m_used_glyphs.insert(it->second);
			return it->second;
		}
		else
		{
			sap::warn("font", "glyph for codepoint U+{04x} not found in font", codepoint);
			return GlyphId::notdef;
		}
	}

	GlyphMetrics FontFile::get_glyph_metrics_impl(GlyphId glyph_id) const
	{
		this->markGlyphAsUsed(glyph_id);

		auto u16_array = m_hmtx_table.cast<uint16_t>();

		GlyphMetrics ret {};
		auto gid32 = static_cast<uint32_t>(glyph_id);

		// note the *2, because there's also an 2-byte "lsb".
		if(gid32 >= m_num_hmetrics)
		{
			// the remaining glyphs not in the array use the last value.
			ret.horz_advance = FontScalar(util::convertBEU16(u16_array[(m_num_hmetrics - 1) * 2]));

			// there is an array of lsbs for glyph_ids > num_hmetrics
			auto lsb_array = m_hmtx_table.drop(2 * m_num_hmetrics);
			auto tmp = lsb_array[gid32 - m_num_hmetrics];

			ret.left_side_bearing = FontScalar((int16_t) util::convertBEU16(tmp));
		}
		else
		{
			ret.horz_advance = FontScalar(util::convertBEU16(u16_array[gid32 * 2]));
			ret.left_side_bearing = FontScalar((int16_t) util::convertBEU16(u16_array[gid32 * 2 + 1]));
		}

		// now, figure out xmin and xmax
		if(this->hasTrueTypeOutlines())
		{
			auto bb = truetype::getGlyphBoundingBox(m_truetype_data.get(), glyph_id);
			ret.xmin = FontScalar(bb.xmin);
			ret.ymin = FontScalar(bb.ymin);
			ret.xmax = FontScalar(bb.xmax);
			ret.ymax = FontScalar(bb.ymax);

			// calculate RSB
			ret.right_side_bearing = ret.horz_advance - ret.left_side_bearing - (ret.xmax - ret.xmin);
		}
		else if(this->hasCffOutlines())
		{
			// sap::warn("font/metrics", "bounding-box metrics for CFF-outline fonts are not supported yet!");

			// well, we're shit out of luck.
			// best effort, i guess?
			ret.xmin = FontScalar(m_metrics.xmin);
			ret.xmax = FontScalar(m_metrics.xmax);
			ret.ymin = FontScalar(m_metrics.ymin);
			ret.ymax = FontScalar(m_metrics.ymax);
		}
		else
		{
			sap::internal_error("unsupported outline type?!");
		}

		return ret;
	}


	std::map<size_t, GlyphAdjustment> FontFile::getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const FeatureSet& features) const
	{
		if(m_gpos_table.has_value())
			return off::getPositioningAdjustmentsForGlyphSequence(*m_gpos_table, glyphs, features);
		else if(m_kern_table.has_value())
			return aat::getPositioningAdjustmentsForGlyphSequence(*m_kern_table, glyphs, features);
		else
			return {};
	}

	std::optional<SubstitutedGlyphString> FontFile::performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const FeatureSet& features) const
	{
		if(m_gsub_table.has_value())
			return off::performSubstitutionsForGlyphSequence(*m_gsub_table, glyphs, features);
		else if(m_morx_table.has_value())
			return aat::performSubstitutionsForGlyphSequence(*m_morx_table, glyphs, features);
		else
			return {};
	}
}
