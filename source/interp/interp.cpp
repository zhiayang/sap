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
		using BFD = BuiltinFunctionDefn;
		using Param = FunctionDecl::Param;

		auto any = Type::makeAny();
		auto tio = Type::makeTreeInlineObj();

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
		this->m_stack_frames.push_back(std::unique_ptr<StackFrame>(new StackFrame(nullptr)));
	}

	bool Interpreter::canImplicitlyConvert(const Type* from, const Type* to) const
	{
		if(from == to || to->isAny())
			return true;

		// TODO: not if these are all the cases
		return false;
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

	const Type* Expr::type(Interpreter* cs) const
	{
		if(m_type != nullptr)
			return m_type;

		error(this->location, "cannot get type of expression that was not typechecked!");
	}
}
