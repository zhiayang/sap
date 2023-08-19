// compare_op.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "util.h" // for is_one_of

#include "tree/base.h"

#include "interp/ast.h"         // for ComparisonOp, ComparisonOp::Op, Expr
#include "interp/type.h"        // for Type, ArrayType
#include "interp/value.h"       // for Value
#include "interp/interp.h"      // for Interpreter
#include "interp/eval_result.h" // for EvalResult, TRY_VALUE

namespace sap::interp::ast
{
	static const char* op_to_string(ComparisonOp::Op op)
	{
		switch(op)
		{
			case ComparisonOp::Op::LT: return "<";
			case ComparisonOp::Op::GT: return ">";
			case ComparisonOp::Op::LE: return "<=";
			case ComparisonOp::Op::GE: return ">=";
			case ComparisonOp::Op::EQ: return "==";
			case ComparisonOp::Op::NE: return "!=";
		}
		util::unreachable();
	}

	static bool can_compare(ComparisonOp::Op op, const Type* lhs, const Type* rhs)
	{
		if(lhs->isArray() && rhs->isArray())
			return can_compare(op, lhs->arrayElement(), rhs->arrayElement());

		if(util::is_one_of(op, ComparisonOp::Op::EQ, ComparisonOp::Op::NE))
		{
			return (lhs == rhs)                            //
			    || (lhs->isPointer() && rhs->isNullPtr())  //
			    || (lhs->isNullPtr() && rhs->isPointer())  //
			    || (lhs->isOptional() && rhs->isNullPtr()) //
			    || (lhs->isNullPtr() && rhs->isOptional());
		}

		// if we are doing ordering, make sure that the enum is integral (if it's an enum)
		if((lhs->isEnum() && not lhs->toEnum()->elementType()->isInteger())
		    || (rhs->isEnum() && not rhs->toEnum()->elementType()->isInteger()))
			return false;

		return lhs == rhs                      //
		    && (not lhs->isTreeInlineObj())    //
		    && (not lhs->isTreeBlockObj())     //
		    && (not lhs->isLayoutObject())     //
		    && (not lhs->isTreeInlineObjRef()) //
		    && (not lhs->isTreeBlockObjRef())  //
		    && (not lhs->isLayoutObjectRef())  //
		    && (not lhs->isFunction());
	}

	static bool do_compare(ComparisonOp::Op op, const Value& lhs, const Value& rhs)
	{
		using enum ComparisonOp::Op;

		auto cmp_lt = []<typename T>(const T& a, const T& b) -> bool { return a < b; };

		auto cmp_eq = []<typename T>(const T& a, const T& b) -> bool { return a == b; };

		auto do_cmp = [&]<typename T>(const T& a, const T& b) -> bool {
			switch(op)
			{
				case LT: return cmp_lt(a, b);
				case GT: return not(cmp_lt(a, b) || cmp_eq(a, b));
				case LE: return cmp_lt(a, b) || cmp_eq(a, b);
				case GE: return not cmp_lt(a, b);
				case EQ: return cmp_eq(a, b);
				case NE: return not cmp_eq(a, b);
			}
			util::unreachable();
		};

		if(lhs.isArray())
		{
			assert(rhs.isArray());
			auto& larr = lhs.getArray();
			auto& rarr = rhs.getArray();

			if(op == EQ)
			{
				if(larr.size() != rarr.size())
					return false;

				for(size_t i = 0; i < larr.size(); i++)
				{
					if(not do_compare(EQ, larr[i], rarr[i]))
						return false;
				}
				return true;
			}
			else if(op == NE)
			{
				if(larr.size() != rarr.size())
					return true;

				for(size_t i = 0; i < larr.size(); i++)
				{
					if(not do_compare(EQ, larr[i], rarr[i]))
						return true;
				}
				return false;
			}
			else
			{
				for(size_t i = 0; i < std::min(larr.size(), rarr.size()); i++)
				{
					if(not do_compare(op, larr[i], rarr[i]))
						return false;
				}

				switch(op)
				{
					case LT: return larr.size() < rarr.size();
					case GT: return larr.size() > rarr.size();
					case LE: return larr.size() <= rarr.size();
					case GE: return larr.size() >= rarr.size();

					case EQ:
					case NE: assert(false);
				}
			}
		}
		else if(lhs.type() == rhs.type())
		{
			if(lhs.isBool())
			{
				assert(op == EQ || op == NE);
				auto eq = (lhs.getBool() == rhs.getBool());
				return op == EQ ? eq : not eq;
			}
			else if(lhs.isFunction())
			{
				assert(op == EQ || op == NE);
				auto eq = (lhs.getFunction() == rhs.getFunction());
				return op == EQ ? eq : not eq;
			}
			else if(lhs.isChar())
			{
				return do_cmp(lhs.getChar(), rhs.getChar());
			}
			else if(lhs.isFloating())
			{
				return do_cmp(lhs.getFloating(), rhs.getFloating());
			}
			else if(lhs.isInteger())
			{
				return do_cmp(lhs.getInteger(), rhs.getInteger());
			}
			else if(lhs.isEnum())
			{
				assert(rhs.isEnum());
				assert(lhs.type() == rhs.type());

				return do_compare(op, lhs.getEnumerator(), rhs.getEnumerator());
			}
		}
		else if(lhs.type()->isOptional() && rhs.type()->isNullPtr())
		{
			assert(op == EQ || op == NE);
			bool have_value = lhs.getOptional().has_value();
			return (op == EQ ? not have_value : have_value);
		}
		else if(lhs.type()->isNullPtr() && rhs.type()->isOptional())
		{
			assert(op == EQ || op == NE);
			bool have_value = rhs.getOptional().has_value();
			return (op == EQ ? not have_value : have_value);
		}
		else if(lhs.type()->isPointer() && rhs.type()->isNullPtr())
		{
			assert(op == EQ || op == NE);
			bool have_value = lhs.getPointer() != nullptr;
			return (op == EQ ? not have_value : have_value);
		}
		else if(lhs.type()->isNullPtr() && rhs.type()->isPointer())
		{
			assert(op == EQ || op == NE);
			bool have_value = rhs.getPointer() != nullptr;
			return (op == EQ ? not have_value : have_value);
		}

		sap::internal_error("??? unsupported comparison: {}, {}", lhs.type(), rhs.type());
	}




	ErrorOr<TCResult> ComparisonOp::typecheck_impl(Typechecker* ts, const Type* infer, bool keep_lvalue) const
	{
		assert(not this->rest.empty());

		auto lhs = TRY(this->first->typecheck(ts)).type();

		auto op = this->rest[0].first;
		auto rhs = TRY(this->rest[0].second->typecheck(ts)).type();

		for(size_t i = 0;;)
		{
			if(not can_compare(op, lhs, rhs))
				return ErrMsg(ts, "types '{}' and '{}' are not comparable with operator '{}'", lhs, rhs,
				    op_to_string(op));

			if(i + 1 == this->rest.size())
				break;

			i += 1;
			lhs = rhs;
			op = this->rest[i].first;
			rhs = TRY(this->rest[i].second->typecheck(ts)).type();
		}

		return TCResult::ofRValue(Type::makeBool());
	}

	ErrorOr<EvalResult> ComparisonOp::evaluate_impl(Evaluator* ev) const
	{
		assert(not this->rest.empty());

		auto lhs = TRY_VALUE(this->first->evaluate(ev));

		auto op = this->rest[0].first;
		auto rhs = TRY_VALUE(this->rest[0].second->evaluate(ev));

		for(size_t i = 0;;)
		{
			assert(can_compare(op, lhs.type(), rhs.type()));

			if(not do_compare(op, lhs, rhs))
				return EvalResult::ofValue(Value::boolean(false));

			if(i + 1 == this->rest.size())
				break;

			i += 1;
			lhs = std::move(rhs);
			op = this->rest[i].first;
			rhs = TRY_VALUE(this->rest[i].second->evaluate(ev));
		}

		return EvalResult::ofValue(Value::boolean(true));
	}
}
