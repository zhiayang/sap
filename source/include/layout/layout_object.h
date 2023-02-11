// layout_object.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/style.h"
#include "layout/cursor.h"

namespace pdf
{
	struct Page;
}

namespace sap::layout
{
	struct LayoutBase;
	struct PageLayout;

	struct LayoutObject : Stylable
	{
		explicit LayoutObject(const Style* style, LayoutSize size);
		virtual ~LayoutObject() = default;

		LayoutObject& operator=(LayoutObject&&) = default;
		LayoutObject(LayoutObject&&) = default;

		LayoutSize layoutSize() const;

		void positionAbsolutely(AbsolutePagePos pos);
		void positionRelatively(RelativePos pos);

		bool isPositioned() const;
		bool isAbsolutelyPositioned() const;
		bool isRelativelyPositioned() const;

		RelativePos relativePosition() const;
		AbsolutePagePos absolutePosition() const;
		AbsolutePagePos resolveAbsPosition(const LayoutBase* layout) const;

		void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const;

		virtual layout::PageCursor positionChildren(layout::PageCursor cursor) = 0;

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by constructing and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const = 0;

	protected:
		std::optional<AbsolutePagePos> m_abs_position {};
		std::optional<RelativePos> m_rel_position {};

		LayoutSize m_layout_size;
	};
}
