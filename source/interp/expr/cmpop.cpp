// cmpop.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/ast.h"
#include "interp/type.h"
#include "interp/value.h"
#include "interp/interp.h"
#include "interp/basedefs.h"

namespace sap::interp
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
	}

	static bool can_compare(ComparisonOp::Op op, const Type* lhs, const Type* rhs)
	{
		if(lhs->isArray() && rhs->isArray())
			return can_compare(op, lhs->toArray()->elementType(), rhs->toArray()->elementType());

		if(util::is_one_of(op, ComparisonOp::Op::EQ, ComparisonOp::Op::NE))
			return lhs == rhs;

		return lhs == rhs && (not lhs->isTreeInlineObj()) && (not lhs->isFunction());
	}

	static bool do_compare(ComparisonOp::Op op, const Value& lhs, const Value& rhs)
	{
		auto cmp_lt = []<typename T>(const T& a, const T& b) -> bool {
			return a < b;
		};

		auto cmp_eq = []<typename T>(const T& a, const T& b) -> bool {
			return a == b;
		};

		auto do_cmp = [&]<typename T>(const T& a, const T& b) -> bool {
			switch(op)
			{
				case ComparisonOp::Op::LT: return cmp_lt(a, b);
				case ComparisonOp::Op::GT: return not(cmp_lt(a, b) || cmp_eq(a, b));
				case ComparisonOp::Op::LE: return cmp_lt(a, b) || cmp_eq(a, b);
				case ComparisonOp::Op::GE: return not cmp_lt(a, b);
				case ComparisonOp::Op::EQ: return cmp_eq(a, b);
				case ComparisonOp::Op::NE: return not cmp_eq(a, b);
			}
		};

		if(lhs.isArray())
		{
			assert(rhs.isArray());
			auto& larr = lhs.getArray();
			auto& rarr = rhs.getArray();

			for(size_t i = 0; i < std::min(larr.size(), rarr.size()); i++)
			{
				if(not do_compare(op, larr[i], rarr[i]))
					return false;
			}

			return larr.size() < rarr.size();
		}
		else if(lhs.type() == rhs.type())
		{
			if(lhs.isBool())
			{
				assert(op == ComparisonOp::Op::EQ || op == ComparisonOp::Op::NE);
				auto eq = (lhs.getBool() == rhs.getBool());
				return op == ComparisonOp::Op::EQ ? eq : not eq;
			}
			else if(lhs.isFunction())
			{
				assert(op == ComparisonOp::Op::EQ || op == ComparisonOp::Op::NE);
				auto eq = (lhs.getFunction() == rhs.getFunction());
				return op == ComparisonOp::Op::EQ ? eq : not eq;
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
		}

		sap::internal_error("??? unsupported comparison");
	}




	ErrorOr<const Type*> ComparisonOp::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		assert(not this->rest.empty());

		auto lhs = TRY(this->first->typecheck(cs));

		auto op = this->rest[0].first;
		auto rhs = TRY(this->rest[0].second->typecheck(cs));

		for(size_t i = 0;;)
		{
			if(not can_compare(op, lhs, rhs))
				return ErrFmt("types '{}' and '{}' are not comparable with operator '{}'", lhs, rhs, op_to_string(op));

			if(i + 1 == this->rest.size())
				break;

			i += 1;
			lhs = rhs;
			op = this->rest[i].first;
			rhs = TRY(this->rest[i].second->typecheck(cs));
		}

		return Ok(Type::makeBool());
	}

	ErrorOr<std::optional<Value>> ComparisonOp::evaluate(Interpreter* cs) const
	{
		assert(not this->rest.empty());

		// TODO: handle empty optionals
		auto lhs = *TRY(this->first->evaluate(cs));

		auto op = this->rest[0].first;
		auto rhs = *TRY(this->rest[0].second->evaluate(cs));

		for(size_t i = 0;;)
		{
			assert(can_compare(op, lhs.type(), rhs.type()));

			if(not do_compare(op, lhs, rhs))
				return Ok(Value::boolean(false));

			if(i + 1 == this->rest.size())
				break;

			i += 1;
			lhs = std::move(rhs);
			op = this->rest[i].first;
			rhs = *TRY(this->rest[i].second->evaluate(cs));
		}

		return Ok(Value::boolean(true));
	}
}
