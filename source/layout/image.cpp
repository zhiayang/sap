// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/xobject.h"

#include "tree/image.h"

#include "layout/image.h"

namespace sap::layout
{
	Image::Image(RelativePos pos, Size2d size, ImageBitmap image) : LayoutObject(pos, size), m_image(std::move(image))
	{
	}

	void Image::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		auto pos = layout->convertPosition(m_layout_position);
		auto page = pages[pos.page_num];

		auto page_obj = util::make<pdf::Image>( //
		    m_image,                            //
		    m_layout_size.into(),               //
		    page->convertVector2(pos.pos.into<pdf::Position2d_YDown>()));

		page->addObject(page_obj);
	}
}


namespace sap::tree
{
	auto Image::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		auto sz = this->size(cursor);

		cursor = cursor.ensureVerticalSpace(sz.y());

		auto img = std::unique_ptr<layout::Image>(new layout::Image(cursor.position(), sz, this->image()));
		img->setStyle(parent_style);

		cursor = cursor.newLine(sz.y());
		cursor = cursor.moveRight(sz.x());

		return LayoutResult::make(cursor, std::move(img));
	}
}
