// metrics.cpp
// Copyright (c) 2021, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"  // for convertBEU16
#include "types.h" // for GlyphId

#include "font/truetype.h"    // for BoundingBox, getGlyphBoundingBox
#include "font/font_file.h"   // for GlyphMetrics, FontFile, FontMetrics
#include "font/font_scalar.h" // for FontScalar, font_design_space

namespace font
{
	GlyphMetrics FontSource::getGlyphMetrics(GlyphId glyph_id) const
	{
		if(auto it = m_glyph_metrics.find(glyph_id); it != m_glyph_metrics.end())
			return it->second;

		auto metrics = this->get_glyph_metrics_impl(glyph_id);
		return m_glyph_metrics.emplace(glyph_id, std::move(metrics)).first->second;
	}

}
