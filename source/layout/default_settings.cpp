// default_settings.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/font.h"

#include "sap/font_family.h"
#include "sap/document_settings.h"

#include "interp/interp.h"

namespace sap::layout
{
	DocumentSettings fillDefaultSettings(interp::Interpreter* cs, DocumentSettings settings)
	{
		auto mm = [](auto x) {
			return DynLength(x, DynLength::MM);
		};

		auto pt = [](auto x) {
			return sap::Length(25.4 * (x / 72.0));
		};

		using Margins = DocumentSettings::Margins;

		settings.font_size = settings.font_size.value_or(DynLength(12, DynLength::PT));
		settings.paper_size = settings.paper_size.value_or(DynLength2d { mm(210), mm(297) });
		settings.line_spacing = settings.line_spacing.value_or(1.0);
		settings.paragraph_spacing = settings.paragraph_spacing.value_or(DynLength(0, DynLength::MM));

		auto paper_width = settings.paper_size->x.resolveWithoutFont(pt(12), pt(12));
		auto paper_height = settings.paper_size->y.resolveWithoutFont(pt(12), pt(12));

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
		{
			settings.font_family = sap::FontFamily(                                           //
			    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesRoman)),  //
			    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesItalic)), //
			    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesBold)),   //
			    &cs->addLoadedFont(pdf::PdfFont::fromBuiltin(pdf::BuiltinFont::TimesBoldItalic)));
		}

		return settings;
	}
}
