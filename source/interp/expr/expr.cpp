// expr.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap.h"
#include "interp/tree.h"
#include "interp/state.h"
#include "interp/interp.h"

namespace sap::interp
{
	ErrorOr<const Type*> Ident::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto tree = cs->current();

		std::vector<Declaration*> decls {};
		if(auto result = tree->lookup(this->name); result.ok())
			decls = result.take_value();
		else
			error(this->location, "{}", result.take_error());

		assert(decls.size() > 0);
		if(decls.size() == 1)
		{
			m_type = TRY(decls[0]->typecheck(cs));
			m_resolved_decl = decls[0];

			return Ok(m_type);
		}
		else
		{
			// note: function calls should not call typecheck() on an ident directly,
			// since we do not (and don't want to) provide a mechanism to return multiple
			// "overloaded" types for a single identifier.
			return ErrFmt("ambiguous '{}'", this->name);
		}
	}

	ErrorOr<std::optional<Value>> Ident::evaluate(Interpreter* cs) const
	{
		return Ok(std::nullopt);
	}



	ErrorOr<const Type*> InlineTreeExpr::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return ErrFmt("aoeu");
	}

	ErrorOr<std::optional<Value>> InlineTreeExpr::evaluate(Interpreter* cs) const
	{
		// i guess we have to assert that we cannot evaluate something twice, because
		// otherwise this becomes untenable.
		assert(this->object != nullptr);

		return Ok(Value::treeInlineObject(std::move(this->object)));
	}



	ErrorOr<const Type*> BinaryOp::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return Ok(nullptr);
	}

	ErrorOr<std::optional<Value>> BinaryOp::evaluate(Interpreter* cs) const
	{
		return Ok(std::nullopt);
	}



	ErrorOr<const Type*> NumberLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return Ok(Type::makeNumber());
	}

	ErrorOr<std::optional<Value>> NumberLit::evaluate(Interpreter* cs) const
	{
		if(this->is_floating)
			return Ok(Value::decimal(float_value));
		else
			return Ok(Value::integer(int_value));
	}
}
