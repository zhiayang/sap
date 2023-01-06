// paragraph.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar

#include "sap/style.h" // for Style

#include "interp/tree.h" // for Paragraph

#include "layout/base.h" // for Cursor, LayoutObject, RectPageLayout, Inter...
#include "layout/word.h" // for Word

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

		static std::pair<std::optional<const tree::Paragraph*>, Cursor> layout(interp::Interpreter* cs, RectPageLayout* layout,
		    Cursor cursor, const Style* parent_style, const tree::Paragraph* treepara);
		virtual void render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		std::vector<PositionedWord> m_words {};
	};
}
