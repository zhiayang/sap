// image.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "tree/base.h"

namespace sap::tree
{
	struct Image : BlockObject
	{
		explicit Image(OwnedImageBitmap image, Size2d size);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		virtual ErrorOr<LayoutResult> createLayoutObject(interp::Interpreter* cs,
			const Style* parent_style,
			Size2d available_space) const override;

		static std::unique_ptr<Image> fromImageFile(zst::str_view file_path,
			sap::Length width,
			std::optional<sap::Length> height = std::nullopt);

		ImageBitmap image() const { return m_image.span(); }

	private:
		OwnedImageBitmap m_image;
		Size2d m_size;
	};
}
