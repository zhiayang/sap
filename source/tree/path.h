// path.h
// Copyright (c) 2023, yuki
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/path.h"
#include "tree/base.h"

namespace sap::layout
{
	struct Path;
}

namespace sap::tree
{
	struct Path : BlockObject
	{
		explicit Path(PathStyle path_style, std::vector<PathSegment> segments);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;
		ErrorOr<layout::Path> createLayoutObjectWithoutInterp(const Style& final_style) const;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		PathStyle m_path_style;
		std::shared_ptr<PathSegments> m_segments;
	};
}
