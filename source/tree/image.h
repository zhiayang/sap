// image.h
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct Image : BlockObject
	{
		explicit Image(ImageBitmap image, Size2d size);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		static ErrorOr<zst::SharedPtr<Image>> fromImageFile(const Location& loc,
		    zst::str_view file_path,
		    sap::Length width,
		    std::optional<sap::Length> height = std::nullopt);

		ImageBitmap image() const { return m_image; }

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		ImageBitmap m_image;
		Size2d m_size;
	};
}
