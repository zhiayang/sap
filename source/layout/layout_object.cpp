// layout_object.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "layout/base.h"
#include "layout/layout_object.h"

namespace sap::layout
{
	LayoutObject::LayoutObject(const Style* style, LayoutSize size) : Stylable(style), m_layout_size(size)
	{
	}

	LayoutSize LayoutObject::layoutSize() const
	{
		return m_layout_size;
	}

	void LayoutObject::positionAbsolutely(AbsolutePagePos pos)
	{
		m_abs_position = std::move(pos);
	}

	void LayoutObject::positionRelatively(RelativePos pos)
	{
		m_rel_position = std::move(pos);
	}

	bool LayoutObject::isPositioned() const
	{
		return m_abs_position.has_value() || m_rel_position.has_value();
	}

	bool LayoutObject::isAbsolutelyPositioned() const
	{
		return m_abs_position.has_value();
	}

	bool LayoutObject::isRelativelyPositioned() const
	{
		return m_rel_position.has_value() && not m_abs_position.has_value();
	}

	AbsolutePagePos LayoutObject::absolutePosition() const
	{
		assert(this->isAbsolutelyPositioned());
		return *m_abs_position;
	}

	RelativePos LayoutObject::relativePosition() const
	{
		assert(this->isRelativelyPositioned());
		return *m_rel_position;
	}

	AbsolutePagePos LayoutObject::resolveAbsPosition(const LayoutBase* layout) const
	{
		if(this->isRelativelyPositioned())
			return layout->convertPosition(*m_rel_position);
		else
			return *m_abs_position;
	}

	void LayoutObject::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		if(not this->isPositioned())
			sap::internal_error("cannot render without position! ({})", (void*) this);

		this->render_impl(layout, pages);
	}
}
