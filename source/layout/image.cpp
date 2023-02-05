// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/xobject.h"

#include "tree/image.h"

#include "layout/image.h"

namespace sap::layout
{
	Image::Image(Size2d size, ImageBitmap image) : LayoutObject(size), m_image(std::move(image))
	{
	}

	void Image::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		auto pos = this->resolveAbsPosition(layout);
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
	ErrorOr<void> Image::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	auto Image::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		// images are always fixed size, and don't care about the available space.
		// (for now, at least...)

		auto img = std::unique_ptr<layout::Image>(new layout::Image(m_size, this->image()));
		img->setStyle(parent_style);

		return Ok(LayoutResult::make(std::move(img)));
	}
}
