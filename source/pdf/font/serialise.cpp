// serialise.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "types.h" // for GlyphId

#include "pdf/font.h"     // for Font, Font::ENCODING_CID, Font::ENCODING_W...
#include "pdf/misc.h"     // for error
#include "pdf/units.h"    // for TextSpace1d
#include "pdf/object.h"   // for Dictionary, Integer, Name, IndirectRef
#include "pdf/document.h" // for File
#include "pdf/builtin_font.h"

#include "font/misc.h"
#include "font/font_file.h" // for FontFile, FontMetrics, generateSubsetName

namespace pdf
{
	PdfFont::PdfFont()
	{
		this->font_dictionary = Dictionary::create(names::Font, {});
	}

	Dictionary* PdfFont::serialise(File* doc) const
	{
		assert(this->font_dictionary->isIndirect());

		if(m_did_serialise)
			sap::internal_error("trying to re-serialise font!");

		m_did_serialise = true;

		assert(this->glyph_widths_array != nullptr);

		// we need to write out the widths.
		if(m_source->isBuiltin())
		{
			auto source = dynamic_cast<BuiltinFont*>(m_source.get());

			// write out the widths, but in a dum way.
			this->glyph_widths_array->clear();

			for(auto g = source->getFirstGlyphId(); g != source->getLastGlyphId(); g = g + 1)
			{
				auto w = this->getMetricsForGlyph(g).horz_advance;
				this->glyph_widths_array->append(Integer::create(static_cast<int>(this->scaleMetricForPDFTextSpace(w).value())));
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
					if(static_cast<uint64_t>(prev.first->value()) + prev.second.size() == static_cast<uint64_t>(widths[i].first))
						prev.second.push_back(Integer::create(static_cast<uint32_t>(widths[i].second)));
					else
						goto foo;
				}
			}

			this->glyph_widths_array->clear();
			for(auto& [gid, ws] : widths2)
			{
				this->glyph_widths_array->append(gid);
				this->glyph_widths_array->append(Array::create(std::move(ws)));
			}

			// finally, make a font subset based on the glyphs that we use.
			assert(this->embedded_contents != nullptr);

			source_file->writeSubset(this->pdf_font_name, this->embedded_contents);

			// write the cmap we'll use for /ToUnicode.
			this->writeUnicodeCMap(doc);

			// and the cidset
			this->writeCIDSet(doc);
		}

		return this->font_dictionary;
	}

	struct FontParts
	{
		Dictionary* font_obj;
		Dictionary* font_descriptor;
		Array* widths_array;
	};

	static FontParts create_font_dictionary(File* doc, zst::str_view name, const font::FontSource* font)
	{
		auto dict = Dictionary::createIndirect(doc, names::Font, {});

		auto glyph_widths_array = Array::createIndirect(doc, {});
		dict->add(names::W, IndirectRef::create(glyph_widths_array));

		const auto& font_metrics = font->metrics();
		auto units_per_em_scale = font_metrics.units_per_em / 1000.0;
		auto scale_metric = [units_per_em_scale](auto metric) -> Integer* {
			return Integer::create(static_cast<int64_t>(metric / units_per_em_scale));
		};

		auto font_bbox = Array::create({
		    scale_metric(font_metrics.xmin),
		    scale_metric(font_metrics.ymin),
		    scale_metric(font_metrics.xmax),
		    scale_metric(font_metrics.ymax),
		});

		int cap_height = 0;
		if(font_metrics.cap_height != 0)
		{
			cap_height = font_metrics.cap_height;
		}
		else
		{
			// basically look for the glyph for 'E' if it exists, then
			// calculate the capheight from that.

			// TODO: i don't feel like doing this now
			cap_height = 700;
		}

		int x_height = 0;
		if(font_metrics.x_height != 0)
			x_height = font_metrics.x_height;
		else
			x_height = 400;

		// not all fonts contain stemv
		int stem_v = font_metrics.stem_v == 0 ? 69 : font_metrics.stem_v;

		// TODO: use the FontFile to determine what the flags should be. no idea how important this is
		// for the pdf to display properly but it's probably entirely inconsequential.
		auto font_desc = Dictionary::createIndirect(doc, names::FontDescriptor,
		    {
		        { names::FontName, Name::create(name) },
		        { names::Flags, Integer::create(4) },
		        { names::FontBBox, font_bbox },
		        { names::ItalicAngle, Integer::create(static_cast<int32_t>(font_metrics.italic_angle)) },
		        { names::Ascent, scale_metric(font_metrics.hhea_ascent) },
		        { names::Descent, scale_metric(font_metrics.hhea_descent) },
		        { names::CapHeight, scale_metric(cap_height) },
		        { names::XHeight, scale_metric(x_height) },
		        { names::StemV, scale_metric(stem_v) },
		    });

		dict->add(names::FontDescriptor, IndirectRef::create(font_desc));
		return {
			.font_obj = dict,
			.font_descriptor = font_desc,
			.widths_array = glyph_widths_array,
		};
	}




	PdfFont* PdfFont::fromFontFile(File* doc, std::unique_ptr<font::FontFile> font_file)
	{
		auto ret = util::make<PdfFont>();

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
		ret->pdf_font_name = font::generateSubsetName(font_file.get());
		auto basefont_name = Name::create(ret->pdf_font_name);

		// start with making the CIDFontType2/0 entry.
		auto font_parts = create_font_dictionary(doc, ret->pdf_font_name, font_file.get());
		auto cidfont_dict = font_parts.font_obj;

		ret->glyph_widths_array = font_parts.widths_array;

		cidfont_dict->add(names::BaseFont, basefont_name);
		cidfont_dict->add(names::CIDSystemInfo,
		    Dictionary::create({
		        { names::Registry, String::create("Adobe") },
		        { names::Ordering, String::create("Identity") },
		        { names::Supplement, Integer::create(0) },
		    }));

		if(font_file->hasTrueTypeOutlines())
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

		// we need a CIDSet for subset fonts
		ret->cidset = Stream::create(doc, {});
		cidfont_dict->add(names::CIDSet, IndirectRef::create(ret->cidset));

		ret->embedded_contents = Stream::create(doc, {});
		ret->embedded_contents->setCompressed(true);

		if(font_file->hasTrueTypeOutlines())
		{
			font_parts.font_descriptor->add(names::FontFile2, IndirectRef::create(ret->embedded_contents));
			ret->font_type = FONT_TRUETYPE_CID;
		}
		else
		{
			// the spec is very very poorly worded on this, BUT from what I can gather, for CFF fonts we can
			// just always use FontFile3 and CIDFontType0C.

			ret->embedded_contents->dictionary()->add(names::Subtype, names::CIDFontType0C.ptr());

			font_parts.font_descriptor->add(names::FontFile3, IndirectRef::create(ret->embedded_contents));
			ret->font_type = FONT_CFF_CID;
		}

		// make a cmap to use for ToUnicode
		ret->unicode_cmap = Stream::create(doc, {});
		ret->unicode_cmap->setCompressed(true);

		// finally, construct the top-level Type0 font.
		auto type0 = ret->font_dictionary;
		type0->makeIndirect(doc);

		type0->add(names::Subtype, names::Type0.ptr());
		type0->add(names::BaseFont, basefont_name);

		// TODO: here's the thing about CMap. if we use Identity-H then we don't need a CMap, but it forces the text
		// stream to contain glyph IDs instead of anything sensible.
		type0->add(names::Encoding, Name::create("Identity-H"));

		// add the unicode map so text selection isn't completely broken
		type0->add(names::ToUnicode, IndirectRef::create(ret->unicode_cmap));
		type0->add(names::DescendantFonts, Array::create(IndirectRef::create(cidfont_dict)));

		ret->encoding_kind = ENCODING_CID;

		ret->m_font_resource_name = zpr::sprint("F{}", doc->getNextFontResourceNumber());
		ret->m_source = std::move(font_file);

		return ret;
	}

	PdfFont* PdfFont::fromBuiltin(File* doc, BuiltinFont::Core14 font_name)
	{
		auto ret = util::make<PdfFont>();
		auto font_dict = ret->font_dictionary;

		ret->font_type = FONT_TYPE1;

		auto builtin_font = BuiltinFont::get(font_name);

		font_dict->makeIndirect(doc);

		font_dict->add(names::Subtype, names::Type1.ptr());
		font_dict->add(names::BaseFont, Name::create(builtin_font->name()));
		font_dict->add(names::Encoding, Name::create("StandardEncoding"));
		ret->encoding_kind = ENCODING_WIN_ANSI;

		ret->m_font_resource_name = zpr::sprint("F{}", doc->getNextFontResourceNumber());

		auto font_parts = create_font_dictionary(doc, builtin_font->name(), builtin_font.get());
		ret->glyph_widths_array = font_parts.widths_array;

		auto desc = font_parts.font_descriptor;
		font_dict->add(names::FontDescriptor, desc);
		font_dict->add(names::Widths, ret->glyph_widths_array);
		font_dict->add(names::FirstChar, Integer::create(static_cast<int>(builtin_font->getFirstGlyphId())));
		font_dict->add(names::LastChar, Integer::create(static_cast<int>(builtin_font->getLastGlyphId())));

		ret->m_source = std::move(builtin_font);

		return ret;
	}
}
