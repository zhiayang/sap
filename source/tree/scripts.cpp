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
	ScriptCall::ScriptCall(ProcessingPhase phase) : InlineObject(Kind::ScriptCall), ScriptObject(phase)
	{
	}

	ErrorOr<void> ScriptCall::typecheckCall(interp::Typechecker* ts) const
	{
		m_typechecked_call = TRY(this->call->typecheck(ts)).take<interp::cst::FunctionCall>();
		return Ok();
	}

	auto ScriptCall::evaluate_script(interp::Interpreter* cs, const Style& style, Size2d available_space) const
	    -> ErrorOr<std::optional<ScriptEvalResult>>
	{
		if(m_run_phase != cs->currentPhase())
			return Ok(std::nullopt);

		auto maybe_value = TRY([&]() -> ErrorOr<interp::EvalResult> {
			if(m_typechecked_call != nullptr)
				return m_typechecked_call->evaluate(&cs->evaluator());


			auto call_expr = TRY(this->call->typecheck(&cs->typechecker())).take_expr();
			return call_expr->evaluate(&cs->evaluator());
		}());

		if(not maybe_value.hasValue())
			return Ok(std::nullopt);

		// it's a call, idk how you managed to return an lvalue from that
		assert(not maybe_value.isLValue());

		auto value = maybe_value.take();
		auto value_type = value.type();

		using Type = interp::Type;
		const Type* t_tio = Type::makeTreeInlineObj();
		const Type* t_otio = Type::makeOptional(t_tio);

		if(util::is_one_of(value_type, t_tio, t_otio))
		{
			if(value_type == t_otio)
			{
				if(not value.haveOptionalValue())
					return Ok(Left<ScriptEvalResult::left_type>());

				value = std::move(value).takeOptional().value();
				value_type = value.type();
			}

			assert(value.type()->isTreeInlineObj());

			auto tmp = TRY(cs->evaluator().convertValueToText(std::move(value)));
			return Ok(Left(std::move(tmp)));
		}


		const Type* t_tbo = Type::makeTreeBlockObj();
		const Type* t_otbo = Type::makeOptional(t_tbo);

		const Type* t_lo = Type::makeLayoutObject();
		const Type* t_olo = Type::makeOptional(t_lo);

		if(not util::is_one_of(value_type, t_tbo, t_otbo, t_lo, t_olo))
		{
			return ErrMsg(this->call->loc(),
			    "invalid result from document script call; got type '{}', expected either '{}' or '{}', "
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
			return Ok(Right(std::move(layout_obj)));

		return Ok(std::nullopt);
	}

	ErrorOr<void> ScriptCall::evaluateScripts(interp::Interpreter* cs) const
	{
		auto space = Size2d(Length(INFINITY), Length(INFINITY));
		return this->evaluate_script(cs, cs->evaluator().currentStyle(), space).remove_value();
	}

	auto ScriptCall::create_layout_object_impl(interp::Interpreter* cs,
	    const Style& parent_style,
	    Size2d available_space) const -> ErrorOr<LayoutResult>
	{
		auto obj = TRY(this->evaluate_script(cs, parent_style, available_space));
		if(obj.has_value())
		{
			assert(obj->is_right());
			return Ok(LayoutResult::make(obj->take_right()));
		}

		return Ok(LayoutResult::empty());
	}



	ScriptBlock::ScriptBlock(ProcessingPhase phase) : BlockObject(Kind::ScriptBlock), ScriptObject(phase)
	{
	}

	ErrorOr<void> ScriptBlock::evaluateScripts(interp::Interpreter* cs) const
	{
		if(m_run_phase != cs->currentPhase())
			return Ok();

		if(m_typechecked_stmt == nullptr)
			m_typechecked_stmt = TRY(this->body->typecheck(&cs->typechecker())).take_stmt();

		return m_typechecked_stmt->evaluate(&cs->evaluator()).remove_value();
	}

	auto ScriptBlock::create_layout_object_impl(interp::Interpreter* cs,
	    const Style& parent_style,
	    Size2d available_space) const -> ErrorOr<LayoutResult>
	{
		TRY(this->evaluateScripts(cs));
		return Ok(LayoutResult::empty());
	}



	InlineObject::~InlineObject()
	{
	}
}
