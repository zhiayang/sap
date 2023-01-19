// line.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <span>

#include "layout/base.h"

namespace sap::tree
{
	struct BlockObject;
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

		static std::pair<LineCursor, std::unique_ptr<Line>> fromInlineObjects(interp::Interpreter* cs,
		    LineCursor cursor,
		    const linebreak::BrokenLine& broken_line,
		    const Style* style,
		    std::span<const std::unique_ptr<tree::InlineObject>> objs,
		    bool is_last_line);

		static std::pair<LineCursor, std::optional<std::unique_ptr<Line>>> fromBlockObjects(interp::Interpreter* cs,
		    LineCursor cursor,
		    const Style* style,
		    std::span<tree::BlockObject*> objs);

		const std::vector<std::unique_ptr<LayoutObject>>& objects() const { return m_objects; }

	private:
		explicit Line(RelativePos position, Size2d size, const Style* style, std::vector<std::unique_ptr<LayoutObject>> objs);

	private:
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};
}
