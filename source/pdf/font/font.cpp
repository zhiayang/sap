// font.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId, GlyphId::notdef

#include "pdf/font.h"              // for Font, Font::ENCODING_CID, Font::E...
#include "pdf/misc.h"              // for error
#include "pdf/units.h"             // for PdfScalar, Size2d_YDown
#include "pdf/win_ansi_encoding.h" // for WIN_ANSI

#include "font/tag.h"         // for Tag
#include "font/features.h"    // for GlyphAdjustment, FeatureSet, Subs...
#include "font/font_file.h"   // for GlyphInfo, CharacterMapping, Glyp...
#include "font/font_scalar.h" // for FontScalar, font_design_space

namespace pdf
{
	std::string PdfFont::getFontResourceName() const
	{
		return this->font_resource_name;
	}


	const font::FontMetrics& PdfFont::getFontMetrics() const
	{
		// TODO: metrics for the 14 built-in fonts
		// see https://stackoverflow.com/questions/6383511/
		assert(m_source_file);

		return m_source_file->metrics();
	}

	void PdfFont::markGlyphAsUsed(GlyphId glyph) const
	{
		m_source_file->markGlyphAsUsed(glyph);
	}

	font::GlyphMetrics PdfFont::getMetricsForGlyph(GlyphId glyph) const
	{
		return m_source_file->getGlyphMetrics(glyph);
	}

	GlyphId PdfFont::getGlyphIdFromCodepoint(char32_t codepoint) const
	{
		if(this->encoding_kind == ENCODING_WIN_ANSI)
		{
			return GlyphId { encoding::WIN_ANSI(codepoint) };
		}
		else if(this->encoding_kind == ENCODING_CID)
		{
			assert(m_source_file != nullptr);

			// also pre-load the metrics (because in all likelihood we'll need the metrics soon)
			auto gid = m_source_file->getGlyphIndexForCodepoint(codepoint);
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


	Size2d_YDown PdfFont::getWordSize(zst::wstr_view text, PdfScalar font_size) const
	{
		auto sz = m_source_file->getWordSize(text);
		return Size2d_YDown(                                 //
		    this->scaleMetricForFontSize(sz.x(), font_size), //
		    this->scaleMetricForFontSize(sz.y(), font_size));
	}

	std::vector<font::GlyphInfo> PdfFont::getGlyphInfosForString(zst::wstr_view text) const
	{
		assert(m_source_file != nullptr);
		return m_source_file->getGlyphInfosForString(text);
	}

	void PdfFont::addGlyphUnicodeMapping(GlyphId glyph, std::vector<char32_t> codepoints) const
	{
		if(auto it = m_extra_unicode_mappings.find(glyph); it != m_extra_unicode_mappings.end() && it->second != codepoints)
		{
			sap::warn("font", "conflicting unicode mapping for glyph '{}' (existing: {}, new: {})", glyph, it->second,
			    codepoints);
		}

		m_extra_unicode_mappings[glyph] = std::move(codepoints);
	}


	std::map<size_t, font::GlyphAdjustment> PdfFont::getPositioningAdjustmentsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const font::off::FeatureSet& features) const
	{
		if(!m_source_file)
			return {};

		return m_source_file->getPositioningAdjustmentsForGlyphSequence(glyphs, features);
	}

	std::optional<std::vector<GlyphId>> PdfFont::performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const font::off::FeatureSet& features) const
	{
		if(!m_source_file)
			return std::nullopt;

		auto subst = m_source_file->performSubstitutionsForGlyphSequence(glyphs, features);
		if(not subst.has_value())
			return std::nullopt;

		auto& cmap = m_source_file->characterMapping();
		for(auto& [out, in] : subst->mapping.replacements)
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

		for(auto& [out, ins] : subst->mapping.contractions)
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

		for(auto& g : subst->mapping.extra_glyphs)
		{
			this->markGlyphAsUsed(g);

			if(cmap.reverse.find(g) == cmap.reverse.end())
				this->addGlyphUnicodeMapping(g, find_codepoint_for_gid(g));
		}

		return std::move(subst->glyphs);
	}
}
