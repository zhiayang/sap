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

	struct LayoutObject : Stylable
	{
		LayoutObject(RelativePos position, Size2d size) : m_layout_position(std::move(position)), m_layout_size(size) { }
		virtual ~LayoutObject() = default;

		LayoutObject& operator=(LayoutObject&&) = default;
		LayoutObject(LayoutObject&&) = default;

		Size2d layoutSize() const { return m_layout_size; }
		RelativePos layoutPosition() const { return m_layout_position; }

		/*
		    Render (emit PDF commands) the object. Must be called after layout(). For now, we render directly to
		    the PDF page (by construcitng and emitting PageObjects), instead of returning a pageobject -- some
		    layout objects might require multiple pdf page objects, so this is a more flexible design.
		*/
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const = 0;

	protected:
		RelativePos m_layout_position;
		Size2d m_layout_size;
	};
}
