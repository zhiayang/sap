// base.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "tree/wrappers.h"

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
			if(m_override_width.has_value())
				obj->overrideLayoutSizeX(*m_override_width);

			// TODO: maybe allow being able to specify ascent and descent separately
			if(m_override_height.has_value())
				obj->overrideLayoutSizeY(*m_override_height);

			if(m_abs_position_override.has_value())
				obj->overrideAbsolutePosition(*m_abs_position_override);

			if(m_rel_position_offset.has_value())
				obj->addRelativePositionOffset(*m_rel_position_offset);

			if(not std::holds_alternative<std::monostate>(m_link_destination))
				obj->setLinkDestination(m_link_destination);
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

	void BlockObject::overrideLayoutWidth(Length x)
	{
		m_override_width = std::move(x);
	}

	void BlockObject::overrideLayoutHeight(Length y)
	{
		m_override_height = std::move(y);
	}

	void BlockObject::setLinkDestination(LinkDestination dest)
	{
		m_link_destination = std::move(dest);
	}

	LinkDestination BlockObject::linkDestination() const
	{
		return m_link_destination;
	}

	void InlineObject::setLinkDestination(LinkDestination dest)
	{
		m_link_destination = std::move(dest);
	}

	LinkDestination InlineObject::linkDestination() const
	{
		return m_link_destination;
	}

	void InlineObject::copyAttributesFrom(const InlineObject& obj)
	{
		m_style = obj.m_style;
		m_raise_height = obj.m_raise_height;
		m_link_destination = obj.m_link_destination;
		m_parent_span = obj.m_parent_span;

		if(dynamic_cast<InlineSpan*>(this) && dynamic_cast<const InlineSpan*>(&obj))
			static_cast<InlineSpan*>(this)->m_override_width = static_cast<const InlineSpan&>(obj).m_override_width;
	}

	InlineSpan::InlineSpan() : m_objects()
	{
	}

	InlineSpan::InlineSpan(std::vector<zst::SharedPtr<InlineObject>> objs) : m_objects(std::move(objs))
	{
		for(auto& obj : m_objects)
			obj->setParentSpan(this);
	}

	void InlineSpan::addObject(zst::SharedPtr<InlineObject> obj)
	{
		obj->setParentSpan(this);
		m_objects.push_back(std::move(obj));
	}

	void InlineSpan::addObjects(std::vector<zst::SharedPtr<InlineObject>> objs)
	{
		for(auto& x : objs)
			x->setParentSpan(this);

		std::move(objs.begin(), objs.end(), std::back_inserter(m_objects));
	}

	void InlineSpan::addGeneratedLayoutSpan(layout::PseudoSpan* span) const
	{
		m_generated_layout_spans.push_back(span);
	}



	LayoutResult::~LayoutResult() = default;

	LayoutResult LayoutResult::empty()
	{
		return LayoutResult(std::nullopt);
	}

	LayoutResult LayoutResult::make(std::unique_ptr<layout::LayoutObject> obj)
	{
		return LayoutResult(std::move(obj));
	}

	LayoutResult::LayoutResult(decltype(object) x) : object(std::move(x))
	{
	}
}
