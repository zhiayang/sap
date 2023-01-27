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

	ErrorOr<std::optional<layout::PageCursor>> ScriptCall::evaluate_scripts(interp::Interpreter* cs) const
	{
		TRY(this->call->typecheck(&cs->typechecker()));
		if(m_run_phase != cs->currentPhase())
			return Ok(std::nullopt);

		// steal the parent output context (since we don't actually contain stuff)
		auto prev = cs->evaluator().getBlockContext();
		auto _ = cs->evaluator().pushBlockContext(*prev.cursor_ref, this, prev.output_context);

		auto value = TRY(this->call->evaluate(&cs->evaluator()));
		if(not value.hasValue())
			return Ok(std::nullopt);

		auto& output_context = cs->evaluator().getBlockContext().output_context;
		auto val = value.take();
		if(val.isTreeBlockObj())
		{
			auto& fn = output_context.add_block_object;
			if(not fn.has_value())
				return ErrMsg(cs->evaluator().loc(), "block object not allowed in this context");

			auto [cursor, obj] = TRY((*fn)(std::move(val).takeTreeBlockObj(), std::nullopt));
			return Ok(std::move(cursor));
		}
		else
		{
			auto& fn = output_context.add_inline_object;
			if(not fn.has_value())
				return ErrMsg(cs->evaluator().loc(), "inline object not allowed in this context");

			std::vector<std::unique_ptr<InlineObject>> tios {};
			if(val.isTreeInlineObj())
				tios = std::move(val).takeTreeInlineObj();
			else
				tios = TRY(cs->evaluator().convertValueToText(std::move(val)));

			for(auto& tio : tios)
				TRY((*fn)(std::move(tio)));

			return Ok(std::nullopt);
		}
	}

	ErrorOr<void> ScriptCall::evaluateScripts(interp::Interpreter* cs) const
	{
		return this->evaluate_scripts(cs).remove_value();
	}

	auto ScriptCall::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> ErrorOr<LayoutResult>
	{
		auto new_cursor = TRY(this->evaluate_scripts(cs));
		return Ok(LayoutResult::make(new_cursor.value_or(cursor)));
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

	auto ScriptBlock::createLayoutObject(interp::Interpreter* cs, layout::PageCursor cursor, const Style* parent_style) const
	    -> ErrorOr<LayoutResult>
	{
		TRY(this->evaluateScripts(cs));
		return Ok(LayoutResult::make(cursor));
	}



	InlineObject::~InlineObject()
	{
	}
}
