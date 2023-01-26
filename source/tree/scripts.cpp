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

#if 0
		// steal the parent output context (since we don't actually contain stuff)
		auto _ = cs->evaluator().pushBlockContext(cursor, this, cs->evaluator().getBlockContext().output_context);

		auto value_or_err = cs->run(this->call.get());

		if(value_or_err.is_err())
			value_or_err.error().showAndExit();

		auto value_or_empty = value_or_err.take_value();
		if(not value_or_empty.hasValue())
			return LayoutResult::make(cursor);

		auto style = cs->evaluator().currentStyle()->extendWith(parent_style);
		if(value_or_empty.get().isTreeBlockObj())
		{
			return m_created_block_objects.emplace_back(std::move(value_or_empty.get()).takeTreeBlockObj())
			    ->createLayoutObject(cs, std::move(cursor), style);
		}
		else
		{
			auto tmp = cs->evaluator().convertValueToText(std::move(value_or_empty).take());
			if(tmp.is_err())
				error("interp", "convertion to text failed: {}", tmp.error());

			auto objs = tmp.take_value();
			auto new_para = std::make_unique<Paragraph>();
			new_para->addObjects(std::move(objs));

			return m_created_block_objects.emplace_back(std::move(new_para))->createLayoutObject(cs, std::move(cursor), style);
		}
#endif
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
