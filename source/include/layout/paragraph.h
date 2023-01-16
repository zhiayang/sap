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
	struct Paragraph : LayoutObject
	{
		using LayoutObject::LayoutObject;

		static std::unique_ptr<Paragraph> fromLines(interp::Interpreter* cs,
		    LineCursor cursor,
		    const Style* parent_style,
		    std::vector<std::unique_ptr<Line>> lines);

		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		friend struct tree::Paragraph;
		Paragraph(RelativePos pos, Size2d size, std::vector<std::unique_ptr<Line>> lines);

	private:
		std::vector<std::unique_ptr<Line>> m_lines {};
	};
}
