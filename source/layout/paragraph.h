// paragraph.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pdf/font.h"
#include "pdf/units.h"

#include "layout/base.h"
#include "layout/word.h"
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

		virtual layout::PageCursor compute_position_impl(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		friend struct tree::Paragraph;
		Paragraph(const Style& style,
		    LayoutSize size,
		    std::vector<std::unique_ptr<Line>> lines,
		    std::vector<zst::SharedPtr<tree::InlineObject>> para_inline_objs);

	private:
		std::vector<std::unique_ptr<Line>> m_lines {};
		std::vector<zst::SharedPtr<tree::InlineObject>> m_para_inline_objs;
	};
}
