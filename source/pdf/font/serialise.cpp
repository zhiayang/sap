// serialise.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/file.h"   // for File
#include "pdf/font.h"   // for Font, Font::ENCODING_CID, Font::ENCODING_W...
#include "pdf/misc.h"   // for error
#include "pdf/units.h"  // for TextSpace1d
#include "pdf/object.h" // for Dictionary, Integer, Name, IndirectRef
#include "pdf/builtin_font.h"

#include "font/misc.h"
#include "font/font_file.h" // for FontFile, FontMetrics, generateSubsetName

namespace pdf
{
	void PdfFont::serialise() const
	{
		assert(m_font_dictionary->isIndirect());

		if(m_did_serialise)
			return;

		m_did_serialise = true;

		assert(m_glyph_widths_array != nullptr);

		// we need to write out the widths.
		if(m_source->isBuiltin())
		{
			auto source = dynamic_cast<BuiltinFont*>(m_source.get());

			// write out the widths, but in a dum way.
			m_glyph_widths_array->clear();

			for(auto g = source->getFirstGlyphId(); g != source->getLastGlyphId(); g = g + 1)
			{
				auto w = this->getMetricsForGlyph(g).horz_advance;
				m_glyph_widths_array
				    ->append(Integer::create(static_cast<int>(this->scaleMetricForPDFTextSpace(w).value())));
			}
		}
		else
		{
			auto source_file = dynamic_cast<font::FontFile*>(m_source.get());

			std::vector<std::pair<GlyphId, double>> widths {};
			for(auto& gid : source_file->usedGlyphs())
			{
				auto width = this->getMetricsForGlyph(gid).horz_advance;
				widths.emplace_back(gid, this->scaleMetricForPDFTextSpace(width).value());
			}

			std::sort(widths.begin(), widths.end(), [](const auto& a, const auto& b) -> bool {
				return a.first < b.first;
			});

			std::vector<std::pair<Integer*, std::vector<Object*>>> widths2;
			for(size_t i = 0; i < widths.size(); i++)
			{
				if(widths2.empty())
				{
				foo:
					widths2.emplace_back(Integer::create(static_cast<uint32_t>(widths[i].first)),
					    std::vector<Object*> { Integer::create(static_cast<uint32_t>(widths[i].second)) });
				}
				else
				{
					auto& prev = widths2.back();
					if(static_cast<uint64_t>(prev.first->value()) + prev.second.size()
					    == static_cast<uint64_t>(widths[i].first))
						prev.second.push_back(Integer::create(static_cast<uint32_t>(widths[i].second)));
					else
						goto foo;
				}
			}

			m_glyph_widths_array->clear();
			for(auto& [gid, ws] : widths2)
			{
				m_glyph_widths_array->append(gid);
				m_glyph_widths_array->append(Array::create(std::move(ws)));
			}

			// finally, make a font subset based on the glyphs that we use.
			assert(m_embedded_contents != nullptr);

			source_file->writeSubset(m_pdf_font_name, m_embedded_contents);

			// write the cmap we'll use for /ToUnicode.
			this->writeUnicodeCMap();
			this->writeUTF8CMap();

			// and the cidset
			this->writeCIDSet();
		}
	}
}
