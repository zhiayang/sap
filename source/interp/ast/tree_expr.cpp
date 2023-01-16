// tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/paragraph.h"
#include "tree/container.h"

#include "interp/ast.h"         // for NumberLit, InlineTreeExpr, StringLit
#include "interp/type.h"        // for Type
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	static StrErrorOr<void> typecheck_list_of_tios(Typechecker* ts, //
	    const std::vector<std::unique_ptr<tree::InlineObject>>& tios)
	{
		for(auto& obj : tios)
		{
			if(auto txt = dynamic_cast<const tree::Text*>(obj.get()); txt)
				; // do nothing

			else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj.get()); sc)
				TRY(sc->call->typecheck(ts));

			else
				sap::internal_error("unsupported thing A: {}", typeid(obj.get()).name());
		}

		return Ok();
	}

	static StrErrorOr<std::vector<std::unique_ptr<tree::InlineObject>>> evaluate_list_of_tios(Evaluator* ev,
	    const std::vector<std::unique_ptr<tree::InlineObject>>& tios)
	{
		std::vector<std::unique_ptr<tree::InlineObject>> ret {};
		for(auto& obj : tios)
		{
			if(auto txt = dynamic_cast<const tree::Text*>(obj.get()); txt)
			{
				ret.emplace_back(new tree::Text(txt->contents(), txt->style()));
			}
			else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj.get()); sc)
			{
				auto tmp = TRY(ev->convertValueToText(TRY_VALUE(sc->call->evaluate(ev))));
				ret.insert(tios.end(), std::move_iterator(tmp.begin()), std::move_iterator(tmp.end()));
			}
			else
			{
				sap::internal_error("unsupported thing B: {}", typeid(obj.get()).name());
			}
		}

		return Ok(std::move(ret));
	}




	StrErrorOr<TCResult> TreeInlineExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		TRY(typecheck_list_of_tios(ts, this->objects));
		return TCResult::ofRValue(Type::makeTreeInlineObj());
	}

	StrErrorOr<EvalResult> TreeInlineExpr::evaluate(Evaluator* ev) const
	{
		auto tios = TRY(evaluate_list_of_tios(ev, this->objects));
		return EvalResult::ofValue(Value::treeInlineObject(std::move(tios)));
	}




	static StrErrorOr<void> typecheck_block_obj(Typechecker* ts, tree::BlockObject* obj)
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
		else if(auto bc = dynamic_cast<const tree::BlockContainer*>(obj); bc)
		{
			for(auto& inner : bc->contents())
				TRY(typecheck_block_obj(ts, inner.get()));
		}
		else
		{
			sap::internal_error("unsupported block object: {}", typeid(obj).name());
		}

		return Ok();
	}

	static StrErrorOr<EvalResult> evaluate_block_obj(Evaluator* ev, tree::BlockObject* obj)
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
		else if(auto bc = dynamic_cast<const tree::BlockContainer*>(obj); bc)
		{
			auto container = std::make_unique<tree::BlockContainer>();
			for(auto& inner : bc->contents())
				container->contents().push_back(TRY_VALUE(evaluate_block_obj(ev, inner.get())).takeTreeBlockObj());

			return EvalResult::ofValue(Value::treeBlockObject(std::move(container)));
		}
		else
		{
			sap::internal_error("unsupported block object: {}", typeid(obj).name());
		}
	}






	StrErrorOr<TCResult> TreeBlockExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		TRY(typecheck_block_obj(ts, this->object.get()));
		return TCResult::ofRValue(Type::makeTreeBlockObj());
	}

	StrErrorOr<EvalResult> TreeBlockExpr::evaluate(Evaluator* ev) const
	{
		return evaluate_block_obj(ev, this->object.get());
	}
}
