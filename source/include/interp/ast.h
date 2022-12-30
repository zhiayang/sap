// ast.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <functional> // for function

#include "type.h" // for Type
#include "util.h" // for ErrorOr, Ok, TRY
// #include "value.h"
#include "location.h" // for Location

#include "interp/value.h"       // for Interpreter, Value
#include "interp/basedefs.h"    // for InlineObject
#include "interp/eval_result.h" // for EvalResult
#include "interp/parser_type.h"

namespace sap::interp
{
	struct Value;
	struct Interpreter;

	struct Stmt
	{
		virtual ~Stmt();
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const = 0;

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const = 0;
		virtual ErrorOr<const Type*> typecheck(Interpreter* cs, const Type* infer = nullptr) const
		{
			if(m_type == nullptr)
				m_type = TRY(this->typecheck_impl(cs, infer));

			return Ok(m_type);
		}

		std::optional<Location> location;

		const Type* get_type() const
		{
			assert(m_type != nullptr);
			return m_type;
		}

	protected:
		mutable const Type* m_type = nullptr;
	};

	struct Expr : Stmt
	{
		const Type* type(Interpreter* cs) const;
	};

	struct InlineTreeExpr : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		mutable std::unique_ptr<tree::InlineObject> object;
	};

	struct Declaration;
	struct FunctionDecl;

	struct Ident : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		QualifiedId name {};
		mutable const Declaration* m_resolved_decl = nullptr;
	};

	struct FunctionCall : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		struct Arg
		{
			std::optional<std::string> name;
			std::unique_ptr<Expr> value;
		};

		bool rewritten_ufcs = false;
		std::unique_ptr<Expr> callee;
		std::vector<Arg> arguments;

	private:
		mutable const Declaration* m_resolved_func_decl = nullptr;
	};

	struct NumberLit : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::u32string string;
	};

	struct BinaryOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		enum Op
		{
			Add,
			Subtract,
			Multiply,
			Divide,
		};
		Op op;
	};

	struct ComparisonOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		enum Op
		{
			EQ,
			NE,
			LT,
			GT,
			LE,
			GE,
		};

		std::unique_ptr<Expr> first;
		std::vector<std::pair<Op, std::unique_ptr<Expr>>> rest;
	};

	struct DotOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::string rhs;
	};





	struct Block : Stmt
	{
		Block() { }

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::vector<std::unique_ptr<Stmt>> body;
	};


	struct Definition;

	struct Declaration : Stmt
	{
		Declaration(const std::string& name) : name(name) { }

		std::string name;
		const Definition* resolved_defn = nullptr;
	};

	struct Definition : Stmt
	{
		Definition(Declaration* decl) : declaration(decl) { declaration->resolved_defn = this; }

		// the definition owns its declaration
		std::unique_ptr<Declaration> declaration {};
	};

	struct VariableDecl : Declaration
	{
		VariableDecl(const std::string& name) : Declaration(name) { }

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;
	};

	struct VariableDefn : Definition
	{
		VariableDefn(const std::string& name, bool is_mutable, std::unique_ptr<Expr> init,
		    std::optional<frontend::PType> explicit_type)
		    : Definition(new VariableDecl(name))
		    , is_mutable(is_mutable)
		    , initialiser(std::move(init))
		    , explicit_type(explicit_type)
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		bool is_mutable;
		std::unique_ptr<Expr> initialiser;
		std::optional<frontend::PType> explicit_type;
	};

	struct FunctionDecl : Declaration
	{
		struct Param
		{
			std::string name;
			frontend::PType type;
			std::unique_ptr<Expr> default_value;
		};

		FunctionDecl(const std::string& name, std::vector<Param>&& params, frontend::PType return_type)
		    : Declaration(name)
		    , m_params(std::move(params))
		    , m_return_type(return_type)
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		const std::vector<Param>& params() const { return m_params; }

	private:
		std::vector<Param> m_params;
		frontend::PType m_return_type;
	};

	template <std::same_as<FunctionDecl::Param>... P>
	std::vector<FunctionDecl::Param> makeParamList(P&&... params)
	{
		std::vector<FunctionDecl::Param> ret {};
		(ret.push_back(std::move(params)), ...);

		return ret;
	}

	struct FunctionDefn : Definition
	{
		FunctionDefn(const std::string& name, std::vector<FunctionDecl::Param> params, frontend::PType return_type)
		    : Definition(new FunctionDecl(name, std::move(params), return_type))
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		ErrorOr<EvalResult> call(Interpreter* cs, std::vector<Value>& args) const;

		std::vector<std::unique_ptr<Stmt>> body;

		mutable std::vector<std::unique_ptr<VariableDefn>> param_defns;
	};


	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = std::function<ErrorOr<EvalResult>(Interpreter*, std::vector<Value>&)>;

		BuiltinFunctionDefn(const std::string& name, std::vector<FunctionDecl::Param>&& params, frontend::PType return_type,
		    const FuncTy& fn)
		    : Definition(new FunctionDecl(name, std::move(params), return_type))
		    , function(fn)
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		FuncTy function;
	};


	struct StructDecl : Declaration
	{
		StructDecl(const std::string& name) : Declaration(name) { }

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;
	};

	struct StructDefn : Definition
	{
		StructDefn(const std::string& name, std::vector<std::pair<std::string, frontend::PType>> fields)
		    : Definition(new StructDecl(name))
		    , fields(std::move(fields))
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::vector<std::pair<std::string, frontend::PType>> fields;
	};
}





template <>
struct zpr::print_formatter<sap::interp::QualifiedId>
{
	template <typename Cb>
	void print(const sap::interp::QualifiedId& qid, Cb&& cb, format_args fmt)
	{
		for(auto& p : qid.parents)
		{
			cb(p.c_str(), p.size());
			cb("::", 2);
		}

		cb(qid.name.c_str(), qid.name.size());
	}
};
