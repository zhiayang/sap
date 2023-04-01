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
		m_position = Right(std::move(pos));
	}

	void LayoutObject::positionRelatively(RelativePos pos)
	{
		m_position = Left(std::move(pos));
	}

	bool LayoutObject::isPositioned() const
	{
		return m_position.has_value();
	}

	bool LayoutObject::isAbsolutelyPositioned() const
	{
		return m_position.has_value() && m_position->is_right();
	}

	bool LayoutObject::isRelativelyPositioned() const
	{
		return m_position.has_value() && m_position->is_left();
	}

	AbsolutePagePos LayoutObject::absolutePosition() const
	{
		assert(this->isAbsolutelyPositioned());
		return m_position->right();
	}

	RelativePos LayoutObject::relativePosition() const
	{
		assert(this->isRelativelyPositioned());
		return m_position->left();
	}

	AbsolutePagePos LayoutObject::resolveAbsPosition(const LayoutBase* layout) const
	{
		if(this->isRelativelyPositioned())
			return layout->convertPosition(m_position->left());
		else
			return m_position->right();
	}

	void LayoutObject::overrideLayoutSizeX(Length x)
	{
		m_layout_size.width = x;
	}

	void LayoutObject::overrideLayoutSizeY(Length y)
	{
		m_layout_size.descent = y;
	}


	void LayoutObject::overrideAbsolutePosition(AbsolutePagePos pos)
	{
		m_absolute_pos_override = std::move(pos);
	}

	void LayoutObject::addRelativePositionOffset(Size2d offset)
	{
		m_relative_pos_offset = std::move(offset);
	}

	PageCursor LayoutObject::computePosition(PageCursor cursor)
	{
		if(m_absolute_pos_override.has_value())
		{
			cursor = cursor.moveToPosition(RelativePos {
				.pos = RelativePos::Pos(m_absolute_pos_override->pos.x(), m_absolute_pos_override->pos.y()),
				.page_num = m_absolute_pos_override->page_num,
			});
		}

		if(m_relative_pos_offset.has_value())
		{
			cursor = cursor.moveRight(m_relative_pos_offset->x());
			cursor = cursor.newLine(m_relative_pos_offset->y());
		}

		auto ret = this->compute_position_impl(cursor);
		if(not m_position.has_value())
			sap::internal_error("compute_position_impl did not set a position!");

		return ret;
	}

	void LayoutObject::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		if(not this->isPositioned())
			sap::internal_error("cannot render without position! ({})", (void*) this);

		this->render_impl(layout, pages);
	}
}
