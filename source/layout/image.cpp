// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/xobject.h"

#include "interp/tree.h"

#include "layout/image.h"

namespace sap::layout
{
	Image::Image(zst::byte_span image_data, size_t pixel_width, size_t pixel_height)
	    : m_image_data(image_data)
	    , m_pixel_width(pixel_width)
	    , m_pixel_height(pixel_height)
	{
	}

	Cursor Image::fromTree(interp::Interpreter* cs, RectPageLayout* layout, Cursor cursor, const Style* parent_style,
	    const tree::DocumentObject* doc_obj)
	{
		auto tree_img = static_cast<const tree::Image*>(doc_obj);

		auto img = std::unique_ptr<Image>(new Image(tree_img->span(), tree_img->pixelWidth(), tree_img->pixelHeight()));
		img->m_size = tree_img->size();
		img->m_position = cursor;
		img->setStyle(parent_style);

		layout->addObject(std::move(img));
		return layout->newLineFrom(cursor, tree_img->size().y());
	}

	void Image::render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const
	{
		auto page = pages[m_position.page_num];
		auto page_obj = util::make<pdf::Image>(
		    pdf::Image::Data {
		        .bytes = m_image_data,
		        .width = m_pixel_width,
		        .height = m_pixel_height,
		        .bits_per_pixel = 8,
		    },
		    m_size.x().into(), m_size.y().into());

		page->addObject(page_obj);
	}
}
