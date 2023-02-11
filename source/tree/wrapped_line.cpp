// wrapped_line.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "tree/wrappers.h"

#include "layout/line.h"
#include "interp/interp.h"

namespace sap::tree
{
	ErrorOr<void> WrappedLine::evaluateScripts(interp::Interpreter* cs) const
	{
		return Ok();
	}

	void WrappedLine::addObjects(std::vector<std::unique_ptr<InlineObject>> objs)
	{
		m_objects.insert(m_objects.end(), std::move_iterator(objs.begin()), std::move_iterator(objs.end()));
	}

	auto WrappedLine::createLayoutObject(interp::Interpreter* cs, //
		const Style* parent_style,
		Size2d available_space) const -> ErrorOr<LayoutResult>
	{
		auto _ = cs->evaluator().pushBlockContext(this);
		auto cur_style = m_style->useDefaultsFrom(parent_style)->useDefaultsFrom(cs->evaluator().currentStyle());

		auto metrics = layout::computeLineMetrics(m_objects, cur_style);

		auto line = layout::Line::fromInlineObjects(cs, cur_style, m_objects, metrics, available_space,
			/* is_first: */ true, /* is_last: */ true);

		m_generated_layout_object = line.get();
		return Ok(LayoutResult::make(std::move(line)));
	}
}
