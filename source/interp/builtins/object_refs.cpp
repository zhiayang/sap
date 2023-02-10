// object_refs.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"

namespace sap::interp::builtin
{
	ErrorOr<EvalResult> ref_object(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		assert(args[0].type()->isPointer());
		auto ty = args[0].type()->pointerElement();

		if(args[0].getPointer() == nullptr)
			return ErrMsg(ev, "cannot call `ref()` with a null pointer");

		auto& value = *args[0].getPointer();

		if(ty->isTreeInlineObj())
		{
			std::vector<tree::InlineObject*> obj_ptrs {};
			for(auto& obj : value.getTreeInlineObj())
			{
				if(obj == nullptr)
					sap::internal_error("unexpected nullptr");

				obj_ptrs.push_back(obj.get());
			}

			return EvalResult::ofValue(Value::treeInlineObjectRef(std::move(obj_ptrs)));
		}
		else if(ty->isTreeBlockObj())
		{
			return EvalResult::ofValue( //
				Value::treeBlockObjectRef(const_cast<tree::BlockObject*>(&value.getTreeBlockObj())));
		}
		else if(ty->isLayoutObject())
		{
			return EvalResult::ofValue( //
				Value::layoutObjectRef(const_cast<layout::LayoutObject*>(&value.getLayoutObject())));
		}
		else
		{
			sap::internal_error("??? '{}'", ty);
		}
	}
}
