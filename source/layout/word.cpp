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
			TODO: honestly, i don't know how this is "supposed" to be handled. Our values
			are the same as what lualatex puts out for the font, so that's ok. the problem
			is that, according to the spec, adj1 is supposed to apply to the first glyph and
			adj2 to the second one (duh).

			except that all the kerning pairs only have adjustments for the first in the pair,
			ie. their effect would be to move the first glyph closer to the second one, instead
			of (logically) the second one closer to the first.

			again, how lualatex does this is (apparently) to use adj1 to move the second glyph
			instead (which produces the same graphical effect).

			so far i haven't found a font that has a non-zero adj2 (and there's a warning here in
			case such a font appears), so for now we just copy what lualatex (allegedly) does.


			the result of that long paragraph is that instead of doing (cur_glyph, next_glyph), we
			do (prev_glyph, cur_glyph), since we are applying the kerning value to the second glyph
			instead. this shouldn't have any noticeable effect on the output (as mentioned above),
			but it is something to note in case any weird cases are discovered.


			UPDATE:
			ok, so I think the way it's *supposed* to work is that the kerning affects the *advance*
			of the first glyph. it still has the same effect as moving the second glyph to the left
			(and not the first glyph to the right), just via a slightly different mechanism.

			the layout/rendering code below would need to be slightly modified to fit that. Still not
			quite sure what to do with non-zero adj2 values...
		*/

		uint32_t prev_gid = -1;
		while(sv.size() > 0)
		{
			auto gid = read_one_glyphid(font, sv);
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
			if(prev_gid != static_cast<uint32_t>(-1))
			{
				auto kerning_pair = font->getKerningForGlyphs(prev_gid, gid);
				if(kerning_pair.has_value())
				{
					// note: since as mentioned above we're doing (prev, cur), the adjustment
					// goes before the current glyph.
					adjust = -1 * kerning_pair->first.horz_advance;

					if(kerning_pair->second.horz_advance != 0)
						zpr::println("warning: unsupported case where adj2 is not 0");
				}
			}

			ret.push_back({ gid, adjust });
			prev_gid = gid;
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

		// TODO: vertical writing mode

		// PDF 1.7: 9.2.4 Glyph Positioning and Metrics
		// ... the units of glyph space are one-thousandth of a unit of text space ...
		constexpr auto GLYPH_SPACE_UNITS = 1000.0;

		// size is in sap units, which is in mm; metrics are in typographic units, so 72dpi;
		// calculate the scale accordingly.
		auto font_metrics = font->getFontMetrics();

		constexpr auto tpu = [](auto... xs) -> auto { return pdf::typographic_unit_y_down(xs...); };

		this->size = { 0, 0 };
		this->size.y() = ((tpu(font_metrics.ymax) - tpu(font_metrics.ymin))
						* (font_size.value() / GLYPH_SPACE_UNITS)).into(sap::Scalar{});

		auto font_size_tpu = font_size.into(dim::units::pdf_typographic_unit_y_down{});

		for(auto& [ gid, kern ] : m_glyphs)
		{
			auto met = font->getMetricsForGlyph(gid);

			// note: positive kerns move left, so subtract it.
			this->size.x() += ((met.horz_advance * font_size_tpu - tpu(kern)) / GLYPH_SPACE_UNITS).into(sap::Scalar{});
		}

		{
			auto space_gid = font->getGlyphIdFromCodepoint(' ');
			auto space_width = font->getMetricsForGlyph(space_gid).horz_advance;
			m_space_width = ((space_width * font_size_tpu) / GLYPH_SPACE_UNITS).into(sap::Scalar{});
		}
	}

	Scalar Word::spaceWidth() const
	{
		return m_space_width;
	}

	void Word::render(pdf::Text* text) const
	{
		zpr::println("word = '{}', cursor = {}, linebreak = {}", this->text, m_position, m_linebreak_after);

		// check if we have kerning for the



		text->setFont(m_style->font(), m_style->font_size().into(pdf::Scalar{}));
		for(auto& [ gid, kern ] : m_glyphs)
		{
			if(kern != 0)
				text->offset(-pdf::Scalar(kern));

			if(m_style->font()->encoding_kind == pdf::Font::ENCODING_CID)
				text->addEncoded(2, gid);
			else
				text->addEncoded(1, gid);
		}
	}
}
