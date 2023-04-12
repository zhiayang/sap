// tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "tree/raw.h"
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
	    const std::vector<zst::SharedPtr<tree::InlineObject>>& tios)
	{
		for(auto& obj : tios)
		{
			if(obj->isText() || obj->isSeparator())
			{
				; // do nothing
			}
			else if(auto sp = obj->castToSpan())
			{
				TRY(typecheck_list_of_tios(ts, sp->objects()));
			}
			else if(auto sc = obj->castToScriptCall())
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

	static ErrorOr<std::vector<zst::SharedPtr<tree::InlineObject>>> evaluate_list_of_tios(Evaluator* ev,
	    const std::vector<zst::SharedPtr<tree::InlineObject>>& tios)
	{
		std::vector<zst::SharedPtr<tree::InlineObject>> ret {};
		for(auto& obj : tios)
		{
			if(auto txt = obj->castToText())
			{
				auto new_text = zst::make_shared<tree::Text>(txt->contents());
				new_text->copyAttributesFrom(*txt);

				ret.push_back(std::move(new_text));
			}
			else if(auto sep = obj->castToSeparator())
			{
				auto new_sep = zst::make_shared<tree::Separator>(sep->kind(), sep->hyphenationCost());
				new_sep->copyAttributesFrom(*sep);

				ret.push_back(std::move(new_sep));
			}
			else if(auto span = obj->castToSpan())
			{
				auto tmp = TRY(evaluate_list_of_tios(ev, span->objects()));

				auto new_span = zst::make_shared<tree::InlineSpan>(/* glue: */ false, std::move(tmp));
				new_span->copyAttributesFrom(*span);

				ret.push_back(std::move(new_span));
			}
			else if(auto sc = obj->castToScriptCall())
			{
				auto tmp = TRY(ev->convertValueToText(TRY_VALUE(sc->call->evaluate(ev))));
				ret.push_back(std::move(tmp));
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
		tios = TRY(tree::Paragraph::processWordSeparators(std::move(tios)));

		auto span = zst::make_shared<tree::InlineSpan>(/* glue: */ false, std::move(tios));

		return EvalResult::ofValue(Value::treeInlineObject(std::move(span)));
	}




	static ErrorOr<void> typecheck_block_obj(Typechecker* ts, tree::BlockObject* obj)
	{
		if(auto para = obj->castToParagraph())
		{
			TRY(typecheck_list_of_tios(ts, para->contents()));
		}
		else if(auto sb = obj->castToScriptBlock())
		{
			TRY(sb->body->typecheck(ts));
		}
		else if(auto box = obj->castToContainer())
		{
			for(auto& inner : box->contents())
				TRY(typecheck_block_obj(ts, inner.get()));
		}
		else if(auto line = obj->castToWrappedLine())
		{
			TRY(typecheck_list_of_tios(ts, line->objects()));
		}
		else if(obj->isRawBlock())
		{
			// nothing
		}
		else
		{
			auto& x = *obj;
			sap::internal_error("unsupported block object: {}", typeid(x).name());
		}

		return Ok();
	}

	static ErrorOr<EvalResult> evaluate_block_obj(Evaluator* ev, const zst::SharedPtr<tree::BlockObject>& obj)
	{
		auto make_para_from_tios = [](std::vector<zst::SharedPtr<tree::InlineObject>> tios) {
			auto new_para = zst::make_shared<tree::Paragraph>(std::move(tios));
			return Value::treeBlockObject(std::move(new_para));
		};

		if(auto para = obj->castToParagraph())
		{
			if(para->contents().size() == 1)
			{
				auto& obj = para->contents()[0];
				if(auto sc = obj->castToScriptCall())
				{
					auto tmp = TRY_VALUE(sc->call->evaluate(ev));
					auto ty = tmp.type();

					if(not(ty->isTreeBlockObj() || (ty->isOptional() && ty->optionalElement()->isTreeBlockObj())))
					{
						return EvalResult::ofValue(make_para_from_tios({
						    TRY(ev->convertValueToText(std::move(tmp))),
						}));
					}

					if(tmp.isOptional() && not tmp.haveOptionalValue())
						return EvalResult::ofVoid();

					zst::SharedPtr<tree::BlockObject> blk {};
					if(tmp.isOptional())
						blk = std::move(**tmp.getOptional()).takeTreeBlockObj();
					else
						blk = std::move(tmp).takeTreeBlockObj();

					return EvalResult::ofValue(Value::treeBlockObject(std::move(blk)));
				}
				else
				{
					goto do_para;
				}
			}
			else
			{
			do_para:
				auto tios = TRY(evaluate_list_of_tios(ev, para->contents()));
				return EvalResult::ofValue(make_para_from_tios(std::move(tios)));
			}
		}
		else if(auto line = obj->castToWrappedLine())
		{
			auto tios = TRY(evaluate_list_of_tios(ev, line->objects()));
			tios = TRY(tree::Paragraph::processWordSeparators(std::move(tios)));

			return EvalResult::ofValue(Value::treeBlockObject(zst::make_shared<tree::WrappedLine>(std::move(tios))));
		}
		else if(auto box = obj->castToContainer())
		{
			auto container = zst::make_shared<tree::Container>(box->direction());
			for(auto& inner : box->contents())
				container->contents().push_back(TRY_VALUE(evaluate_block_obj(ev, inner)).takeTreeBlockObj());

			return EvalResult::ofValue(Value::treeBlockObject(std::move(container)));
		}
		else if(auto raw = obj->castToRawBlock())
		{
			return EvalResult::ofValue(Value::treeBlockObject(obj));
		}
		else if(auto sc = obj->castToScriptBlock())
		{
			sap::internal_error("nani??");
		}
		else
		{
			auto& x = *obj;
			sap::internal_error("unsupported block object: {}", typeid(x).name());
		}
	}






	ErrorOr<TCResult> TreeBlockExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		TRY(typecheck_block_obj(ts, this->object.get()));
		return TCResult::ofRValue(Type::makeTreeBlockObj());
	}

	ErrorOr<EvalResult> TreeBlockExpr::evaluate_impl(Evaluator* ev) const
	{
		return evaluate_block_obj(ev, this->object);
	}
}
