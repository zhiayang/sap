// expr.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/type.h"     // for Type
#include "interp/state.h"    // for Interpreter, DefnTree
#include "interp/value.h"    // for Value
#include "interp/interp.h"   // for Ident, NumberLit, InlineTreeExpr, Decla...
#include "interp/basedefs.h" // for InlineObject

namespace sap::interp
{
	ErrorOr<const Type*> Ident::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		auto tree = cs->current();

		std::vector<const Declaration*> decls {};
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

	ErrorOr<const Type*> NumberLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		if(this->is_floating)
			return Ok(Type::makeFloating());
		else
			return Ok(Type::makeInteger());
	}

	ErrorOr<std::optional<Value>> NumberLit::evaluate(Interpreter* cs) const
	{
		if(this->is_floating)
			return Ok(Value::floating(float_value));
		else
			return Ok(Value::integer(int_value));
	}

	ErrorOr<const Type*> StringLit::typecheck_impl(Interpreter* cs, const Type* infer) const
	{
		return Ok(Type::makeString());
	}

	ErrorOr<std::optional<Value>> StringLit::evaluate(Interpreter* cs) const
	{
		return Ok(Value::string(this->string));
	}
}
