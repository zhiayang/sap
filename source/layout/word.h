// word.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h"
#include "sap/units.h"

#include "tree/paragraph.h"

#include "layout/base.h"

namespace pdf
{
	struct Text;
}

namespace sap::layout
{
	struct Word : LayoutObject
	{
		Word(std::u32string text, const Style& style, Length relative_offset, Length raise_height, LayoutSize size);

		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		zst::wstr_view text() const { return m_text; }

		Length relativeOffset() const { return m_relative_offset; }

	private:
		void render(AbsolutePagePos line_pos,
		    std::vector<pdf::Page*>& pages,
		    pdf::Text* text,
		    bool is_first_in_text,
		    Length offset_from_prev) const;

		void render_to_text(pdf::Text* text) const;

		friend struct Line;

	private:
		Length m_relative_offset {};
		Length m_raise_height {};
		std::u32string m_text {};
	};
}
