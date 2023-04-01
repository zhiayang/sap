// word.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h" // for Stylable, Style
#include "sap/units.h" // for Length

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
		Word(zst::wstr_view text, const Style* style, Length relative_offset, LayoutSize size);

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
		zst::wstr_view m_text {};
	};
}
