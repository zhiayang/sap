// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/xobject.h"

#include "tree/image.h"

#include "layout/image.h"

namespace sap::layout
{
	Image::Image(const Style* style, LayoutSize size, ImageBitmap image)
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


namespace sap::tree
{
	ErrorOr<void> Image::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	auto Image::create_layout_object_impl(interp::Interpreter* cs, const Style* parent_style, Size2d available_space)
		const -> ErrorOr<LayoutResult>
	{
		// images are always fixed size, and don't care about the available space.
		// (for now, at least...)

		auto layout_size = LayoutSize { .width = m_size.x(), .ascent = 0, .descent = m_size.y() };
		auto img = std::unique_ptr<layout::Image>(new layout::Image(parent_style, layout_size, this->image()));

		m_generated_layout_object = img.get();
		return Ok(LayoutResult::make(std::move(img)));
	}
}
