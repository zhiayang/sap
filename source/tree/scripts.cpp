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

	ErrorOr<std::optional<std::unique_ptr<layout::LayoutObject>>> ScriptCall::evaluate_scripts(interp::Interpreter* cs) const
	{
		TRY(this->call->typecheck(&cs->typechecker()));
		if(m_run_phase != cs->currentPhase())
			return Ok(std::nullopt);

		auto value = TRY(this->call->evaluate(&cs->evaluator()));
		if(not value.hasValue())
			return Ok(std::nullopt);

		auto val = value.take();
		if(val.isTreeBlockObj())
		{
			auto tbo = std::move(val).takeTreeBlockObj();

			// TODO: calculate available space correctly
			auto layout_obj = TRY(tbo->createLayoutObject(cs, cs->evaluator().currentStyle(), //
				Size2d(Length(INFINITY), Length(INFINITY))));

			if(not layout_obj.object.has_value())
				return Ok(std::nullopt);

			return Ok(std::move(*layout_obj.object));
		}
		else
		{
			return ErrMsg(cs->evaluator().loc(), "inline object not allowed in this context");
		}
	}

	ErrorOr<void> ScriptCall::evaluateScripts(interp::Interpreter* cs) const
	{
		return this->evaluate_scripts(cs).remove_value();
	}

	auto ScriptCall::createLayoutObject(interp::Interpreter* cs, const Style* parent_style, Size2d available_space) const
		-> ErrorOr<LayoutResult>
	{
		TRY(this->evaluate_scripts(cs));
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
