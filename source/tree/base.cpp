// base.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "layout/base.h"

namespace sap::tree
{
	ErrorOr<LayoutResult> BlockObject::createLayoutObject( //
		interp::Interpreter* cs,                           //
		const Style* parent_style,
		Size2d available_space) const
	{
		auto result = TRY(this->create_layout_object_impl(cs, parent_style, available_space));
		if(result.object.has_value())
		{
			auto& obj = *result.object;
			if(m_override_size_x.has_value())
				obj->overrideLayoutSizeX(*m_override_size_x);

			// TODO: maybe allow being able to specify ascent and descent separately
			if(m_override_size_y.has_value())
				obj->overrideLayoutSizeY(*m_override_size_y);

			if(m_abs_position_override.has_value())
				obj->overrideAbsolutePosition(*m_abs_position_override);

			if(m_rel_position_offset.has_value())
				obj->addRelativePositionOffset(*m_rel_position_offset);
		}

		return Ok(std::move(result));
	}


	void BlockObject::offsetRelativePosition(Size2d offset)
	{
		m_rel_position_offset = std::move(offset);
	}

	void BlockObject::overrideAbsolutePosition(layout::AbsolutePagePos pos)
	{
		m_abs_position_override = std::move(pos);
	}

	void BlockObject::overrideLayoutSizeX(Length x)
	{
		m_override_size_x = std::move(x);
	}

	void BlockObject::overrideLayoutSizeY(Length y)
	{
		m_override_size_y = std::move(y);
	}
}
