// tree.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tree/raw.h"
#include "tree/wrappers.h"
#include "tree/paragraph.h"
#include "tree/container.h"

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"

namespace sap::interp::ast
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
				// the only thing that needs typechecking in a TIO is a script call
				// unfortunately, I don't know how to store the result of the typecheck
				TRY(sc->typecheckCall(ts));
			}
			else
			{
				sap::internal_error("unsupported thing A: {}", typeid(obj.get()).name());
			}
		}

		return Ok();
	}


	ErrorOr<TCResult> TreeInlineExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		TRY(typecheck_list_of_tios(ts, this->objects));
		return TCResult::ofRValue<cst::TreeInlineExpr>(m_location, this->objects);
	}



	static ErrorOr<std::unique_ptr<cst::TreeBlockExpr>> typecheck_block_obj(Typechecker* ts,
	    Location location,
	    const zst::SharedPtr<tree::BlockObject>& obj)
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
				TRY(typecheck_block_obj(ts, location, inner));
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

		return Ok(std::make_unique<cst::TreeBlockExpr>(location, obj));
	}



	ErrorOr<TCResult> TreeBlockExpr::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		return TCResult::ofRValue(TRY(typecheck_block_obj(ts, m_location, this->object)));
	}
}
