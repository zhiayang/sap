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

		static Cursor fromTree(interp::Interpreter* cs, RectPageLayout* layout, Cursor cursor, const Style* parent_style,
		    const tree::DocumentObject* obj);

		virtual void render(const RectPageLayout* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		explicit Image(zst::byte_span m_image_data, size_t m_pixel_width, size_t m_pixel_height);

		Cursor m_position {};
		Size2d m_size {};

		zst::byte_span m_image_data;
		size_t m_pixel_width;
		size_t m_pixel_height;
	};
}
