// tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/wrappers.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "interp/ast.h"         // for NumberLit, InlineTreeExpr, StringLit
#include "interp/type.h"        // for Type
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	static ErrorOr<void> typecheck_list_of_tios(Typechecker* ts, //
		const std::vector<std::unique_ptr<tree::InlineObject>>& tios)
	{
		for(auto& obj : tios)
		{
			if(dynamic_cast<const tree::Text*>(obj.get()) || dynamic_cast<const tree::Separator*>(obj.get()))
			{
				; // do nothing
			}
			else if(auto sp = dynamic_cast<const tree::InlineSpan*>(obj.get()); sp)
			{
				TRY(typecheck_list_of_tios(ts, sp->objects()));
			}
			else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj.get()); sc)
			{
				TRY(sc->call->typecheck(ts));
			}
			else
			{
				sap::internal_error("unsupported thing A: {}", typeid(obj.get()).name());
			}
		}

		return Ok();
	}

	static ErrorOr<std::vector<std::unique_ptr<tree::InlineObject>>> evaluate_list_of_tios(Evaluator* ev,
		const std::vector<std::unique_ptr<tree::InlineObject>>& tios)
	{
		std::vector<std::unique_ptr<tree::InlineObject>> ret {};
		for(auto& obj : tios)
		{
			if(auto txt = dynamic_cast<const tree::Text*>(obj.get()); txt)
			{
				ret.emplace_back(new tree::Text(txt->contents(), txt->style()));
			}
			else if(auto sep = dynamic_cast<const tree::Separator*>(obj.get()); sep)
			{
				ret.emplace_back(new tree::Separator(sep->kind(), sep->hyphenationCost()));
			}
			else if(auto span = dynamic_cast<const tree::InlineSpan*>(obj.get()); span)
			{
				auto tmp = TRY(evaluate_list_of_tios(ev, span->objects()));
				std::move(tmp.begin(), tmp.end(), std::back_inserter(ret));
			}
			else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj.get()); sc)
			{
				auto tmp = TRY(ev->convertValueToText(TRY_VALUE(sc->call->evaluate(ev))));
				std::move(tmp.begin(), tmp.end(), std::back_inserter(ret));
			}
			else
			{
				sap::internal_error("unsupported thing B: {}", typeid(obj.get()).name());
			}
		}

		return Ok(std::move(ret));
	}




	ErrorOr<TCResult> TreeInlineExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		TRY(typecheck_list_of_tios(ts, this->objects));
		return TCResult::ofRValue(Type::makeTreeInlineObj());
	}

	ErrorOr<EvalResult> TreeInlineExpr::evaluate_impl(Evaluator* ev) const
	{
		auto tios = TRY(evaluate_list_of_tios(ev, this->objects));
		auto span = std::make_unique<tree::InlineSpan>(std::move(tios));

		return EvalResult::ofValue(Value::treeInlineObject(std::move(span)));
	}




	static ErrorOr<void> typecheck_block_obj(Typechecker* ts, tree::BlockObject* obj)
	{
		if(auto para = dynamic_cast<const tree::Paragraph*>(obj); para)
		{
			TRY(typecheck_list_of_tios(ts, para->contents()));
		}
		else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj); sc)
		{
			TRY(sc->call->typecheck(ts));
		}
		else if(auto sb = dynamic_cast<const tree::ScriptBlock*>(obj); sb)
		{
			TRY(sb->body->typecheck(ts));
		}
		else if(auto box = dynamic_cast<const tree::Container*>(obj); box)
		{
			for(auto& inner : box->contents())
				TRY(typecheck_block_obj(ts, inner.get()));
		}
		else
		{
			sap::internal_error("unsupported block object: {}", typeid(obj).name());
		}

		return Ok();
	}

	static ErrorOr<EvalResult> evaluate_block_obj(Evaluator* ev, tree::BlockObject* obj)
	{
		auto make_para_from_tios = [](std::vector<std::unique_ptr<tree::InlineObject>> tios) {
			auto new_para = std::make_unique<tree::Paragraph>(std::move(tios));
			return Value::treeBlockObject(std::move(new_para));
		};

		if(auto para = dynamic_cast<const tree::Paragraph*>(obj); para)
		{
			auto tios = TRY(evaluate_list_of_tios(ev, para->contents()));
			return EvalResult::ofValue(make_para_from_tios(std::move(tios)));
		}
		else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj); sc)
		{
			auto value = TRY_VALUE(sc->call->evaluate(ev));

			if(value.isTreeBlockObj())
				return EvalResult::ofValue(std::move(value));
			else
				return EvalResult::ofValue(make_para_from_tios(TRY(ev->convertValueToText(std::move(value)))));
		}
		else if(auto box = dynamic_cast<const tree::Container*>(obj); box)
		{
			auto container = std::make_unique<tree::Container>(box->direction());
			for(auto& inner : box->contents())
				container->contents().push_back(TRY_VALUE(evaluate_block_obj(ev, inner.get())).takeTreeBlockObj());

			return EvalResult::ofValue(Value::treeBlockObject(std::move(container)));
		}
		else
		{
			sap::internal_error("unsupported block object: {}", typeid(obj).name());
		}
	}






	ErrorOr<TCResult> TreeBlockExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		TRY(typecheck_block_obj(ts, this->object.get()));
		return TCResult::ofRValue(Type::makeTreeBlockObj());
	}

	ErrorOr<EvalResult> TreeBlockExpr::evaluate_impl(Evaluator* ev) const
	{
		return evaluate_block_obj(ev, this->object.get());
	}
}
