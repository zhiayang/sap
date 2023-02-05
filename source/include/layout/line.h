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

	struct LineMetrics
	{
		sap::Length total_space_width;
		sap::Length total_word_width;
		std::vector<sap::Length> widths;

		sap::Length ascent_height;
		sap::Length descent_height;
		sap::Length default_line_spacing;
	};

	struct Line : LayoutObject
	{
		virtual void render(const LayoutBase* layout, std::vector<pdf::Page*>& pages) const override;

		static std::unique_ptr<Line> fromInlineObjects(interp::Interpreter* cs,
			const Style* style,
			std::span<const std::unique_ptr<tree::InlineObject>> objs,
			const LineMetrics& line_metrics,
			Size2d available_space,
			bool is_first_line,
			bool is_last_line);

		const std::vector<std::unique_ptr<LayoutObject>>& objects() const { return m_objects; }

	private:
		explicit Line(Size2d size, const Style* style, std::vector<std::unique_ptr<LayoutObject>> objs);

	private:
		std::vector<std::unique_ptr<LayoutObject>> m_objects;
	};

	LineMetrics computeLineMetrics(std::span<const std::unique_ptr<tree::InlineObject>> objs, const Style* parent_style);
}
