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

		static LineCursor fromTree(interp::Interpreter* cs, LayoutBase* layout, LineCursor cursor, const Style* parent_style,
		    const tree::DocumentObject* obj);

		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

	private:
		explicit Image(ImageBitmap m_image);

		Size2d m_size {};
		ImageBitmap m_image;
	};
}
