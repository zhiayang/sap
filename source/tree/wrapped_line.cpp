// wrapped_line.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "tree/wrappers.h"
#include "tree/paragraph.h"

#include "layout/line.h"
#include "interp/interp.h"

namespace sap::tree
{
	ErrorOr<void> WrappedLine::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	WrappedLine::WrappedLine(std::vector<zst::SharedPtr<InlineObject>> objs)
	    : BlockObject(Kind::WrappedLine), m_objects(std::move(objs))
	{
	}

	void WrappedLine::addObjects(std::vector<zst::SharedPtr<InlineObject>> objs)
	{
		std::move(objs.begin(), objs.end(), std::back_inserter(m_objects));
	}

	auto WrappedLine::create_layout_object_impl(interp::Interpreter* cs, //
	    const Style& parent_style,
	    Size2d available_space) const -> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);
		auto cur_style = m_style.useDefaultsFrom(parent_style).useDefaultsFrom(cs->evaluator().currentStyle());

		auto objs = TRY(tree::processWordSeparators(m_objects));
		objs = TRY(tree::performReplacements(cur_style, std::move(objs)));

		auto line = layout::Line::fromInlineObjects(cs, cur_style, m_objects, available_space,
		    /* is_first: */ true, /* is_last: */ true);

		return Ok(LayoutResult::make(std::move(line)));
	}
}
