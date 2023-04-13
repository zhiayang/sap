// default_settings.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"

#include "sap/font_family.h"
#include "sap/document_settings.h"

#include "interp/interp.h"

namespace sap::layout
{
	static FontFamily get_default_font_family(interp::Interpreter* cs)
	{
		static auto ret = sap::FontFamily(                                                //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesRoman)),  //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesItalic)), //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesBold)),   //
		    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesBoldItalic)));

		return ret;
	}

	static constexpr double DEFAULT_FONT_SIZE_PT = 11;
	static constexpr double DEFAULT_LINE_SPACING = 1.0;
	static constexpr double DEFAULT_SENTENCE_SPACE_STRETCH = 1.5;

	const Style* getDefaultStyle(interp::Interpreter* cs)
	{
		auto default_style = util::make<sap::Style>();

		auto font_size = pdf::PdfScalar(DEFAULT_FONT_SIZE_PT).into();
		default_style->set_font_family(get_default_font_family(cs))
		    .set_font_style(sap::FontStyle::Regular)
		    .set_font_size(font_size)
		    .set_root_font_size(font_size)
		    .set_line_spacing(DEFAULT_LINE_SPACING)
		    .set_sentence_space_stretch(DEFAULT_SENTENCE_SPACE_STRETCH)
		    .set_paragraph_spacing(Length(0))
		    .set_alignment(Alignment::Justified)
		    .enable_smart_quotes(true);

		return default_style;
	}

	DocumentSettings fillDefaultSettings(interp::Interpreter* cs, DocumentSettings settings)
	{
		auto pt = [](auto x) { return Length(x); };
		auto mm = [](auto x) { return DynLength(x, DynLength::MM); };

		using Margins = DocumentSettings::Margins;

		settings.font_size = settings.font_size.value_or(DynLength(DEFAULT_FONT_SIZE_PT, DynLength::PT));
		settings.paper_size = settings.paper_size.value_or(DynLength2d { mm(210), mm(297) });
		settings.line_spacing = settings.line_spacing.value_or(DEFAULT_LINE_SPACING);
		settings.sentence_space_stretch = settings.sentence_space_stretch.value_or(DEFAULT_SENTENCE_SPACE_STRETCH);
		settings.paragraph_spacing = settings.paragraph_spacing.value_or(DynLength(0, DynLength::MM));

		constexpr auto FONT_SZ = DEFAULT_FONT_SIZE_PT;
		auto paper_width = settings.paper_size->x.resolveWithoutFont(pt(FONT_SZ), pt(FONT_SZ));
		auto paper_height = settings.paper_size->y.resolveWithoutFont(pt(FONT_SZ), pt(FONT_SZ));

		if(settings.margins.has_value())
		{
			auto& m = *settings.margins;
			if(not m.top.has_value())
			{
				if(m.bottom.has_value())
					m.top = *m.bottom;
				else if(m.left.has_value())
					m.top = *m.left;
				else if(m.right.has_value())
					m.top = *m.right;
				else
					m.top = DynLength(paper_height * 0.15);
			}
			if(not m.bottom.has_value())
			{
				if(m.top.has_value())
					m.bottom = *m.top;
				else if(m.left.has_value())
					m.bottom = *m.left;
				else if(m.right.has_value())
					m.bottom = *m.right;
				else
					m.bottom = DynLength(paper_height * 0.15);
			}
			if(not m.left.has_value())
			{
				if(m.right.has_value())
					m.left = *m.right;
				else if(m.top.has_value())
					m.left = *m.top;
				else if(m.bottom.has_value())
					m.left = *m.bottom;
				else
					m.left = DynLength(paper_width * 0.15);
			}
			if(not m.right.has_value())
			{
				if(m.left.has_value())
					m.right = *m.left;
				else if(m.top.has_value())
					m.right = *m.top;
				else if(m.bottom.has_value())
					m.right = *m.bottom;
				else
					m.right = DynLength(paper_width * 0.15);
			}
		}
		else
		{
			settings.margins = Margins {
				.top = DynLength(paper_height * 0.15),
				.bottom = DynLength(paper_height * 0.15),
				.left = DynLength(paper_width * 0.15),
				.right = DynLength(paper_width * 0.15),
			};
		}

		if(not settings.font_family.has_value())
			settings.font_family = get_default_font_family(cs);

		return settings;
	}
}

namespace sap
{
	static Length pt(double x)
	{
		return sap::Length(25.4 * (x / 72.0));
	}

	Length DocumentSettings::DEFAULT_FONT_SIZE = pt(layout::DEFAULT_FONT_SIZE_PT);
}
