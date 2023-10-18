// call.cpp
// Copyright (c) 2022, zhiayang
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
	static ErrorOr<EvalResult> eval_arg(Evaluator* ev, const Either<std::unique_ptr<Expr>, const Expr*>& arg)
	{
		if(arg.is_left())
			return arg.left()->evaluate(ev);
		else
			return arg.right()->evaluate(ev);
	}

	static Location get_arg_loc(const Either<std::unique_ptr<Expr>, const Expr*>& arg)
	{
		if(arg.is_left())
			return arg.left()->loc();
		else
			return arg.right()->loc();
	}

	ErrorOr<EvalResult> FunctionCall::evaluate_impl(Evaluator* ev) const
	{
		auto& param_types = this->callee->type->toFunction()->parameterTypes();
		if(param_types.size() != this->arguments.size())
			return ErrMsg(m_location, "function arity mismatch");

		std::vector<Value> processed_args {};
		for(size_t i = 0; i < this->arguments.size(); i++)
		{
			auto& arg = this->arguments[i];
			auto val = TRY(eval_arg(ev, arg));

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
					return ErrMsg(get_arg_loc(arg),
					    "cannot pass a non-cloneable value of type '{}' as an argument; move with `*`",
					    val.get().type());
				}

				if(param_types[i]->isVariadicArray())
				{
					auto variadic_elm = param_types[i]->arrayElement();
					std::vector<Value> vararg_array {};

					// FIXME: currently we assume that if the argument is not a spread op,
					// then this arg (and the following args) will be part of the variadic array.
					if(val.get().type()->isVariadicArray())
					{
						// if the value itself is variadic, it means it's a spread op
						auto arr = val.take().takeArray();
						for(auto& v : arr)
							vararg_array.push_back(ev->castValue(std::move(v), variadic_elm));
					}
					else
					{
						for(size_t k = i; k < this->arguments.size(); k++)
						{
							auto vv = TRY(eval_arg(ev, this->arguments[k]));
							if(vv.isLValue() && not vv.get().type()->isCloneable())
							{
								return ErrMsg(get_arg_loc(this->arguments[k]),
								    "cannot pass a non-cloneable value of type '{}' as an argument; move with `*`",
								    vv.get().type());
							}

							vararg_array.push_back(ev->castValue(vv.take(), variadic_elm));
						}
					}

					processed_args.push_back(Value::array(variadic_elm, std::move(vararg_array)));
				}
				else
				{
					processed_args.push_back(ev->castValue(val.take(), param_types[i]));
				}
			}
		}

		auto _ = ev->pushCallFrame();

		// check what kind of defn it is
		auto callee_defn = this->callee->definition();
		if(auto builtin_defn = dynamic_cast<const BuiltinFunctionDefn*>(callee_defn); builtin_defn != nullptr)
		{
			return builtin_defn->function(ev, processed_args);
		}
		else if(auto func_defn = dynamic_cast<const FunctionDefn*>(callee_defn); func_defn != nullptr)
		{
			return func_defn->call(ev, processed_args);
		}
		else
		{
			return ErrMsg(ev, "not implemented");
		}
	}
}
