// paragraph.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"  // for Font
#include "pdf/units.h" // for PdfScalar

#include "layout/base.h" // for Cursor, LayoutObject, RectPageLayout, Inter...
#include "layout/word.h" // for Word
#include "layout/line.h"

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
#if 0
	struct PositionedWord
	{
		Word word;
		const pdf::PdfFont* font;
		pdf::PdfScalar font_size;
		RelativePos start;
		RelativePos end;
	};
#endif

	struct Paragraph : LayoutObject
	{
		using LayoutObject::LayoutObject;

		static LineCursor fromTree(interp::Interpreter* cs, LayoutBase* layout, LineCursor cursor, const Style* parent_style,
		    const tree::DocumentObject* obj);

		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		Paragraph(RelativePos pos, Size2d size, std::vector<std::unique_ptr<Line>> lines);

		std::vector<std::unique_ptr<Line>> m_lines {};
	};
}
