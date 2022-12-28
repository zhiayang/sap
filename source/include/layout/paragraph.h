// paragraph.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "layout/base.h"
#include "layout/word.h"

namespace sap::layout
{
	struct PositionedWord
	{
		Word word;
		const pdf::Font* font;
		pdf::PdfScalar font_size;
		Cursor start;
		Cursor end;
	};

	// for now we are not concerned with lines.
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
