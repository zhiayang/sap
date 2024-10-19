// call.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h"

#include "interp/cst.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/eval_result.h"
#include "interp/overload_resolution.h"

namespace sap::interp::cst
{
	ErrorOr<EvalResult> FunctionCall::evaluate_impl(Evaluator* ev) const
	{
		auto& param_types = this->callee->type->toFunction()->parameterTypes();
		if(param_types.size() != this->arguments.size())
			return ErrMsg(m_location, "function arity mismatch");

		std::vector<Value> processed_args {};
		for(size_t i = 0; i < this->arguments.size(); i++)
		{
			auto& arg = this->arguments[i];
			auto val = TRY(arg->evaluate(ev));

			if(i == 0 && this->ufcs_kind != UFCSKind::None)
			{
				if(this->ufcs_kind == UFCSKind::ByValue)
				{
					processed_args.push_back(val.take());
				}
				else
				{
					Value* ptr = nullptr;
					if(val.isLValue())
						ptr = &val.get();
					else
						ptr = ev->frame().createTemporary(std::move(val).take());

					Value self {};
					if(this->ufcs_kind == UFCSKind::MutablePointer)
						self = Value::mutablePointer(ptr->type(), ptr);
					else
						self = Value::pointer(ptr->type(), ptr);

					processed_args.push_back(std::move(self));
				}
			}
			else
			{
				if(val.isLValue() && not val.get().type()->isCloneable())
				{
					return ErrMsg(arg->loc(),
					    "cannot pass a non-cloneable value of type '{}' as an argument; move with `*`",
					    val.get().type());
				}

				if(param_types[i]->isVariadicArray())
				{
					auto variadic_elm = param_types[i]->arrayElement();
					assert(val.get().type()->isVariadicArray());

					auto arr = val.take().takeArray();
					for(auto& elm : arr)
						elm = ev->castValue(std::move(elm), variadic_elm);

					processed_args.push_back(Value::array(variadic_elm, std::move(arr), /* variadic: */ true));
				}
				else
				{
					processed_args.push_back(ev->castValue(val.take(), param_types[i]));
				}
			}
		}

		return ev->call(this->callee->definition(), processed_args);
	}
}
