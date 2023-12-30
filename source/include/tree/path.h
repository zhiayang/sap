// path.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sap/path.h"
#include "tree/base.h"

namespace sap::tree
{
	struct Path : BlockObject
	{
		explicit Path(std::vector<PathSegment> segments);

		virtual ErrorOr<void> evaluateScripts(interp::Interpreter* cs) const override;

	private:
		virtual ErrorOr<LayoutResult> create_layout_object_impl(interp::Interpreter* cs,
		    const Style& parent_style,
		    Size2d available_space) const override;

	private:
		std::shared_ptr<PathSegments> m_segments;
	};
}
