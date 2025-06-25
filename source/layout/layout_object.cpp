// layout_object.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"

#include "layout/base.h"
#include "layout/line.h"
#include "layout/document.h"
#include "layout/layout_object.h"

namespace sap::layout
{
	LayoutObject::LayoutObject(const Style& style, LayoutSize size) : Stylable(style), m_layout_size(size)
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
		else if(this->isAbsolutelyPositioned())
			return m_position->right();
		else
			sap::internal_error("layout object was not positioned!");
	}

	void LayoutObject::setLinkDestination(tree::LinkDestination dest)
	{
		m_link_destination = std::move(dest);
	}

	tree::LinkDestination LayoutObject::linkDestination() const
	{
		return m_link_destination;
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
		if(std::holds_alternative<std::monostate>(m_link_destination))
			return;

		static constexpr double POS_OFFSET_SCALE = 2.5;

		AbsolutePagePos dest_pos {};
		if(auto app = std::get_if<AbsolutePagePos>(&m_link_destination); app)
		{
			dest_pos = *app;
		}
		else
		{
			LayoutObject* lo = nullptr;
			if(auto tbo = std::get_if<tree::BlockObject*>(&m_link_destination); tbo)
			{
				if(auto tmp = (*tbo)->getGeneratedLayoutObject(); not tmp.has_value())
					sap::internal_error("link destination did not generate a layout object!");
				else
					lo = *tmp;
			}
			else if(auto tmp = std::get_if<LayoutObject*>(&m_link_destination); tmp)
			{
				lo = *tmp;
			}

			dest_pos = lo->resolveAbsPosition(layout);
			dest_pos.pos.y() -= POS_OFFSET_SCALE * lo->layoutSize().ascent;
		}

		auto pos = this->resolveAbsPosition(layout);
		pos.pos.y() += m_layout_size.descent;

		layout->document()->addAnnotation(LinkAnnotation {
		    .position = pos,
		    .size = Size2d { m_layout_size.width, m_layout_size.total_height() },
		    .destination = dest_pos,
		});
	}




	void* LayoutObject::operator new(size_t count)
	{
		thread_local static auto pool = util::Pool<uint8_t, /* region size: */ (1 << 16)>();
		return pool.allocate_raw(count);
	}

	void LayoutObject::operator delete(void* ptr, size_t count)
	{
	}


	LayoutSpan::LayoutSpan(Length relative_offset, Length raise_height, LayoutSize size)
	    : LayoutObject(Style::empty(), size), m_relative_offset(relative_offset), m_raise_height(raise_height)
	{
	}

	layout::PageCursor LayoutSpan::compute_position_impl(layout::PageCursor cursor)
	{
		// does nothing
		this->positionRelatively(cursor.position());
		return cursor;
	}

	void LayoutSpan::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		// do nothing (for now)
	}
}
