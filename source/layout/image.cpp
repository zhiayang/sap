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
		zpr::println("img pos = {}", pos.pos);

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
	auto Image::createLayoutObject(interp::Interpreter* cs, layout::LineCursor cursor, const Style* parent_style) const
	    -> LayoutResult
	{
		auto img = std::unique_ptr<layout::Image>(new layout::Image(cursor.position(), this->size(), this->image()));
		img->setStyle(parent_style);

		// cursor = cursor.newLine(this->size().y());
		cursor = cursor.moveRight(this->size().x());

		return { cursor, std::move(img) };
	}
}
