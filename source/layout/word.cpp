// word.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "util.h"

#include "font/font.h"

#include "pdf/pdf.h"
#include "pdf/text.h"
#include "pdf/misc.h"
#include "pdf/font.h"

namespace sap
{
	// TODO: this needs to handle unicode composing/decomposing also, which is a massive pain
	static std::optional<uint32_t> peek_one_glyphid(const pdf::Font* font, zst::byte_span utf8, size_t* num_bytes)
	{
		if(utf8.empty())
			return { };

		auto copy = utf8;
		auto cp = unicode::consumeCodepointFromUtf8(copy);
		auto gid = font->getGlyphIdFromCodepoint(cp);

		if(num_bytes != nullptr)
			*num_bytes = utf8.size() - copy.size();

		return gid;
	}

	static uint32_t read_one_glyphid(const pdf::Font* font, zst::byte_span& utf8)
	{
		assert(utf8.size() > 0);
		size_t num = 0;
		auto gid = peek_one_glyphid(font, utf8, &num);
		utf8.remove_prefix(num);
		return *gid;
	}

	static std::vector<std::pair<uint32_t, int>> get_glyphs_and_spacing(const pdf::Font* font, zst::str_view text)
	{
		std::vector<std::pair<uint32_t, int>> ret;
		auto sv = text.bytes();

		/*
			We handle the normal case correctly, in that `adj1` decreases the advance of the first
			glyph, in effect moving the second glyph closer to the first. The thing is, I don't know
			what to do for nonzero `adj2` values...
		*/

		while(sv.size() > 0)
		{
		#if 0
			if(auto ligatures = font->getLigaturesForGlyph(gid); ligatures.has_value())
			{
				auto copy = sv;

				// setup a small array and prime it with as many glyphs as we can.
				size_t lookahead_size = 1;
				uint32_t lookahead[font::MAX_LIGATURE_LENGTH] { };
				size_t codepoint_bytes[font::MAX_LIGATURE_LENGTH] { };
				lookahead[0] = gid;
				codepoint_bytes[0] = 0; // this one is 0 because it doesn't matter.

				for(; !copy.empty() && lookahead_size < font::MAX_LIGATURE_LENGTH; lookahead_size++)
				{
					size_t nbytes = 0;
					auto tmp = peek_one_glyphid(font, copy, &nbytes);
					assert(tmp.has_value());
					copy.remove_prefix(nbytes);

					lookahead[lookahead_size] = *tmp;
					codepoint_bytes[lookahead_size] = nbytes;
				}

				for(auto& liga : ligatures->ligatures)
				{
					if(liga.num_glyphs > lookahead_size)
						continue;

					if(memcmp(liga.glyphs, lookahead, liga.num_glyphs * sizeof(uint32_t)) == 0)
					{
						sap::log("layout", "substituting ligature: '{}' -> '{}'", liga.glyphs, liga.substitute);

						// perform the substitution:
						gid = liga.substitute;
						font->loadMetricsForGlyph(gid);
						font->markLigatureUsed(liga);

						// and drop the glyphs from the real array, noting that the first one was already consumed
						for(size_t i = 1; i < liga.num_glyphs; i++)
							sv.remove_prefix(codepoint_bytes[i]);

						// and quit the loop.
						break;
					}
				}
			}
		#endif

			int adjust = 0;
			auto gid = read_one_glyphid(font, sv);
			auto next_gid = peek_one_glyphid(font, sv, nullptr);

			if(next_gid.has_value())
			{
				auto kerning_pair = font->getKerningForGlyphs(gid, *next_gid);
				if(kerning_pair.has_value())
				{
					adjust = -1 * kerning_pair->first.horz_advance;
					if(kerning_pair->second.horz_advance != 0)
						zpr::println("warning: unsupported case where adj2 is not 0");
				}
			}

			ret.push_back({ gid, adjust });
		}

		return ret;
	}

	void Word::computeMetrics(const Style* parent_style)
	{
		auto style = Style::combine(m_style, parent_style);
		this->setStyle(style);

		auto font = style->font();
		auto font_size = style->font_size();
		m_glyphs = get_glyphs_and_spacing(font, this->text);

		// we shouldn't have 0 glyphs in a word... right?
		assert(m_glyphs.size() > 0);

		// TODO: vertical writing mode

		// size is in sap units, which is in mm; metrics are in typographic units, so 72dpi;
		// calculate the scale accordingly.
		const auto font_metrics = font->getFontMetrics();

		constexpr auto tpu = [](auto... xs) -> auto { return pdf::typographic_unit(xs...); };

		this->size = { 0, 0 };
		this->size.y() = ((tpu(font_metrics.ymax) - tpu(font_metrics.ymin))
						* (font_size.value() / pdf::GLYPH_SPACE_UNITS)).into(sap::Scalar{});

		auto font_size_tpu = font_size.into(dim::units::pdf_typographic_unit{});

		for(auto& [ gid, kern ] : m_glyphs)
		{
			auto met = font->getMetricsForGlyph(gid);

			// note: positive kerns move left, so subtract it.
			auto glyph_width = font->scaleFontMetricForFontSize(met.horz_advance, font_size_tpu);
			auto kern_adjust = font->scaleFontMetricForFontSize(kern, font_size_tpu);

			this->size.x() += (glyph_width - kern_adjust).into(sap::Scalar{});
		}

		{
			auto space_gid = font->getGlyphIdFromCodepoint(' ');
			auto space_adv = font->getMetricsForGlyph(space_gid).horz_advance;
			auto space_width = font->scaleFontMetricForFontSize(space_adv, font_size_tpu);

			m_space_width = space_width.into(sap::Scalar{});
		}
	}

	Scalar Word::spaceWidth() const
	{
		return m_space_width;
	}

	void Word::render(pdf::Text* text) const
	{
		// zpr::println("word = '{}', cursor = {}, linebreak = {}", this->text, m_position, m_linebreak_after);

		const auto font = m_style->font();
		const auto font_size = m_style->font_size();
		text->setFont(font, font_size.into(pdf::Scalar{}));

		auto add_gid = [&font, text](uint32_t gid) {
			if(font->encoding_kind == pdf::Font::ENCODING_CID)
				text->addEncoded(2, gid);
			else
				text->addEncoded(1, gid);
		};

		for(auto& [ gid, kern ] : m_glyphs)
		{
			add_gid(gid);
			text->offset(-font->scaleFontMetricForPDFTextSpace(kern));
		}

		if(!m_linebreak_after && m_next_word != nullptr)
		{
			auto space_gid = font->getGlyphIdFromCodepoint(' ');
			add_gid(space_gid);

			if(m_post_space_ratio != 1.0)
			{
				auto space_adv = font->getMetricsForGlyph(space_gid).horz_advance;

				// ratio > 1 = expand, < 1 = shrink
				auto extra = font->scaleFontMetricForPDFTextSpace(space_adv) * (m_post_space_ratio - 1.0);
				text->offset(extra);

				// zpr::println("'{}': font = {}, ratio = {}, extra = {}", this->text, font_size_tpu, m_post_space_ratio, extra);
			}

			/*
				TODO: here, we also want to handle kerning between the space and the start of the next word
				(see the longer explanation in layout/paragraph.cpp)
			*/
		}
	}
}
