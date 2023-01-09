// ast.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <functional> // for function

#include "type.h"     // for Type
#include "util.h"     // for ErrorOr, Ok, TRY
#include "location.h" // for Location

#include "interp/basedefs.h" // for InlineObject
#include "interp/tc_result.h"
#include "interp/parser_type.h"

namespace sap::interp
{
	struct Value;
	struct DefnTree;
	struct Evaluator;
	struct EvalResult;
	struct Typechecker;

	struct Stmt
	{
		virtual ~Stmt();
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const = 0;

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const = 0;
		virtual ErrorOr<TCResult> typecheck(Typechecker* ts, const Type* infer = nullptr) const
		{
			if(not m_tc_result.has_value())
				m_tc_result = TRY(this->typecheck_impl(ts, infer));

			return Ok(*m_tc_result);
		}

		std::optional<Location> location;

		const Type* get_type() const
		{
			assert(m_tc_result.has_value());
			return m_tc_result->type();
		}

	protected:
		mutable std::optional<TCResult> m_tc_result {};
	};

	struct Expr : Stmt
	{
	};

	struct InlineTreeExpr : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		mutable std::vector<std::unique_ptr<tree::InlineObject>> objects;
	};

	struct Declaration;
	struct FunctionDecl;

	struct Ident : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		QualifiedId name {};

		void resolve(const Declaration* decl) { m_resolved_decl = decl; }

	private:
		mutable const Declaration* m_resolved_decl = nullptr;
	};

	struct FunctionCall : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		struct Arg
		{
			std::optional<std::string> name;
			std::unique_ptr<Expr> value;
		};

		bool rewritten_ufcs = false;
		bool is_optional_ufcs = false;
		std::unique_ptr<Expr> callee;
		std::vector<Arg> arguments;

	private:
		mutable bool m_ufcs_self_is_mutable = false;
		mutable const Declaration* m_resolved_func_decl = nullptr;
	};

	struct NullLit : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;
	};

	struct NumberLit : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::u32string string;
	};

	struct StructDefn;
	struct StructLit : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		using Arg = FunctionCall::Arg;

		QualifiedId struct_name;
		std::vector<Arg> field_inits;

	private:
		mutable const StructDefn* m_struct_defn = nullptr;
	};


	struct BinaryOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		enum Op
		{
			Add,
			Subtract,
			Multiply,
			Divide,
			Modulo,
		};
		Op op;
	};

	struct AssignOp : Stmt
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		enum Op
		{
			None,
			Add,
			Subtract,
			Multiply,
			Divide,
			Modulo,
		};
		Op op;
	};

	struct ComparisonOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

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
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::string rhs;

		bool is_optional = false;

	private:
		mutable const StructType* m_struct_type = nullptr;
	};

	struct OptionalCheckOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> expr;
	};

	struct NullCoalesceOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

	private:
		mutable enum { Flatmap, ValueOr } m_kind;
	};

	struct DereferenceOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> expr;
	};

	struct AddressOfOp : Expr
	{
		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> expr;
		bool is_mutable = false;
	};



	struct Block : Stmt
	{
		Block() { }

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		bool checkAllPathsReturn(const Type* return_type);

		std::vector<std::unique_ptr<Stmt>> body;
		std::optional<QualifiedId> target_scope;
	};


	struct IfStmt : Stmt
	{
		IfStmt() { }

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> if_cond;
		std::unique_ptr<Block> if_body;
		std::unique_ptr<Block> else_body;
	};

	struct ReturnStmt : Stmt
	{
		ReturnStmt() { }

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> expr;
		mutable const Type* return_value_type;
	};



	struct Definition;

	struct Declaration : Stmt
	{
		Declaration(const std::string& name) : name(name) { }

		std::string name;

		const Definition* definition() const { return m_resolved_defn; }
		void resolve(const Definition* defn) { m_resolved_defn = defn; }

		const DefnTree* declaredTree() const { return m_declared_tree; }
		void declareAt(const DefnTree* tree) { m_declared_tree = tree; }

	private:
		const Definition* m_resolved_defn = nullptr;
		const DefnTree* m_declared_tree = nullptr;
	};

	struct Definition : Stmt
	{
		Definition(Declaration* decl) : declaration(decl) { declaration->resolve(this); }

		// the definition owns its declaration
		std::unique_ptr<Declaration> declaration {};
	};

	struct VariableDecl : Declaration
	{
		VariableDecl(const std::string& name, bool mut) : Declaration(name), is_mutable(mut) { }

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		bool is_mutable;
	};

	struct VariableDefn : Definition
	{
		VariableDefn(const std::string& name, bool is_mutable, std::unique_ptr<Expr> init,
		    std::optional<frontend::PType> explicit_type)
		    : Definition(new VariableDecl(name, is_mutable))
		    , initialiser(std::move(init))
		    , explicit_type(explicit_type)
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

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

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

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

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		ErrorOr<EvalResult> call(Evaluator* ev, std::vector<Value>& args) const;

		std::unique_ptr<Block> body;

		mutable std::vector<std::unique_ptr<VariableDefn>> param_defns;
	};


	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = std::function<ErrorOr<EvalResult>(Evaluator*, std::vector<Value>&)>;

		BuiltinFunctionDefn(const std::string& name, std::vector<FunctionDecl::Param>&& params, frontend::PType return_type,
		    const FuncTy& fn)
		    : Definition(new FunctionDecl(name, std::move(params), return_type))
		    , function(fn)
		{
		}

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

		FuncTy function;
	};


	struct StructDecl : Declaration
	{
		StructDecl(const std::string& name) : Declaration(name) { }

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;
	};

	struct StructDefn : Definition
	{
		struct Field
		{
			std::string name;
			frontend::PType type;
			std::unique_ptr<Expr> initialiser;
		};

		StructDefn(const std::string& name, std::vector<Field> fields)
		    : Definition(new StructDecl(name))
		    , m_fields(std::move(fields))
		{
		}

		const std::vector<Field>& fields() const { return m_fields; }

		virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr) const override;

	private:
		mutable std::vector<Field> m_fields;
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
