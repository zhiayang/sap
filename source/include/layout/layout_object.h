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
		LayoutObject(Size2d size) : m_layout_size(size) { }
		virtual ~LayoutObject() = default;

		LayoutObject& operator=(LayoutObject&&) = default;
		LayoutObject(LayoutObject&&) = default;

		Size2d layoutSize() const { return m_layout_size; }

		void positionAbsolutely(AbsolutePagePos pos) { m_abs_position = std::move(pos); }
		void positionRelatively(RelativePos pos) { m_rel_position = std::move(pos); }

		bool isPositioned() const { return m_abs_position.has_value() || m_rel_position.has_value(); }
		bool isAbsolutelyPositioned() const { return m_abs_position.has_value(); }
		bool isRelativelyPositioned() const { return m_rel_position.has_value() && not m_abs_position.has_value(); }

		AbsolutePagePos absolutePosition() const
		{
			assert(this->isAbsolutelyPositioned());
			return *m_abs_position;
		}

		RelativePos relativePosition() const
		{
			assert(this->isRelativelyPositioned());
			return *m_rel_position;
		}

		AbsolutePagePos resolveAbsPosition(const LayoutBase* layout) const;

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by construcitng and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const = 0;

	protected:
		std::optional<AbsolutePagePos> m_abs_position {};
		std::optional<RelativePos> m_rel_position {};

		Size2d m_layout_size;
	};
}
