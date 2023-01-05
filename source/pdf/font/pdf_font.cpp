// pdf_font.cpp
// Copyright (c) 2021, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h"  // for hashmap
#include "types.h" // for GlyphId, GlyphId::notdef

#include "pdf/font.h"              // for Font, Font::ENCODING_CID, Font::E...
#include "pdf/misc.h"              // for error
#include "pdf/units.h"             // for PdfScalar, Size2d_YDown
#include "pdf/object.h"            //
#include "pdf/document.h"          //
#include "pdf/win_ansi_encoding.h" // for WIN_ANSI

#include "font/tag.h"         // for Tag
#include "font/misc.h"        //
#include "font/features.h"    // for GlyphAdjustment, FeatureSet, Subs...
#include "font/font_file.h"   // for GlyphInfo, CharacterMapping, Glyp...
#include "font/font_scalar.h" // for FontScalar, font_design_space

namespace pdf
{
	static Dictionary* create_font_descriptor(File* doc, zst::str_view name, const font::FontSource* font)
	{
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

		return font_desc;
	}

	PdfFont::PdfFont(File* doc, std::unique_ptr<pdf::BuiltinFont> source_)
	    : m_source(std::move(source_))
	    , m_font_dictionary(Dictionary::createIndirect(doc, names::Font, {}))
	    , m_glyph_widths_array(Array::createIndirect(doc))
	    , m_font_resource_name(zpr::sprint("F{}", doc->getNextFontResourceNumber()))
	{
		auto* builtin_src = static_cast<BuiltinFont*>(m_source.get());

		this->font_type = FONT_TYPE1;

		auto descriptor = create_font_descriptor(doc, builtin_src->name(), m_source.get());

		m_font_dictionary->add(names::FontDescriptor, descriptor);
		m_font_dictionary->add(names::Widths, m_glyph_widths_array);
		m_font_dictionary->add(names::FirstChar, Integer::create(static_cast<int>(builtin_src->getFirstGlyphId())));
		m_font_dictionary->add(names::LastChar, Integer::create(static_cast<int>(builtin_src->getLastGlyphId())));

		m_font_dictionary->add(names::Subtype, names::Type1.ptr());
		m_font_dictionary->add(names::BaseFont, Name::create(builtin_src->name()));
		m_font_dictionary->add(names::Encoding, Name::create("StandardEncoding"));
	}

	PdfFont::PdfFont(File* doc, std::unique_ptr<font::FontFile> font_file_)
	    : m_source(std::move(font_file_))
	    , m_font_dictionary(Dictionary::createIndirect(doc, names::Font, {}))
	    , m_glyph_widths_array(Array::createIndirect(doc))
	    , m_font_resource_name(zpr::sprint("F{}", doc->getNextFontResourceNumber()))
	{
		auto file_src = static_cast<font::FontFile*>(m_source.get());

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
		m_pdf_font_name = font::generateSubsetName(file_src);
		auto basefont_name = Name::create(m_pdf_font_name);

		// start with making the CIDFontType2/0 entry.
		auto cidfont_dict = Dictionary::createIndirect(doc, names::Font, {});
		auto descriptor = create_font_descriptor(doc, m_pdf_font_name, file_src);

		cidfont_dict->add(names::W, m_glyph_widths_array);
		cidfont_dict->add(names::FontDescriptor, descriptor);
		cidfont_dict->add(names::BaseFont, basefont_name);
		cidfont_dict->add(names::CIDSystemInfo,
		    Dictionary::create({
		        { names::Registry, String::create("Adobe") },
		        { names::Ordering, String::create("Identity") },
		        { names::Supplement, Integer::create(0) },
		    }));

		if(file_src->hasTrueTypeOutlines())
		{
			this->font_type = FONT_TRUETYPE_CID;
			cidfont_dict->add(names::Subtype, names::CIDFontType2.ptr());
		}
		else
		{
			this->font_type = FONT_CFF_CID;
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
		m_cidset = Stream::create(doc, {});
		descriptor->add(names::CIDSet, IndirectRef::create(m_cidset));

		m_embedded_contents = Stream::create(doc, {});
		m_embedded_contents->setCompressed(true);

		if(file_src->hasTrueTypeOutlines())
		{
			descriptor->add(names::FontFile2, m_embedded_contents);
			cidfont_dict->add(names::CIDToGIDMap, names::Identity.ptr());
		}
		else
		{
			// the spec is very very poorly worded on this, BUT from what I can gather, for CFF fonts we can
			// just always use FontFile3 and CIDFontType0C.
			m_embedded_contents->dictionary()->add(names::Subtype, names::CIDFontType0C.ptr());

			descriptor->add(names::FontFile3, m_embedded_contents);
		}

		// make a cmap to use for ToUnicode
		m_unicode_cmap = Stream::create(doc, {});
		m_unicode_cmap->setCompressed(true);

		// finally, construct the top-level Type0 font.
		m_font_dictionary = Dictionary::createIndirect(doc, names::Font, {});
		m_font_dictionary->add(names::Subtype, names::Type0.ptr());
		m_font_dictionary->add(names::BaseFont, basefont_name);

		// TODO: here's the thing about CMap. if we use Identity-H then we don't need a CMap, but it forces the text
		// stream to contain glyph IDs instead of anything sensible.
		m_font_dictionary->add(names::Encoding, Name::create("Identity-H"));

		// add the unicode map so text selection isn't completely broken
		m_font_dictionary->add(names::ToUnicode, m_unicode_cmap);
		m_font_dictionary->add(names::DescendantFonts, Array::create(IndirectRef::create(cidfont_dict)));
	}



	Size2d_YDown PdfFont::getWordSize(zst::wstr_view text, PdfScalar font_size) const
	{
		auto sz = m_source->getWordSize(text);
		return Size2d_YDown(                                 //
		    this->scaleMetricForFontSize(sz.x(), font_size), //
		    this->scaleMetricForFontSize(sz.y(), font_size));
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
	    const font::FeatureSet& features) const
	{
		return m_source->getPositioningAdjustmentsForGlyphSequence(glyphs, features);
	}

	std::optional<std::vector<GlyphId>> PdfFont::performSubstitutionsForGlyphSequence(zst::span<GlyphId> glyphs,
	    const font::FeatureSet& features) const
	{
		auto subst = m_source->performSubstitutionsForGlyphSequence(glyphs, features);
		if(not subst.has_value())
			return std::nullopt;

		auto& cmap = m_source->characterMapping();
		for(auto& [out, in] : subst->mapping.replacements)
		{
			m_source->markGlyphAsUsed(out);

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
			m_source->markGlyphAsUsed(out);
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
			m_source->markGlyphAsUsed(g);

			if(cmap.reverse.find(g) == cmap.reverse.end())
				this->addGlyphUnicodeMapping(g, find_codepoint_for_gid(g));
		}

		return std::move(subst->glyphs);
	}



	std::unique_ptr<PdfFont> PdfFont::fromSource(File* doc, std::unique_ptr<font::FontSource> font)
	{
		auto ptr = font.release();
		if(auto x = dynamic_cast<font::FontFile*>(ptr); x != nullptr)
			return std::unique_ptr<PdfFont>(new PdfFont(doc, std::unique_ptr<font::FontFile>(x)));

		else if(auto x = dynamic_cast<pdf::BuiltinFont*>(ptr); x != nullptr)
			return std::unique_ptr<PdfFont>(new PdfFont(doc, std::unique_ptr<pdf::BuiltinFont>(x)));

		else
			sap::internal_error("unsupported font source");
	}

	std::unique_ptr<PdfFont> PdfFont::fromBuiltin(File* doc, BuiltinFont::Core14 font_name)
	{
		return std::unique_ptr<PdfFont>(new PdfFont(doc, BuiltinFont::get(font_name)));
	}
}
