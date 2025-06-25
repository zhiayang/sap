// metrics.cpp
// Copyright (c) 2021, yuki
// SPDX-License-Identifier: Apache-2.0

#include "util.h"
#include "types.h"

#include "font/truetype.h"
#include "font/font_file.h"
#include "font/font_scalar.h"

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
