// image.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/xobject.h"

#include "tree/image.h"

#include "layout/image.h"

namespace sap::layout
{
	Image::Image(const Style& style, LayoutSize size, ImageBitmap image)
	    : LayoutObject(style, size), m_image(std::move(image))
	{
	}

	bool Image::requires_space_reservation() const
	{
		return true;
	}

	layout::PageCursor Image::compute_position_impl(layout::PageCursor cursor)
	{
		this->positionRelatively(cursor.position());
		return cursor.moveRight(m_layout_size.width).newLine(m_layout_size.descent);
	}

	void Image::render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		if(not sap::isDraftMode())
		{
			auto pos = this->resolveAbsPosition(layout);
			auto page = pages[pos.page_num];

			auto pdf_size = pdf::Size2d(m_layout_size.width.into(), m_layout_size.total_height().into());

			auto page_obj = util::make<pdf::Image>( //
			    m_image,                            //
			    pdf_size,                           //
			    page->convertVector2(pos.pos.into<pdf::Position2d_YDown>()));

			page->addObject(page_obj);
		}
	}
}
