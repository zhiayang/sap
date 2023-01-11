// paragraph.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar

#include "layout/base.h" // for Cursor, LayoutObject, RectPageLayout, Inter...
#include "layout/word.h" // for Word

namespace sap
{
	struct Style;
	namespace tree
	{
		struct Paragraph;
		struct DocumentObject;
	}
}

namespace sap::layout
{
	struct PositionedWord
	{
		Word word;
		const pdf::PdfFont* font;
		pdf::PdfScalar font_size;
		Cursor start;
		Cursor end;
	};

	struct Paragraph : LayoutObject
	{
		using LayoutObject::LayoutObject;

		static Cursor fromTree(interp::Interpreter* cs, RectPageLayout* layout, Cursor cursor, const Style* parent_style,
		    const tree::DocumentObject* obj);

		virtual void render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		std::vector<PositionedWord> m_words {};
	};
}
