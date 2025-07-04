// spacer.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tree/spacer.h"
#include "layout/spacer.h"
#include "interp/interp.h"

namespace sap::tree
{
	Spacer::Spacer(DynLength2d size, bool page_break)
	    : BlockObject(Kind::Spacer), m_size(size), m_page_break(page_break)
	{
	}

	ErrorOr<void> Spacer::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	auto Spacer::create_layout_object_impl(interp::Interpreter* cs, const Style& parent_style, Size2d available_space)
	    const -> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto cur_style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());
		auto size = m_size.resolve(cur_style);

		auto layout_size = LayoutSize {
			.width = size.x(),
			.ascent = 0,
			.descent = m_page_break ? Length(INFINITY) : size.y(),
		};

		auto ret = std::make_unique<layout::Spacer>(cur_style, layout_size);
		return Ok(LayoutResult::make(std::move(ret)));
	}
}
