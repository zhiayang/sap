// interp.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>
#include <memory> // for unique_ptr
#include <concepts>
#include <stddef.h>
#include <stdint.h>
#include <functional> // for function

#include "defs.h" // for ErrorOr, Ok, TRY
#include "type.h" // for Type
// #include "value.h"
#include "location.h" // for Location

namespace sap::tree
{
	struct InlineObject;
}

namespace sap::interp
{
	struct Value;
	struct Interpreter;

	struct Stmt
	{
		virtual ~Stmt();
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const = 0;

		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const = 0;
		virtual ErrorOr<const Type*> typecheck(Interpreter* cs, const Type* infer = nullptr) const
		{
			if(m_type == nullptr)
				m_type = TRY(this->typecheck_impl(cs, infer));

			return Ok(m_type);
		}

		std::optional<Location> location;

	protected:
		mutable const Type* m_type = nullptr;
	};

	struct Expr : Stmt
	{
		const Type* type(Interpreter* cs) const;
	};

	struct InlineTreeExpr : Expr
	{
		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		mutable std::unique_ptr<tree::InlineObject> object;
	};


	struct QualifiedId
	{
		std::vector<std::string> parents;
		std::string name;
	};

	struct Declaration;
	struct FunctionDecl;

	struct Ident : Expr
	{
		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		QualifiedId name {};
		mutable Declaration* m_resolved_decl = nullptr;
	};

	struct FunctionCall : Expr
	{
		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		struct Arg
		{
			std::optional<std::string> name;
			std::unique_ptr<Expr> value;
		};

		std::unique_ptr<Expr> callee;
		std::vector<Arg> arguments;

	private:
		mutable Declaration* m_resolved_func_decl = nullptr;
	};

	struct NumberLit : Expr
	{
		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::u32string string;
	};

	enum class Op
	{
		Add,
		Subtract,
		Multiply,
		Divide,
	};

	constexpr inline bool isComparisonOp(Op op)
	{
		return false;
	}

	constexpr inline bool isAssignmentOp(Op op)
	{
		return false;
	}

	struct BinaryOp : Expr
	{
		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;
		Op op;
	};



	struct Definition;

	struct Declaration : Stmt
	{
		Declaration(const std::string& name, const Type* ty) : name(name), type(ty) { }

		std::string name;
		const Type* type;

		Definition* resolved_defn = nullptr;
	};

	struct Definition : Stmt
	{
		Definition(Declaration* decl) : declaration(decl) { declaration->resolved_defn = this; }

		// the definition owns its declaration
		std::unique_ptr<Declaration> declaration {};
	};

	struct FunctionDecl : Declaration
	{
		struct Param
		{
			std::string name;
			const Type* type;
			std::unique_ptr<Expr> default_value;
		};


		FunctionDecl(const std::string& name, const Type* type, std::vector<Param>&& params)
		    : Declaration(name, type)
		    , m_params(std::move(params))
		{
		}

		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		const std::vector<Param>& params() const { return m_params; }

	private:
		std::vector<Param> m_params;
	};

	template <std::same_as<FunctionDecl::Param>... P>
	std::vector<FunctionDecl::Param> makeParamList(P&&... params)
	{
		std::vector<FunctionDecl::Param> ret {};
		(ret.push_back(std::move(params)), ...);

		return ret;
	}



	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = std::function<ErrorOr<std::optional<Value>>(Interpreter*, std::vector<Value>&)>;

		BuiltinFunctionDefn(const std::string& name, const Type* type, std::vector<FunctionDecl::Param>&& params,
		    const FuncTy& fn)
		    : Definition(new FunctionDecl(name, type, std::move(params)))
		    , function(fn)
		{
		}

		virtual ErrorOr<std::optional<Value>> evaluate(Interpreter* cs) const override;
		virtual ErrorOr<const Type*> typecheck_impl(Interpreter* cs, const Type* infer = nullptr) const override;

		FuncTy function;
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
