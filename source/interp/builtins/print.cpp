// print.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for stringFromU32String

#include "tree/base.h"

#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult
#include "interp/builtin_fns.h"

namespace sap::interp::builtin
{
	static void print_values(std::vector<Value>& values)
	{
		for(size_t i = 0; i < values.size(); i++)
			zpr::print("{}{}", i == 0 ? "" : " ", unicode::stringFromU32String(values[i].toString()));
	}

	ErrorOr<EvalResult> print(Evaluator* ev, std::vector<Value>& args)
	{
		print_values(args);
		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> println(Evaluator* ev, std::vector<Value>& args)
	{
		print_values(args);
		zpr::println("");

		return EvalResult::ofVoid();
	}

	ErrorOr<EvalResult> to_string(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		auto& pv = args[0];
		auto pt = pv.type();

		if(not pt->isPointer())
			return ErrMsg(ev, "expected pointer type, got '{}'", pt);

		auto t = pt->pointerElement();
		if(pv.getPointer() == nullptr)
			return ErrMsg(ev, "null pointer dereference");

		auto& v = *std::move(pv).getPointer();
		auto get_str = [](const char* fmt, auto&&... args) {
			auto u8str = zpr::sprint(fmt, static_cast<decltype(args)&&>(args)...);
			return EvalResult::ofValue(Value::string(unicode::u32StringFromUtf8(u8str)));
		};

		if(t->isInteger())
		{
			return get_str("{}", v.getInteger());
		}
		else if(t->isString())
		{
			return EvalResult::ofValue(v.clone());
		}
		else if(t->isFloating())
		{
			return get_str("{.6f}", v.getFloating());
		}
		else if(t->isBool())
		{
			return get_str("{}", v.getBool() ? "true" : "false");
		}
		else if(t->isChar())
		{
			auto c = v.getChar();
			return EvalResult::ofValue(Value::string(std::u32string(&c, 1)));
		}
		else if(t->isLength())
		{
			auto len = v.getLength();
			return get_str("{.3f}{}", len.value(), DynLength::unitToString(len.unit()));
		}


		return ErrMsg(ev, "cannot convert value of type '{}' to string", t);
	}
}
