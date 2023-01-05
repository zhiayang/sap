// tree.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"         // for NumberLit, InlineTreeExpr, StringLit
#include "interp/tree.h"        //
#include "interp/type.h"        // for Type
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	ErrorOr<TCResult> InlineTreeExpr::typecheck_impl(Typechecker* ts, const Type* infer) const
	{
		for(auto& obj : this->objects)
		{
			if(auto txt = dynamic_cast<const tree::Text*>(obj.get()); txt)
				; // do nothing
			else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj.get()); sc)
				TRY(sc->call->typecheck(ts));
			else
				sap::internal_error("unsupported thing: {}", typeid(obj.get()).name());
		}

		return TCResult::ofRValue(Type::makeTreeInlineObj());
	}

	ErrorOr<EvalResult> InlineTreeExpr::evaluate(Evaluator* ev) const
	{
		std::vector<std::unique_ptr<tree::InlineObject>> tios {};
		for(auto& obj : this->objects)
		{
			if(auto txt = dynamic_cast<const tree::Text*>(obj.get()); txt)
			{
				tios.emplace_back(new tree::Text(txt->contents(), txt->style()));
			}
			else if(auto sc = dynamic_cast<const tree::ScriptCall*>(obj.get()); sc)
			{
				auto tmp = TRY(ev->convertValueToText(TRY_VALUE(sc->call->evaluate(ev))));
				tios.insert(tios.end(), std::move_iterator(tmp.begin()), std::move_iterator(tmp.end()));
			}
			else
			{
				sap::internal_error("unsupported thing: {}", typeid(obj.get()).name());
			}
		}


		return EvalResult::ofValue(Value::treeInlineObject(std::move(tios)));
	}
}
