// path.cpp
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/path.h"
#include "layout/path.h"
#include "interp/interp.h"

namespace sap::tree
{
	Path::Path(std::vector<PathObject> segments) : BlockObject(Kind::Path), m_segments()
	{
		*m_segments = std::move(segments);
	}

	ErrorOr<void> Path::evaluateScripts(interp::Interpreter* cs) const
	{
		// no scripts to evaluate
		return Ok();
	}


	ErrorOr<LayoutResult>
	Path::create_layout_object_impl(interp::Interpreter* cs, const Style& parent_style, Size2d available_space) const
	{
		auto style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());

		return Ok(LayoutResult::make(std::make_unique<layout::Path>(style, LayoutSize {}, m_segments)));
	}
}
