// base.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/raw.h"
#include "tree/base.h"
#include "tree/image.h"
#include "tree/spacer.h"
#include "tree/wrappers.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "layout/base.h"

namespace sap::tree
{
	ErrorOr<LayoutResult> BlockObject::createLayoutObject( //
	    interp::Interpreter* cs,                           //
	    const Style& parent_style,
	    Size2d available_space) const
	{
		if(m_override_width.has_value())
			available_space.x() = *m_override_width;

		if(m_override_height.has_value())
			available_space.y() = *m_override_height;

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

			if(not m_generated_layout_object.has_value())
				m_generated_layout_object = result.object->get();
		}

		return OkMove(result);
	}

	void* InlineObject::operator new(size_t count)
	{
		thread_local static auto pool = util::Pool<uint8_t, /* region size: */ (1 << 16)>();
		return pool.allocate_raw(count);
	}

	void InlineObject::operator delete(void* ptr, size_t count)
	{
	}

	void* BlockObject::operator new(size_t count)
	{
		thread_local static auto pool = util::Pool<uint8_t, /* region size: */ (1 << 16)>();
		return pool.allocate_raw(count);
	}

	void BlockObject::operator delete(void* ptr, size_t count)
	{
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

		if(m_kind == Kind::Span && obj.m_kind == Kind::Span)
		{
			auto self = static_cast<InlineSpan*>(this);
			auto& other = static_cast<const InlineSpan&>(obj);
			self->m_override_width = other.m_override_width;
			self->m_is_glued = other.m_is_glued;
		}
	}


	const Text* InlineObject::castToText() const
	{
		return m_kind == Kind::Text ? static_cast<const Text*>(this) : nullptr;
	}
	const Separator* InlineObject::castToSeparator() const
	{
		return m_kind == Kind::Separator ? static_cast<const Separator*>(this) : nullptr;
	}
	const InlineSpan* InlineObject::castToSpan() const
	{
		return m_kind == Kind::Span ? static_cast<const InlineSpan*>(this) : nullptr;
	}
	const ScriptCall* InlineObject::castToScriptCall() const
	{
		return m_kind == Kind::ScriptCall ? static_cast<const ScriptCall*>(this) : nullptr;
	}
	Text* InlineObject::castToText()
	{
		return m_kind == Kind::Text ? static_cast<Text*>(this) : nullptr;
	}
	Separator* InlineObject::castToSeparator()
	{
		return m_kind == Kind::Separator ? static_cast<Separator*>(this) : nullptr;
	}
	InlineSpan* InlineObject::castToSpan()
	{
		return m_kind == Kind::Span ? static_cast<InlineSpan*>(this) : nullptr;
	}
	ScriptCall* InlineObject::castToScriptCall()
	{
		return m_kind == Kind::ScriptCall ? static_cast<ScriptCall*>(this) : nullptr;
	}





	Image* BlockObject::castToImage()
	{
		return m_kind == Kind::Image ? static_cast<Image*>(this) : nullptr;
	}
	Spacer* BlockObject::castToSpacer()
	{
		return m_kind == Kind::Spacer ? static_cast<Spacer*>(this) : nullptr;
	}
	RawBlock* BlockObject::castToRawBlock()
	{
		return m_kind == Kind::RawBlock ? static_cast<RawBlock*>(this) : nullptr;
	}
	Container* BlockObject::castToContainer()
	{
		return m_kind == Kind::Container ? static_cast<Container*>(this) : nullptr;
	}
	Paragraph* BlockObject::castToParagraph()
	{
		return m_kind == Kind::Paragraph ? static_cast<Paragraph*>(this) : nullptr;
	}
	ScriptBlock* BlockObject::castToScriptBlock()
	{
		return m_kind == Kind::ScriptBlock ? static_cast<ScriptBlock*>(this) : nullptr;
	}
	WrappedLine* BlockObject::castToWrappedLine()
	{
		return m_kind == Kind::WrappedLine ? static_cast<WrappedLine*>(this) : nullptr;
	}
	const Image* BlockObject::castToImage() const
	{
		return m_kind == Kind::Image ? static_cast<const Image*>(this) : nullptr;
	}
	const Spacer* BlockObject::castToSpacer() const
	{
		return m_kind == Kind::Spacer ? static_cast<const Spacer*>(this) : nullptr;
	}
	const RawBlock* BlockObject::castToRawBlock() const
	{
		return m_kind == Kind::RawBlock ? static_cast<const RawBlock*>(this) : nullptr;
	}
	const Container* BlockObject::castToContainer() const
	{
		return m_kind == Kind::Container ? static_cast<const Container*>(this) : nullptr;
	}
	const Paragraph* BlockObject::castToParagraph() const
	{
		return m_kind == Kind::Paragraph ? static_cast<const Paragraph*>(this) : nullptr;
	}
	const ScriptBlock* BlockObject::castToScriptBlock() const
	{
		return m_kind == Kind::ScriptBlock ? static_cast<const ScriptBlock*>(this) : nullptr;
	}
	const WrappedLine* BlockObject::castToWrappedLine() const
	{
		return m_kind == Kind::WrappedLine ? static_cast<const WrappedLine*>(this) : nullptr;
	}












	InlineSpan::InlineSpan(bool glue) : InlineObject(Kind::Span), m_objects(), m_is_glued(glue)
	{
	}

	InlineSpan::InlineSpan(bool glue, zst::SharedPtr<InlineObject> obj)
	    : InlineObject(Kind::Span), m_objects(), m_is_glued(glue)
	{
		obj->setParentSpan(this);
		m_objects.push_back(std::move(obj));
	}

	InlineSpan::InlineSpan(bool glue, std::vector<zst::SharedPtr<InlineObject>> objs)
	    : InlineObject(Kind::Span), m_objects(std::move(objs)), m_is_glued(glue)
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

	void InlineSpan::addGeneratedLayoutSpan(layout::LayoutSpan* span) const
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
