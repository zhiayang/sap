// interp.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include <stddef.h>

#include <vector>
#include <functional>
#include <memory>
#include <string>

#include <zpr.h>

#include "type.h"
#include "location.h"

namespace sap::tree
{
	struct InlineObject;
}

namespace sap::interp
{
	struct Interpreter;
	struct Stmt
	{
		virtual ~Stmt();

		std::optional<Location> location;
	};

	struct Expr : Stmt
	{
		const Type* type = nullptr;
	};

	struct InlineTreeExpr : Expr
	{
		std::unique_ptr<tree::InlineObject> object;
	};


	struct QualifiedId
	{
		std::vector<std::string> parents;
		std::string name;
	};

	struct Ident : Expr
	{
		QualifiedId name;
	};

	struct FunctionCall : Expr
	{
		struct Arg
		{
			std::optional<std::string> name;
			std::unique_ptr<Expr> value;
		};

		std::unique_ptr<Expr> callee;
		std::vector<Arg> arguments;
	};

	struct NumberLit : Expr
	{
		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
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
		Definition(const std::string& name, const Type* type) : declaration(new Declaration(name, type))
		{
			declaration->resolved_defn = this;
		}

		Declaration* declaration = nullptr;
	};

	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = std::function<std::unique_ptr<Expr>(Interpreter*, const std::vector<const Expr*>&)>;

		BuiltinFunctionDefn(const std::string& name, const Type* type, const FuncTy& fn)
			: Definition(name, type), function(fn) { }

		FuncTy function;
	};

	std::optional<std::unique_ptr<tree::InlineObject>> runScriptExpression(Interpreter* cs, const Expr* expr);
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
