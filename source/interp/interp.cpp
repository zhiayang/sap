// interp.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "location.h" // for error

#include "interp/ast.h"         // for Definition, makeParamList, Expr, Stmt
#include "interp/type.h"        // for Type
#include "interp/interp.h"      // for StackFrame, Interpreter, DefnTree
#include "interp/builtin.h"     // for bold1, bold_italic1, italic1, print
#include "interp/eval_result.h" // for EvalResult

namespace sap::interp
{
	static void define_builtins(Interpreter* cs, DefnTree* builtin_ns)
	{
		using namespace sap::frontend;

		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto any = PType::named(TYPE_ANY);
		auto tio = PType::named(TYPE_TREE_INLINE);

		auto define_builtin = [&](auto&&... xs) {
			auto ret = std::make_unique<BFD>(std::forward<decltype(xs)>(xs)...);
			cs->addBuiltinDefinition(std::move(ret))->typecheck(cs);
		};

		auto _ = cs->pushTree(builtin_ns);

		define_builtin("__bold1", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::bold1);
		define_builtin("__italic1", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::italic1);
		define_builtin("__bold_italic1", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::bold_italic1);

		// TODO: make these variadic
		define_builtin("print", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::print);
		define_builtin("println", makeParamList(Param { .name = "_", .type = any }), tio, &builtin::println);
	}

	Interpreter::Interpreter() : m_top(new DefnTree("__top_level", /* parent: */ nullptr)), m_current(m_top.get())
	{
		define_builtins(this, m_top->lookupOrDeclareNamespace("builtin"));

		// always start with a top level frame.
		this->m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(this, nullptr)));
	}

	bool Interpreter::canImplicitlyConvert(const Type* from, const Type* to) const
	{
		if(from == to || to->isAny())
			return true;

		else if(from->isNullPtr() && to->isPointer())
			return true;

		else if(from->isPointer() && to->isPointer())
			return from->toPointer()->elementType() == to->toPointer()->elementType() && from->isMutablePointer();

		// TODO: not if these are all the cases
		return false;
	}

	Value Interpreter::castValue(Value value, const Type* to) const
	{
		if(value.type() == to)
			return value;

		// TODO: handle 'any'
		auto from_type = value.type();
		if(from_type->isNullPtr() && to->isPointer())
		{
			return to->isMutablePointer() //
			         ? Value::mutablePointer(to->toPointer()->elementType(), nullptr)
			         : Value::pointer(to->toPointer()->elementType(), nullptr);
		}
		else if(from_type->isMutablePointer() && to->isPointer()
		        && from_type->toPointer()->elementType() == to->toPointer()->elementType())
		{
			return Value::pointer(to->toPointer()->elementType(), value.getPointer());
		}
		else
		{
			return value;
		}
	}

	ErrorOr<const Type*> Interpreter::resolveType(const frontend::PType& ptype)
	{
		using namespace sap::frontend;
		if(ptype.isNamed())
		{
			auto& name = ptype.name();
			if(name.parents.empty() && !name.top_level)
			{
				auto& nn = name.name;
				if(nn == TYPE_INT)
					return Ok(Type::makeInteger());
				else if(nn == TYPE_ANY)
					return Ok(Type::makeAny());
				else if(nn == TYPE_BOOL)
					return Ok(Type::makeBool());
				else if(nn == TYPE_CHAR)
					return Ok(Type::makeChar());
				else if(nn == TYPE_VOID)
					return Ok(Type::makeVoid());
				else if(nn == TYPE_FLOAT)
					return Ok(Type::makeFloating());
				else if(nn == TYPE_STRING)
					return Ok(Type::makeString());
				else if(nn == TYPE_TREE_INLINE)
					return Ok(Type::makeTreeInlineObj());
			}

			auto decl = TRY(this->current()->lookup(name));
			if(decl.size() > 1)
				return ErrFmt("ambiguous type '{}'", name);

			if(auto struct_decl = dynamic_cast<const StructDecl*>(decl[0]); struct_decl)
				return Ok(struct_decl->get_type());
		}
		else if(ptype.isArray())
		{
			return Ok(Type::makeArray(TRY(this->resolveType(ptype.getArrayElement())), ptype.isVariadicArray()));
		}
		else if(ptype.isFunction())
		{
			std::vector<const Type*> params {};
			for(auto& t : ptype.getTypeList())
				params.push_back(TRY(this->resolveType(t)));

			assert(params.size() > 0);
			auto ret = params.back();
			params.pop_back();

			return Ok(Type::makeFunction(std::move(params), ret));
		}
		else if(ptype.isPointer())
		{
			return Ok(Type::makePointer(TRY(this->resolveType(ptype.getPointerElement())), ptype.isMutablePointer()));
		}
		else
		{
		}

		return ErrFmt("unknown type");
	}


	ErrorOr<const Definition*> Interpreter::getDefinitionForType(const Type* type)
	{
		auto it = m_type_definitions.find(type);
		if(it == m_type_definitions.end())
			return ErrFmt("no definition for type '{}'", type);
		else
			return Ok(it->second);
	}

	ErrorOr<void> Interpreter::addTypeDefinition(const Type* type, const Definition* defn)
	{
		if(m_type_definitions.contains(type))
			return ErrFmt("type '{}' was already defined", type);

		m_type_definitions[type] = defn;
		return Ok();
	}

	Definition* Interpreter::addBuiltinDefinition(std::unique_ptr<Definition> defn)
	{
		m_builtin_defns.push_back(std::move(defn));
		return m_builtin_defns.back().get();
	}


	ErrorOr<EvalResult> Interpreter::run(const Stmt* stmt)
	{
		if(auto res = stmt->typecheck(this); res.is_err())
			error(stmt->location, "{}", res.take_error());

		return stmt->evaluate(this);
	}

	void Interpreter::dropValue(Value&& value)
	{
		// TODO: call destructors
	}


	void StackFrame::dropTemporaries()
	{
		while(not m_temporaries.empty())
		{
			m_interp->dropValue(std::move(m_temporaries.back()));
			m_temporaries.pop_back();
		}
	}
}
