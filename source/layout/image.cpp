// image.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "pdf/page.h"
#include "pdf/xobject.h"

#include "interp/tree.h"

#include "layout/image.h"

namespace sap::layout
{
	Image::Image(ImageBitmap image) : m_image(std::move(image))
	{
	}

	LineCursor Image::fromTree(interp::Interpreter* cs, LayoutBase* layout, LineCursor cursor, const Style* parent_style,
	    const tree::DocumentObject* doc_obj)
	{
		cursor = cursor.newLine(0);
		// cursor = layout->newLineFrom(cursor, 0);

		auto tree_img = static_cast<const tree::Image*>(doc_obj);

		auto img = std::unique_ptr<Image>(new Image(tree_img->image()));
		img->m_size = tree_img->size();
		img->m_position = cursor.position();
		img->setStyle(parent_style);

		layout->addObject(std::move(img));
		return cursor.newLine(tree_img->size().y());
	}

	void Image::render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const
	{
		auto page = pages[m_position.page_num];
		auto page_obj = util::make<pdf::Image>(m_image, m_size.into(),
		    page->convertVector2(m_position.pos.into<pdf::Position2d_YDown>()));

		page->addObject(page_obj);
	}
}
