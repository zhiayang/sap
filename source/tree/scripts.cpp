// scripts.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/base.h"
#include "tree/paragraph.h"

#include "layout/base.h"
#include "interp/interp.h"
#include "interp/evaluator.h"

namespace sap::tree
{
	ScriptCall::ScriptCall(ProcessingPhase phase) : ScriptObject(phase)
	{
	}

	ErrorOr<std::optional<std::unique_ptr<layout::LayoutObject>>> ScriptCall::evaluate_scripts( //
		interp::Interpreter* cs,
		const Style* style,
		Size2d available_space) const
	{
		TRY(this->call->typecheck(&cs->typechecker()));
		if(m_run_phase != cs->currentPhase())
			return Ok(std::nullopt);

		auto maybe_value = TRY(this->call->evaluate(&cs->evaluator()));
		if(not maybe_value.hasValue())
			return Ok(std::nullopt);

		auto value = maybe_value.take();
		auto value_type = value.type();

		using Type = interp::Type;
		const Type* t_tbo = Type::makeTreeBlockObj();
		const Type* t_otbo = Type::makeOptional(t_tbo);

		const Type* t_lo = Type::makeLayoutObject();
		const Type* t_olo = Type::makeOptional(t_lo);

		if(not util::is_one_of(value_type, t_tbo, t_otbo))
		{
			return ErrMsg(this->call->loc(),
				"invalid result from script call in paragraph; got type '{}', expected either '{}' or '{}', "
				"or optional types of them",
				value_type, t_tbo, t_lo);
		}

		std::unique_ptr<layout::LayoutObject> layout_obj {};

		// check the optional guys
		if(util::is_one_of(value_type, t_otbo, t_olo))
		{
			if(not value.haveOptionalValue())
				return Ok(std::nullopt);

			value = std::move(value).takeOptional().value();
			value_type = value.type();
		}

		assert(value.type()->isTreeBlockObj() || value.type()->isLayoutObject());

		if(value.isTreeBlockObj())
		{
			auto tbo = m_created_tbos.emplace_back(std::move(value).takeTreeBlockObj()).get();

			// TODO: calculate available space correctly
			auto result = TRY(tbo->createLayoutObject(cs, style, available_space));

			if(not result.object.has_value())
				return Ok(std::nullopt);

			layout_obj = std::move(*result.object);
		}
		else
		{
			layout_obj = std::move(value).takeLayoutObject();
		}

		if(layout_obj != nullptr)
			return Ok(std::move(layout_obj));

		return Ok(std::nullopt);
	}

	ErrorOr<void> ScriptCall::evaluateScripts(interp::Interpreter* cs) const
	{
		auto space = Size2d(Length(INFINITY), Length(INFINITY));
		return this->evaluate_scripts(cs, cs->evaluator().currentStyle(), space).remove_value();
	}

	auto ScriptCall::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		auto obj = TRY(this->evaluate_scripts(cs, parent_style, available_space));

		if(obj.has_value())
			return Ok(LayoutResult::make(std::move(*obj)));

		return Ok(LayoutResult::empty());
	}



	ScriptBlock::ScriptBlock(ProcessingPhase phase) : ScriptObject(phase)
	{
	}

	ErrorOr<void> ScriptBlock::evaluateScripts(interp::Interpreter* cs) const
	{
		TRY(this->body->typecheck(&cs->typechecker()));

		if(m_run_phase != cs->currentPhase())
			return Ok();

		return this->body->evaluate(&cs->evaluator()).remove_value();
	}

	auto ScriptBlock::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		TRY(this->evaluateScripts(cs));
		return Ok(LayoutResult::empty());
	}



	InlineObject::~InlineObject()
	{
	}
}
