// image.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "layout/base.h"

namespace sap
{
	struct Style;
	namespace tree
	{
		struct Image;
		struct DocumentObject;
	}
}

namespace sap::layout
{
	struct Image : LayoutObject
	{
		using LayoutObject::LayoutObject;

		virtual layout::PageCursor positionChildren(layout::PageCursor cursor) override;
		virtual void render_impl(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		friend struct tree::Image;

		explicit Image(const Style* style, LayoutSize size, ImageBitmap m_image);
		ImageBitmap m_image;
	};
}
