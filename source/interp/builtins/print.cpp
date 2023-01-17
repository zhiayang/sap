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

	static ErrorOr<std::u32string> to_string(Evaluator* ev, const Value& v)
	{
		auto t = v.type();
		auto get_str = [](const char* fmt, auto&&... args) -> ErrorOr<std::u32string> {
			auto u8str = zpr::sprint(fmt, static_cast<decltype(args)&&>(args)...);
			return Ok(unicode::u32StringFromUtf8(u8str));
		};

		if(t->isInteger())
		{
			return get_str("{}", v.getInteger());
		}
		else if(t->isString())
		{
			return Ok(v.getUtf32String());
		}
		else if(t->isFloating())
		{
			return get_str("{.4f}", v.getFloating());
		}
		else if(t->isBool())
		{
			return get_str("{}", v.getBool() ? "true" : "false");
		}
		else if(t->isChar())
		{
			auto c = v.getChar();
			return Ok(std::u32string(&c, 1));
		}
		else if(t->isLength())
		{
			auto len = v.getLength();
			return get_str("{.2f}{}", len.value(), DynLength::unitToString(len.unit()));
		}
		else if(t->isOptional())
		{
			if(not v.haveOptionalValue())
				return get_str("null");
			else
				return to_string(ev, **v.getOptional());
		}
		else if(t->isArray())
		{
			auto& arr = v.getArray();
			if(arr.empty())
				return Ok<std::u32string>(U"[]");

			std::u32string ret {};
			for(size_t i = 0; i < arr.size(); i++)
			{
				if(i != 0)
					ret += U", ";
				ret += TRY(to_string(ev, arr[i]));
			}

			return Ok(U"[ " + ret + U" ]");
		}

		return ErrMsg(ev, "cannot convert value of type '{}' to string", t);
	}




	ErrorOr<EvalResult> to_string(Evaluator* ev, std::vector<Value>& args)
	{
		assert(args.size() == 1);
		auto& pv = args[0];
		auto pt = pv.type();

		if(not pt->isPointer())
			return ErrMsg(ev, "expected pointer type, got '{}'", pt);

		if(pv.getPointer() == nullptr)
			return ErrMsg(ev, "null pointer dereference");

		auto& v = *std::move(pv).getPointer();
		auto str = TRY(to_string(ev, v));

		return EvalResult::ofValue(Value::string(str));
	}
}
