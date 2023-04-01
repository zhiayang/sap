// spacer.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/spacer.h"
#include "layout/spacer.h"
#include "interp/interp.h"

namespace sap::tree
{
	Spacer::Spacer(DynLength2d size) : m_size(size)
	{
	}

	ErrorOr<void> Spacer::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	auto Spacer::create_layout_object_impl(interp::Interpreter* cs, const Style* parent_style, Size2d available_space)
		const -> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);

		auto cur_style = m_style->useDefaultsFrom(parent_style)->useDefaultsFrom(cs->evaluator().currentStyle());
		auto size = m_size.resolve(cur_style);

		auto layout_size = LayoutSize {
			.width = size.x(),
			.ascent = 0,
			.descent = size.y(),
		};

		auto ret = std::make_unique<layout::Spacer>(cur_style, layout_size);
		m_generated_layout_object = ret.get();

		return Ok(LayoutResult::make(std::move(ret)));
	}
}
