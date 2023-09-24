// cst.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/eval_result.h"

namespace sap::interp::cst
{
	struct Stmt
	{
		explicit Stmt(Location loc) : m_location(std::move(loc)) { }
		const Location& loc() const { return m_location; }

		bool isExpr() const { return m_is_expr; }

		virtual ~Stmt();
		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const = 0;

		[[nodiscard]] virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const;

	protected:
		Location m_location;
		bool m_is_expr = false;
	};

	struct Expr : Stmt
	{
		explicit Expr(Location loc, const Type* type) : Stmt(std::move(loc)), m_type(type) { m_is_expr = true; }

		const Type* type() const { return m_type; }

	protected:
		const Type* m_type;
	};

	struct TreeInlineExpr : Expr
	{
		explicit TreeInlineExpr(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<zst::SharedPtr<tree::InlineObject>> objects;
	};

	struct TreeBlockExpr : Expr
	{
		explicit TreeBlockExpr(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		zst::SharedPtr<tree::BlockObject> object;
	};


	struct Definition;

	struct Ident : Expr
	{
		Ident(Location loc, const Type* type, QualifiedId name, const Definition* defn)
		    : Expr(std::move(loc), type), name(std::move(name)), resolved_defn(defn)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		QualifiedId name {};
		const Definition* resolved_defn;
	};

	struct FunctionCall : Expr
	{
		explicit FunctionCall(Location loc, const Type* type, std::vector<std::unique_ptr<Expr>> args)
		    : Expr(std::move(loc), type), arguments(std::move(args))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<std::unique_ptr<Expr>> arguments;
	};

	struct NullLit : Expr
	{
		explicit NullLit(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
	};

	struct BooleanLit : Expr
	{
		explicit BooleanLit(Location loc, bool value_) : Expr(std::move(loc), Type::makeBool()), value(value_) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		bool value;
	};

	struct NumberLit : Expr
	{
		explicit NumberLit(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		explicit StringLit(Location loc) : Expr(std::move(loc), Type::makeString()) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::u32string string;
	};

	struct CharLit : Expr
	{
		explicit CharLit(Location loc, char32_t ch) : Expr(std::move(loc), Type::makeChar()), character(ch) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		char32_t character;
	};

	struct ArrayLit : Expr
	{
		explicit ArrayLit(Location loc, const Type* array_type) : Expr(std::move(loc), array_type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<std::unique_ptr<Expr>> elements;
	};

	struct FStringExpr : Expr
	{
		using Part = zst::Either<std::u32string, std::unique_ptr<Expr>>;

		explicit FStringExpr(Location loc, std::vector<Part> parts)
		    : Expr(std::move(loc), Type::makeString()), parts(std::move(parts))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<Part> parts;
	};

	struct CastExpr : Expr
	{
		enum class CastKind
		{
			None,
			Implicit,
			FloatToInteger,
			CharToInteger,
			IntegerToChar,
		};

		explicit CastExpr(Location loc, std::unique_ptr<Expr> expr, CastKind kind, const Type* target)
		    : Expr(std::move(loc), target), expr(std::move(expr)), cast_kind(kind)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		CastKind cast_kind;
	};

	struct StructDefn;

	struct StructLit : Expr
	{
		explicit StructLit(Location loc, const Type* struct_type, const StructDefn* struct_defn)
		    : Expr(std::move(loc), struct_type), struct_defn(struct_defn)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		// it's either an expression that's part of the literal, or an expression
		// that's from the default initialiser of the struct.
		std::vector<zst::Either<std::unique_ptr<Expr>, Expr*>> field_inits;
		const StructDefn* struct_defn;
	};

	struct UnaryOp : Expr
	{
		explicit UnaryOp(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

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
		enum Op
		{
			Add,
			Subtract,
			Multiply,
			Divide,
			Modulo,
		};

		explicit BinaryOp(Location loc, const Type* type, Op op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
		    : Expr(std::move(loc), type), lhs(std::move(lhs)), rhs(std::move(rhs)), op(op)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		Op op;
	};

	struct LogicalBinOp : Expr
	{
		explicit LogicalBinOp(Location loc) : Expr(std::move(loc), Type::makeBool()) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

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
		enum Op
		{
			None,
			Add,
			Subtract,
			Multiply,
			Divide,
			Modulo,
		};

		explicit AssignOp(Location loc, Op op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
		    : Stmt(std::move(loc)), lhs(std::move(lhs)), rhs(std::move(rhs)), op(op)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		Op op;
	};

	struct ComparisonOp : Expr
	{
		enum Op
		{
			EQ,
			NE,
			LT,
			GT,
			LE,
			GE,
		};


		explicit ComparisonOp(Location loc,
		    std::unique_ptr<Expr> first,
		    std::vector<std::pair<Op, std::unique_ptr<Expr>>> rest)
		    : Expr(std::move(loc), Type::makeBool()), first(std::move(first)), rest(std::move(rest))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> first;
		std::vector<std::pair<Op, std::unique_ptr<Expr>>> rest;
	};

	struct DotOp : Expr
	{
		explicit DotOp(Location loc,
		    const Type* type,
		    bool is_optional,
		    const StructType* struct_type,
		    std::unique_ptr<Expr> expr,
		    std::string field_name)
		    : Expr(std::move(loc), type)
		    , expr(std::move(expr))
		    , field_name(std::move(field_name))
		    , struct_type(struct_type)
		    , is_optional(is_optional)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		std::string field_name;

		const StructType* struct_type;
		bool is_optional;
	};

	struct OptionalCheckOp : Expr
	{
		explicit OptionalCheckOp(Location loc) : Expr(std::move(loc), Type::makeBool()) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
	};

	struct NullCoalesceOp : Expr
	{
		enum class Kind
		{
			Flatmap,
			ValueOr
		};

		explicit NullCoalesceOp(Location loc, const Type* type, Kind kind) : Expr(std::move(loc), type), kind(kind) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;
		Kind kind;
	};

	struct DereferenceOp : Expr
	{
		explicit DereferenceOp(Location loc, const Type* type, std::unique_ptr<Expr> expr)
		    : Expr(std::move(loc), type), expr(std::move(expr))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
	};

	struct AddressOfOp : Expr
	{
		explicit AddressOfOp(Location loc, const Type* type, bool is_mutable)
		    : Expr(std::move(loc), type), is_mutable(is_mutable)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		bool is_mutable;
	};

	struct SubscriptOp : Expr
	{
		explicit SubscriptOp(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> array;
		std::vector<std::unique_ptr<Expr>> indices;
	};

	struct StructUpdateOp : Expr
	{
		explicit StructUpdateOp(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> structure;
		std::vector<std::pair<std::string, std::unique_ptr<Expr>>> updates;
	};



	struct LengthExpr : Expr
	{
		explicit LengthExpr(Location loc) : Expr(std::move(loc), Type::makeLength()) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		DynLength length;
	};

	struct MoveExpr : Expr
	{
		explicit MoveExpr(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
	};

	struct ArraySpreadOp : Expr
	{
		explicit ArraySpreadOp(Location loc, const Type* type, std::unique_ptr<Expr> expr_)
		    : Expr(std::move(loc), type), expr(std::move(expr_))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
	};



	struct Block : Stmt
	{
		explicit Block(Location loc, std::vector<std::unique_ptr<Stmt>> body)
		    : Stmt(std::move(loc)), body(std::move(body))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<std::unique_ptr<Stmt>> body;
	};


	struct IfStmt : Stmt
	{
		explicit IfStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> if_cond;
		std::unique_ptr<Block> if_body;
		std::unique_ptr<Block> else_body;
	};

	struct ReturnStmt : Stmt
	{
		explicit ReturnStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		const Type* return_value_type;
	};

	struct BreakStmt : Stmt
	{
		explicit BreakStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
	};

	struct ContinueStmt : Stmt
	{
		explicit ContinueStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
	};

	struct WhileLoop : Stmt
	{
		explicit WhileLoop(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> condition;
		std::unique_ptr<Block> body;
	};

	struct ForLoop : Stmt
	{
		explicit ForLoop(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		// all 3 of these are optional
		std::unique_ptr<Stmt> init;
		std::unique_ptr<Expr> cond;
		std::unique_ptr<Stmt> update;

		std::unique_ptr<Block> body;
	};


	struct ImportStmt : Stmt
	{
		explicit ImportStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::string file_path;
		std::unique_ptr<Block> imported_block;
	};

	struct HookBlock : Stmt
	{
		explicit HookBlock(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		ProcessingPhase phase;
		std::unique_ptr<Block> body;
	};

	struct Definition : Stmt
	{
		Definition(Location loc, std::string name, const Type* type)
		    : Stmt(std::move(loc)), name(std::move(name)), type(type)
		{
		}

		std::string name;
		const Type* type;
	};

	struct VariableDefn : Definition
	{
		VariableDefn(Location loc, std::string name, bool is_global, std::unique_ptr<Expr> init, const Type* type)
		    : Definition(std::move(loc), std::move(name), type), initialiser(std::move(init)), is_global(is_global)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> initialiser;
		const Type* type;
		bool is_global;
	};

	struct FunctionDefn : Definition
	{
		struct Param
		{
			Location loc;
			std::unique_ptr<VariableDefn> defn;
			std::unique_ptr<Expr> default_value;
		};

		FunctionDefn(Location loc,
		    std::string name,
		    std::vector<Param> params,
		    const Type* signature,
		    std::unique_ptr<Block> body)
		    : Definition(std::move(loc), std::move(name), signature), body(std::move(body)), params(std::move(params))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		ErrorOr<EvalResult> call(Evaluator* ev, std::vector<Value>& args) const;

		std::unique_ptr<Block> body;
		std::vector<Param> params;
	};


	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = ErrorOr<EvalResult> (*)(Evaluator*, std::vector<Value>&);

		BuiltinFunctionDefn(Location loc, std::string name, const Type* signature, FuncTy fn)
		    : Definition(std::move(loc), std::move(name), signature), function(std::move(fn))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		FuncTy function;
	};

	struct EnumeratorDefn : Definition
	{
		EnumeratorDefn(Location loc, std::string name, const Type* type)
		    : Definition(std::move(loc), std::move(name), type)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> value;
		EnumeratorDefn* prev_sibling = nullptr;
	};


	struct StructDefn : Definition
	{
		struct Field
		{
			std::string name;
			const Type* type;
			std::unique_ptr<Expr> initialiser;
		};

		StructDefn(Location loc, std::string name, const Type* type, std::vector<Field> fields)
		    : Definition(std::move(loc), std::move(name), type), fields(std::move(fields))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<Field> fields;
	};
}
