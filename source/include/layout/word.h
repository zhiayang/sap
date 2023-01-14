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
		Word(zst::wstr_view text, const Style* style, RelativePos position, Size2d size);

		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		zst::wstr_view text() const { return m_text; }

	private:
		void line_render(pdf::Text* text) const;
		friend struct Line;

	private:
		zst::wstr_view m_text {};
	};
}
