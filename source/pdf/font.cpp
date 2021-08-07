// font.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include <algorithm>

#include "pdf/font.h"
#include "pdf/misc.h"
#include "pdf/object.h"
#include "pdf/win_ansi_encoding.h"

#include "font/font.h"

namespace pdf
{
	Font::Font()
	{
		this->font_dictionary = Dictionary::create(names::Font, { });
	}

	Dictionary* Font::serialise(Document* doc) const
	{
		if(!this->font_dictionary->is_indirect)
			this->font_dictionary->makeIndirect(doc);

		// we need to write out the widths.
		if(this->source_file && this->glyph_widths_array)
		{
			std::vector<std::pair<uint32_t, int>> widths;
			for(auto& [ gid, m ] : this->glyph_metrics)
				widths.emplace_back(gid, m.horz_advance);

			std::sort(widths.begin(), widths.end(), [](const auto& a, const auto& b) -> bool {
				return a.first < b.first;
			});

			std::vector<std::pair<Integer*, std::vector<Object*>>> widths2;
			for(size_t i = 0; i < widths.size(); i++)
			{
				if(!widths2.empty() && widths2.back().first->value == widths[i].first - 1)
				{
					widths2.back().second.push_back(Integer::create(widths[i].second));
				}
				else
				{
					widths2.emplace_back(Integer::create(widths[i].first),
						std::vector<Object*> { Integer::create(widths[i].second) });
				}
			}

			for(auto& [ gid, ws ] : widths2)
			{
				this->glyph_widths_array->values.push_back(gid);
				this->glyph_widths_array->values.push_back(Array::create(std::move(ws)));
			}
		}

		return this->font_dictionary;
	}





	std::optional<font::GlyphLigatureSet> Font::getLigaturesForGlyph(uint32_t glyph) const
	{
		if(auto it = this->glyph_ligatures.find(glyph); it != this->glyph_ligatures.end())
			return it->second;

		return { };
	}

	void Font::loadMetricsForGlyph(uint32_t glyph) const
	{
		if(!this->source_file || this->glyph_metrics.find(glyph) != this->glyph_metrics.end())
			return;

		// get and cache the glyph widths as well.
		auto metrics = this->source_file->getGlyphMetrics(glyph);

		// if single adjustments are present, then add them now.
		if(auto single_adj = this->source_file->getGlyphAdjustment(glyph); single_adj.has_value())
		{
			metrics.horz_advance += single_adj->horz_advance;
			metrics.vert_advance += single_adj->vert_advance;
			metrics.horz_placement += single_adj->horz_placement;
			metrics.vert_placement += single_adj->vert_placement;
		}

		this->glyph_metrics[glyph] = metrics;
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



	Font* Font::fromFontFile(Document* doc, font::FontFile* font_file)
	{
		auto ret = util::make<Font>();
		ret->source_file = font_file;

		/*
			this is the general structure for composite fonts (which we always create, for now):

			/Font: /Subtype Type0
				/Encoding: something
				/BaseFont: the postscript name
				/DescendantFonts: [ bunch of CIDFonts ]
				/ToUnicode: unicode mapping

			/Font: /Subtype CIDFontType2/CIDFontType0 (depending on truetype or CFF)
				/BaseFont: the postscript name
				/W: a table of widths
				/FontDescriptor: the font descriptor
				/CIDSystemInfo: some useless (but required) stuff

			/FontDescriptor:
				/bunch of metrics;
				/FontFile0/1/2/3: the stream containing the actual file
				/CIDSet: if we're doing a subset (which we aren't for now)
		*/
		auto basefont_name = Name::create(font_file->postscript_name);

		// start with making the CIDFontType2/0 entry.
		auto cidfont_dict = Dictionary::createIndirect(doc, names::Font, { });

		cidfont_dict->add(names::BaseFont, basefont_name);
		cidfont_dict->add(names::CIDSystemInfo, Dictionary::create({
			{ names::Registry, String::create("Sap") },
			{ names::Ordering, String::create("Identity") },
			{ names::Supplement, Integer::create(0) },
		}));

		bool truetype_outlines = (font_file->outline_type == font::FontFile::OUTLINES_TRUETYPE);

		ret->glyph_widths_array = Array::createIndirect(doc, { });
		cidfont_dict->add(names::W, IndirectRef::create(ret->glyph_widths_array));

		if(truetype_outlines)
		{
			cidfont_dict->add(names::CIDToGIDMap, names::Identity.ptr());
			cidfont_dict->add(names::Subtype, names::CIDFontType2.ptr());
		}
		else
		{
			cidfont_dict->add(names::Subtype, names::CIDFontType0.ptr());
		}

		/*
			TODO:
			for now we are outputting glyph IDs to the pdf, which is pretty dumb. in the future
			we probably want to either:

			(a) find a way to make a CMap that reads utf-8 from the text stream and converts that to GIDs
			(b) use some font-specific weirdness to directly map utf-16 (or rather ucs-2) to GIDs. with
				otf-cmap format types 6, 10, 12, and 13, we can very easily make a CMap that is not
				monstrous in size.
		*/

		auto font_bbox = Array::create({
			Integer::create(font_file->metrics.xmin),
			Integer::create(font_file->metrics.ymin),
			Integer::create(font_file->metrics.xmax),
			Integer::create(font_file->metrics.ymax)
		});

		int cap_height = 0;
		if(font_file->metrics.cap_height != 0)
		{
			cap_height = font_file->metrics.cap_height;
		}
		else
		{
			// basically look for the glyph for 'E' if it exists, then
			// calculate the capheight from that.

			// TODO: i don't feel like doing this now
			cap_height = 700;
			zpr::println("your dumb font doesn't tell me cap_height, assuming 700");
		}

		// truetype fonts don't contain stemv.
		static constexpr int STEMV_CONSTANT = 69;

		// TODO: use the FontFile to determine what the flags should be. no idea how important this is
		// for the pdf to display properly but it's probably entirely inconsequential.
		auto font_desc = Dictionary::createIndirect(doc, names::FontDescriptor, {
			{ names::FontName, basefont_name },
			{ names::Flags, Integer::create(4) },
			{ names::FontBBox, font_bbox },
			{ names::ItalicAngle, Integer::create(font_file->metrics.italic_angle) },
			{ names::Ascent, Integer::create(font_file->metrics.ascent) },
			{ names::Descent, Integer::create(font_file->metrics.descent) },
			{ names::CapHeight, Integer::create(cap_height) },
			{ names::XHeight, Integer::create(69) },
			{ names::StemV, Integer::create(STEMV_CONSTANT) }
		});

		cidfont_dict->add(names::FontDescriptor, IndirectRef::create(font_desc));

		/*
			TODO: we want to add the width table (/W) to the cidfont_dict as well. if we don't then the default
			width is used for all glyphs, which is probably ugly af.
		*/

		/*
			TODO: for now we are dumping the entire file into the pdf; we probably want some kind of subsetting functionality,
			which requires improving the robustness of the OTF parser (and handling its glyf [or the CFF equiv.) table.
		*/
		auto file_contents = Stream::create(doc, { });
		file_contents->setCompressed(true);
		file_contents->append(font_file->file_bytes, font_file->file_size);

		if(truetype_outlines)
		{
			// TODO: this makes a copy of the file (since Stream wants to own the memory also),
			// which is not ideal.
			font_desc->add(names::FontFile2, IndirectRef::create(file_contents));
			ret->font_type = FONT_TRUETYPE_CID;
		}
		else
		{
			// the spec is very very poorly worded on this, BUT from what I can gather, for CFF fonts we can
			// just always use FontFile3 and CIDFontType0C.

			file_contents->dict->add(names::Subtype, names::CIDFontType0C.ptr());

			font_desc->add(names::FontFile3, IndirectRef::create(file_contents));
			ret->font_type = FONT_CFF_CID;
		}


		// finally, construct the top-level Type0 font.
		auto type0 = ret->font_dictionary;
		type0->add(names::Subtype, names::Type0.ptr());
		type0->add(names::BaseFont, basefont_name);

		// TODO: here's the thing about CMap. if we use Identity-H then we don't need a CMap, but it forces the text stream
		// to contain glyph IDs instead of anything sensible.
		type0->add(names::Encoding, Name::create("Identity-H"));

		type0->add(names::DescendantFonts, Array::create(IndirectRef::create(cidfont_dict)));

		ret->encoding_kind = ENCODING_CID;

		/*
			TODO: come up with a better way of loading these things. ligatures are probably fine (i don't expect
			thousands of glyphs/combinations), but kerning pairs can explode if they use the ClassDef format of
			defining pairs.

			eg. a font like Myriad Pro that supports a few dozen languages already has 700k+ kerning pairs when
			fully expanded. either we come up with a way to not have a fully expanded map (but that would basically
			be the same as reading straight from the file), or don't preload them at all.

			also, for both kerning and ligatures, ad-hoc caching might lead to the cache table filling up with
			useless entries -- this is why ligatures aren't ad-hoc cached (if not every combination of 4 glyphs
			would eventually end up inside). kerning pairs are similar, where eventually every combination of 2
			glyphs would end up inside ><
		*/
		ret->glyph_ligatures = font_file->getAllGlyphLigatures();
		// ret->kerning_pairs = font_file->getAllKerningPairs();

		return ret;
	}

	Font* Font::fromBuiltin(zst::str_view name)
	{
		const char* known_fonts[] = {
			"Times-Roman", "Times-Italic", "Times-Bold", "Times-BoldItalic",
			"Helvetica", "Helvetica-Oblique", "Helvetica-Bold", "Helvetica-BoldOblique",
			"Courier", "Courier-Oblique", "Courier-Bold", "Courier-BoldOblique",
			"Symbol", "ZapfDingbats",
		};

		auto it = std::find_if(std::begin(known_fonts), std::end(known_fonts), [&name](auto& n) {
			return zst::str_view(n) == name;
		});

		if(it == std::end(known_fonts))
			pdf::error("'{}' is not a PDF builtin font", name);

		auto font = util::make<Font>();
		font->font_type = FONT_TYPE1;

		auto dict = font->font_dictionary;
		dict->add(names::Subtype, names::Type1.ptr());
		dict->add(names::BaseFont, Name::create(name));
		dict->add(names::Encoding, Name::create("WinAnsiEncoding"));
		font->encoding_kind = ENCODING_WIN_ANSI;

		return font;
	}
}
