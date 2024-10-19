// ast.h
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <concepts>
#include <functional> // for function

#include "type.h"     // for Type
#include "util.h"     // for ErrorOr, Ok, TRY
#include "location.h" // for Location

#include "sap/units.h"
#include "tree/base.h"

#include "interp/basedefs.h" // for InlineObject
#include "interp/tc_result.h"
#include "interp/parser_type.h"

namespace sap::tree
{
	struct BlockObject;
	struct InlineObject;
}

namespace sap::interp
{
	struct Value;
	struct DefnTree;
	struct Evaluator;
	struct EvalResult;
	struct Typechecker;
}

namespace sap::interp::cst
{
	struct Stmt;
	struct Expr;
}

namespace sap::interp::ast
{
	struct Stmt
	{
		explicit Stmt(Location loc) : m_location(std::move(loc)) { }

		virtual ~Stmt();
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const
		    = 0;

		[[nodiscard]] virtual ErrorOr<TCResult> typecheck(Typechecker* ts, //
		    const Type* infer = nullptr,
		    bool keep_lvalue = false) const;

		const Location& loc() const { return m_location; }

	protected:
		Location m_location;
	};

	struct Expr : Stmt
	{
		explicit Expr(Location loc) : Stmt(std::move(loc)) { }
	};

	struct TreeInlineExpr : Expr
	{
		explicit TreeInlineExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		mutable std::vector<zst::SharedPtr<tree::InlineObject>> objects;
	};

	struct TreeBlockExpr : Expr
	{
		explicit TreeBlockExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		mutable zst::SharedPtr<tree::BlockObject> object;
	};

	struct Ident : Expr
	{
		explicit Ident(Location loc) : Expr(std::move(loc)) { }
		Ident(Location loc, QualifiedId name_) : Expr(std::move(loc)), name(std::move(name_)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		QualifiedId name {};
	};

	struct FunctionCall : Expr
	{
		explicit FunctionCall(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		struct Arg
		{
			std::optional<std::string> name;
			std::unique_ptr<Expr> value;
		};

		bool rewritten_ufcs = false;
		bool is_optional_ufcs = false;
		std::unique_ptr<Expr> callee;
		std::vector<Arg> arguments;
	};

	struct NullLit : Expr
	{
		explicit NullLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;
	};

	struct BooleanLit : Expr
	{
		explicit BooleanLit(Location loc, bool value_) : Expr(std::move(loc)), value(value_) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		bool value;
	};

	struct NumberLit : Expr
	{
		explicit NumberLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		explicit StringLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::u32string string;
	};

	struct CharLit : Expr
	{
		explicit CharLit(Location loc, char32_t ch) : Expr(std::move(loc)), character(ch) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		char32_t character;
	};

	struct ArrayLit : Expr
	{
		explicit ArrayLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::optional<frontend::PType> elem_type;
		std::vector<std::unique_ptr<Expr>> elements;
	};

	struct FStringExpr : Expr
	{
		explicit FStringExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		using Part = std::variant<std::u32string, std::unique_ptr<Expr>>;
		std::vector<Part> parts;
	};

	struct TypeExpr : Expr
	{
		explicit TypeExpr(Location loc, frontend::PType pt) : Expr(std::move(loc)), ptype(std::move(pt)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		frontend::PType ptype;
	};

	struct CastExpr : Expr
	{
		explicit CastExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
		std::unique_ptr<TypeExpr> target_type;
	};

	struct ImplicitUnionVariantCastExpr : Expr
	{
		explicit ImplicitUnionVariantCastExpr(Location loc, std::unique_ptr<Expr> expr, std::string variant_name)
		    : Expr(std::move(loc)), expr(std::move(expr)), variant_name(std::move(variant_name))
		{
		}

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
		std::string variant_name;
	};


	struct StructDefn;

	struct StructLit : Expr
	{
		explicit StructLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		using Arg = FunctionCall::Arg;

		QualifiedId struct_name;
		std::vector<Arg> field_inits;
		bool is_anonymous = false;
	};

	struct UnaryOp : Expr
	{
		explicit UnaryOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		enum Op
		{
			Plus,
			Minus,
			LogicalNot,
		};

		Op op;
		std::unique_ptr<Expr> expr;
	};

	struct BinaryOp : Expr
	{
		explicit BinaryOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

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

	struct LogicalBinOp : Expr
	{
		explicit LogicalBinOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		enum Op
		{
			And,
			Or,
		};
		Op op;
	};

	struct AssignOp : Stmt
	{
		explicit AssignOp(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

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
		explicit ComparisonOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

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
		explicit DotOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> lhs;
		std::string rhs;

		bool is_optional = false;
	};

	struct NullCoalesceOp : Expr
	{
		explicit NullCoalesceOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;
	};

	struct DereferenceOp : Expr
	{
		explicit DereferenceOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
	};

	struct AddressOfOp : Expr
	{
		explicit AddressOfOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
		bool is_mutable = false;
	};

	struct SubscriptOp : Expr
	{
		explicit SubscriptOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> array;
		std::vector<std::unique_ptr<Expr>> indices;
	};

	struct StructUpdateOp : Expr
	{
		explicit StructUpdateOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		bool is_optional {};
		bool is_assignment {};
		std::unique_ptr<Expr> structure;
		std::vector<std::pair<std::string, std::unique_ptr<Expr>>> updates;
	};



	struct LengthExpr : Expr
	{
		explicit LengthExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		DynLength length;
	};

	struct MoveExpr : Expr
	{
		explicit MoveExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
	};

	struct ArraySpreadOp : Expr
	{
		explicit ArraySpreadOp(Location loc, std::unique_ptr<Expr> expr_) : Expr(std::move(loc)), expr(std::move(expr_))
		{
		}

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
	};



	struct Block : Stmt
	{
		explicit Block(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		bool checkAllPathsReturn(const Type* return_type);

		std::vector<std::unique_ptr<Stmt>> body;
		std::optional<QualifiedId> target_scope;
	};

	struct IfStmtLike : Stmt
	{
		IfStmtLike(Location loc) : Stmt(std::move(loc)) { }

		std::unique_ptr<Block> true_case;
		std::unique_ptr<Block> else_case;
	};


	struct IfStmt : IfStmtLike
	{
		explicit IfStmt(Location loc) : IfStmtLike(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> if_cond;
	};

	struct IfLetUnionStmt : IfStmtLike
	{
		using Binding = cst::IfLetUnionStmt::Binding;

		explicit IfLetUnionStmt(Location loc,
		    QualifiedId variant_name,
		    std::vector<Binding> bindings,
		    std::unique_ptr<Expr> expr)
		    : IfStmtLike(std::move(loc))
		    , variant_name(std::move(variant_name))
		    , bindings(std::move(bindings))
		    , expr(std::move(expr))
		{
		}

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		QualifiedId variant_name;

		std::vector<Binding> bindings;

		std::unique_ptr<Expr> expr;
	};

	struct IfLetOptionalStmt : IfStmtLike
	{
		explicit IfLetOptionalStmt(Location loc, bool is_mutable, std::string name, std::unique_ptr<Expr> expr)
		    : IfStmtLike(std::move(loc)), is_mutable(is_mutable), name(std::move(name)), expr(std::move(expr))
		{
		}

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		bool is_mutable;
		std::string name;
		std::unique_ptr<Expr> expr;
	};


	struct ReturnStmt : Stmt
	{
		explicit ReturnStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> expr;
	};

	struct BreakStmt : Stmt
	{
		explicit BreakStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;
	};

	struct ContinueStmt : Stmt
	{
		explicit ContinueStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;
	};

	struct WhileLoop : Stmt
	{
		explicit WhileLoop(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> condition;
		std::unique_ptr<Block> body;
	};

	struct ForLoop : Stmt
	{
		explicit ForLoop(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		// all 3 of these are optional
		std::unique_ptr<Stmt> init;
		std::unique_ptr<Expr> cond;
		std::unique_ptr<Stmt> update;

		std::unique_ptr<Block> body;
	};


	struct ImportStmt : Stmt
	{
		explicit ImportStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::string file_path;
	};

	struct UsingStmt : Stmt
	{
		explicit UsingStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::string alias;
		QualifiedId module;
	};

	struct HookBlock : Stmt
	{
		explicit HookBlock(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		ProcessingPhase phase;
		std::unique_ptr<Block> body;
	};




	struct Definition : Stmt
	{
		Definition(Location loc, std::string name) : Stmt(std::move(loc)), name(std::move(name)) { }

		virtual ErrorOr<void> declare(Typechecker* ts) const = 0;

		std::string name;

	protected:
		mutable cst::Declaration* declaration = nullptr;
	};

	struct VariableDefn : Definition
	{
		VariableDefn(Location loc,
		    std::string name,
		    bool is_mutable,
		    bool is_global,
		    std::unique_ptr<Expr> init,
		    std::optional<frontend::PType> explicit_type_)
		    : Definition(loc, std::move(name))
		    , initialiser(std::move(init))
		    , explicit_type(explicit_type_)
		    , is_global(is_global)
		    , is_mutable(is_mutable)
		{
		}

		virtual ErrorOr<void> declare(Typechecker* ts) const override;

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::unique_ptr<Expr> initialiser;
		std::optional<frontend::PType> explicit_type;

		bool is_global;
		bool is_mutable;
	};

	struct FunctionDefn : Definition
	{
		struct Param
		{
			std::string name;
			bool is_mutable;
			frontend::PType type;
			std::unique_ptr<Expr> default_value;
			Location loc;
		};

		FunctionDefn(Location loc,
		    std::string name,
		    std::vector<Param> params,
		    frontend::PType return_type,
		    std::vector<std::string> generic_params)
		    : Definition(loc, std::move(name)), params(std::move(params)), return_type(std::move(return_type))
		{
		}

		virtual ErrorOr<void> declare(Typechecker* ts) const override;

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::vector<Param> params;
		frontend::PType return_type;
		std::unique_ptr<Block> body;
	};


	struct BuiltinFunctionDefn : Definition
	{
		using Param = FunctionDefn::Param;
		using FuncTy = ErrorOr<EvalResult> (*)(Evaluator*, std::vector<Value>&);

		BuiltinFunctionDefn(Location loc,
		    std::string name,
		    std::vector<Param>&& params,
		    frontend::PType return_type,
		    const FuncTy& fn)
		    : Definition(loc, std::move(name))
		    , function(fn)
		    , params(std::move(params))
		    , return_type(std::move(return_type))
		{
		}

		virtual ErrorOr<void> declare(Typechecker* ts) const override;

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		FuncTy function;
		std::vector<Param> params;
		frontend::PType return_type;
	};

	struct StructDefn : Definition
	{
		struct Field
		{
			std::string name;
			frontend::PType type;
			std::unique_ptr<Expr> initialiser;
		};

		StructDefn(Location loc, std::string name, std::vector<Field> fields)
		    : Definition(loc, std::move(name)), m_fields(std::move(fields))
		{
		}

		const std::vector<Field>& fields() const { return m_fields; }

		virtual ErrorOr<void> declare(Typechecker* ts) const override;

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

	private:
		std::vector<Field> m_fields;
	};

	struct EnumDefn : Definition
	{
		struct Enumerator
		{
			Location location;
			std::string name;
			std::unique_ptr<Expr> value;

			cst::Declaration* declaration;
		};

		EnumDefn(Location loc,
		    std::string name,
		    frontend::PType type,
		    std::vector<Enumerator> enumerators) //
		    : Definition(loc, std::move(name))
		    , m_enumerator_type(std::move(type))
		    , m_enumerators(std::move(enumerators))
		{
		}

		virtual ErrorOr<void> declare(Typechecker* ts) const override;

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		frontend::PType enumeratorType() const { return m_enumerator_type; }
		const std::vector<Enumerator>& enumerators() const { return m_enumerators; }

	private:
		frontend::PType m_enumerator_type;
		mutable std::vector<Enumerator> m_enumerators;
	};

	struct UnionDefn : Definition
	{
		using Param = FunctionDefn::Param;
		struct Case
		{
			Location location;
			std::string name;
			std::vector<Param> params;
		};

		UnionDefn(Location loc, std::string name, std::vector<Case> cases)
		    : Definition(loc, std::move(name)), cases(std::move(cases))
		{
		}

		virtual ErrorOr<void> declare(Typechecker* ts) const override;

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::vector<Case> cases;
	};


	struct ContextIdent : Expr
	{
		using Arg = FunctionCall::Arg;

		explicit ContextIdent(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
		    const Type* infer = nullptr, //
		    bool keep_lvalue = false) const override;

		std::string name;
		std::vector<Arg> arguments;
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
