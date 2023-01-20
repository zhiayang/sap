// image.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct Image : BlockObject
	{
		explicit Image(OwnedImageBitmap image, sap::Vector2 size);

		virtual LayoutResult createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style)
		    const override;

		static std::unique_ptr<Image> fromImageFile(zst::str_view file_path,
		    sap::Length width,
		    std::optional<sap::Length> height = std::nullopt);

		ImageBitmap image() const { return m_image.span(); }

	private:
		OwnedImageBitmap m_image;
	};
}
