// text.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <cassert>

#include "util.h"
#include "pdf/font.h"
#include "pdf/text.h"
#include "pdf/page.h"
#include "pdf/misc.h"

namespace pdf
{
	// TODO: this needs to handle unicode composing/decomposing also, which is a massive pain
	static std::optional<uint32_t> peek_one_glyphid(const Font* font, zst::byte_span utf8, size_t* num_bytes)
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

	static uint32_t read_one_glyphid(const Font* font, zst::byte_span& utf8)
	{
		assert(utf8.size() > 0);
		size_t num = 0;
		auto gid = peek_one_glyphid(font, utf8, &num);
		utf8.remove_prefix(num);
		return *gid;
	}


	// also encode it.
	static std::string encode_text_with_font(const Font* font, zst::str_view text)
	{
		std::string ret;
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
		*/

		uint32_t prev_gid = -1;
		while(sv.size() > 0)
		{
			auto gid = read_one_glyphid(font, sv);
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
					zpr::println("gid = {}, trying liga n={}, {}", gid, liga.num_glyphs, liga.glyphs);
					zpr::println("lookahead = {}", lookahead);
					if(liga.num_glyphs > lookahead_size)
						continue;

					if(memcmp(liga.glyphs, lookahead, liga.num_glyphs * sizeof(uint32_t)) == 0)
					{
						// perform the substitution:
						gid = liga.substitute;
						font->loadMetricsForGlyph(gid);

						zpr::println("substituted {} glyphs for {}", liga.num_glyphs, gid);

						// and drop the glyphs from the real array, noting that the first one was already consumed
						for(size_t i = 1; i < liga.num_glyphs; i++)
							sv.remove_prefix(codepoint_bytes[i]);

						// and quit the loop.
						break;
					}
				}
			}

			// TODO: use CMap or something to handle this situation.
			if(gid > 0xffff)
				pdf::error("TODO: sorry");

			// TODO: this is pretty suboptimal output
			auto append_glyph = [&ret, &font](uint32_t g) {
				if(font->encoding_kind == Font::ENCODING_CID)
					ret += zpr::sprint("<{02x}{02x}>", (g & 0xff00) >> 8, g & 0xff);
				else
					ret += zpr::sprint("<{02x}>", g & 0xff);
			};

			if(prev_gid != static_cast<uint32_t>(-1))
			{
				auto kerning_pair = font->getKerningForGlyphs(prev_gid, gid);
				if(kerning_pair.has_value())
				{
					auto adj = -1 * kerning_pair->first.horz_advance;

					if(kerning_pair->second.horz_advance != 0)
						zpr::println("warning: unsupported case where adj2 is not 0");

					// note: since as mentioned above we're doing (prev, cur), the adjustment
					// goes before the current glyph.
					if(adj != 0)
						ret += zpr::sprint("{}", adj);
				}
			}

			append_glyph(gid);
			prev_gid = gid;
		}

		return ret;
	}



	std::string Text::serialise(const Page* page) const
	{
		if(this->font == nullptr || this->font_height.zero())
			return "";

		return zpr::sprint("BT\n  /{} {} Tf\n {}\nET\n",
			page->getNameForFont(this->font), this->font_height, commands);
	}

	void Text::setFont(Font* font, Scalar height)
	{
		assert(font != nullptr);
		this->font = font;
		this->font_height = height;
	}

	// convention is that all appends to the command list should start with a " ".
	void Text::moveAbs(Coord pos)
	{
		// do this in two steps; first, replace the text matrix with the identity to get to (0, 0),
		// then perform an offset to get to the desired position.
		this->commands += zpr::sprint(" 1 0 0 1 0 0 Tm");
		this->offset(pos);
	}

	void Text::offset(Coord offset)
	{
		this->commands += zpr::sprint(" {} {} Td", offset.x, offset.y);
	}

	void Text::startGroup()
	{
		if(this->in_group)
			pdf::error("text groups (TJ) cannot be nested");

		this->in_group = true;
		this->commands += " [";
	}

	void Text::addText(zst::str_view text)
	{
		if(this->in_group)
			this->commands += zpr::sprint(" {}", encode_text_with_font(this->font, text));
		else
			this->commands += zpr::sprint(" [{}] TJ", encode_text_with_font(this->font, text));
	}

	void Text::addText(Scalar offset, zst::str_view text)
	{
		if(!this->in_group)
			pdf::error("addText() with offset can only be used after beginGroup()");

		this->commands += zpr::sprint(" {} {}", offset, encode_text_with_font(this->font, text));
	}

	void Text::endGroup()
	{
		if(!this->in_group)
			pdf::error("endGroup() can only be used after beginGroup()");

		this->in_group = false;
		this->commands += "]";
	}
}
