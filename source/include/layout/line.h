// line.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>

#include "layout/base.h"

namespace sap::tree
{
	struct InlineObject;
}

namespace sap::layout
{
	namespace linebreak
	{
		struct BrokenLine;
	}

	struct Line : LayoutObject
	{
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		static std::unique_ptr<Line> fromInlineObjects(LineCursor cursor, const linebreak::BrokenLine& broken_line,
		    const Style* style, std::span<const std::unique_ptr<tree::InlineObject>> objs);

	private:
		explicit Line(std::vector<std::unique_ptr<LayoutObject>> objs, RelativePos position);

	private:
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};
}
