// cst.h
// Copyright (c) 2023, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/eval_result.h"

namespace sap::interp
{
	struct DefnTree;
}

namespace sap::interp::cst
{
	struct Expr;

	struct ExprOrDefaultPtr
	{
		/* implicit */
		ExprOrDefaultPtr(std::unique_ptr<Expr> expr) : m_expr(std::move(expr)), m_default(nullptr), m_is_default(false)
		{
		}

		/* implicit */
		template <typename T>
		ExprOrDefaultPtr(std::unique_ptr<T> expr)
		    requires(std::derived_from<T, Expr>)
		    : m_expr(std::move(expr)), m_default(nullptr), m_is_default(false)
		{
		}


		/* implicit */ ExprOrDefaultPtr(const Expr* def) : m_expr(), m_default(def), m_is_default(true) { }

		const Expr* operator->() const
		{
			if(m_is_default)
				return m_default;
			else
				return m_expr.get();
		}

		std::unique_ptr<Expr> take_expr()
		{
			assert(not m_is_default);
			return std::move(m_expr);
		}

		const Expr* default_expr() const
		{
			assert(m_is_default);
			return m_default;
		}

		bool is_default() const { return m_is_default; }
		bool is_null() const
		{
			if(m_is_default)
				return m_default == nullptr;
			else
				return m_expr == nullptr;
		}

	private:
		std::unique_ptr<Expr> m_expr;
		const Expr* m_default;
		bool m_is_default;
	};

	// using ExprOrDefaultPtr = Either<std::unique_ptr<Expr>, const Expr*>;

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

	struct EmptyStmt : Stmt
	{
		explicit EmptyStmt(Location loc) : Stmt(std::move(loc)) { }
		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const { return EvalResult::ofVoid(); }
	};

	struct Expr : Stmt
	{
		explicit Expr(Location loc, const Type* type) : Stmt(std::move(loc)), m_type(type) { m_is_expr = true; }

		const Type* type() const { return m_type; }

	protected:
		const Type* m_type;
	};

	struct TypeExpr : Expr
	{
		explicit TypeExpr(Location loc, const Type* type) : Expr(std::move(loc), type) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
	};

	struct TreeInlineExpr : Expr
	{
		explicit TreeInlineExpr(Location loc, std::vector<zst::SharedPtr<tree::InlineObject>> objs)
		    : Expr(std::move(loc), Type::makeTreeInlineObj()), objects(std::move(objs))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<zst::SharedPtr<tree::InlineObject>> objects;
	};

	struct TreeBlockExpr : Expr
	{
		explicit TreeBlockExpr(Location loc, zst::SharedPtr<tree::BlockObject> obj)
		    : Expr(std::move(loc), Type::makeTreeBlockObj()), object(std::move(obj))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		zst::SharedPtr<tree::BlockObject> object;
	};


	struct Definition;
	struct Declaration;

	struct Ident : Expr
	{
		Ident(Location loc, const Type* type, QualifiedId name, const Declaration* decl)
		    : Expr(std::move(loc), type), name(std::move(name)), resolved_decl(decl)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		QualifiedId name {};
		const Declaration* resolved_decl;
	};

	struct FunctionCall : Expr
	{
		enum class UFCSKind
		{
			None,
			ByValue,
			ConstPointer,
			MutablePointer,
		};

		explicit FunctionCall(Location loc,
		    const Type* type,
		    UFCSKind ufcs_kind,
		    std::vector<ExprOrDefaultPtr> args,
		    const Declaration* callee)
		    : Expr(std::move(loc), type), ufcs_kind(ufcs_kind), arguments(std::move(args)), callee(callee)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		UFCSKind ufcs_kind;
		std::vector<ExprOrDefaultPtr> arguments;
		const Declaration* callee;
	};

	struct NullLit : Expr
	{
		explicit NullLit(Location loc) : Expr(std::move(loc), Type::makeNullPtr()) { }

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
		explicit NumberLit(Location loc, const Type* type, bool is_floating)
		    : Expr(std::move(loc), type), is_floating(is_floating)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		explicit StringLit(Location loc, std::u32string str)
		    : Expr(std::move(loc), Type::makeString()), string(std::move(str))
		{
		}

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
		explicit ArrayLit(Location loc, const Type* array_type, std::vector<std::unique_ptr<Expr>> elements)
		    : Expr(std::move(loc), array_type), elements(std::move(elements))
		{
		}

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
			Pointer,
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

	struct UnionVariantCastExpr : Expr
	{
		explicit UnionVariantCastExpr(Location loc,
		    ExprOrDefaultPtr expr,
		    const Type* type,
		    const UnionType* union_type,
		    size_t variant_idx)
		    : Expr(std::move(loc), type), expr(std::move(expr)), union_type(union_type), variant_idx(variant_idx)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		ExprOrDefaultPtr expr;
		const UnionType* union_type;
		size_t variant_idx;
	};

	struct StructDefn;

	struct StructLit : Expr
	{
		explicit StructLit(Location loc, const Type* struct_type, std::vector<ExprOrDefaultPtr> field_inits)
		    : Expr(std::move(loc), struct_type), field_inits(std::move(field_inits))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		// it's either an expression that's part of the literal, or an expression
		// that's from the default initialiser of the struct.
		std::vector<ExprOrDefaultPtr> field_inits;
	};

	struct UnaryOp : Expr
	{
		enum Op
		{
			Plus,
			Minus,
			LogicalNot,
		};

		explicit UnaryOp(Location loc, const Type* type, Op op, std::unique_ptr<Expr> expr)
		    : Expr(std::move(loc), type), expr(std::move(expr)), op(op)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		Op op;
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
		enum Op
		{
			And,
			Or,
		};

		explicit LogicalBinOp(Location loc, Op op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
		    : Expr(std::move(loc), Type::makeBool()), lhs(std::move(lhs)), rhs(std::move(rhs)), op(op)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

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

	struct StructContextSelf : Expr
	{
		explicit StructContextSelf(Location loc, const StructType* type) : Expr(std::move(loc), type) { }
		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
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

	struct NullCoalesceOp : Expr
	{
		enum class Kind
		{
			Flatmap,
			ValueOr
		};

		explicit NullCoalesceOp(Location loc,
		    const Type* type,
		    Kind kind,
		    std::unique_ptr<Expr> lhs,
		    std::unique_ptr<Expr> rhs)
		    : Expr(std::move(loc), type), lhs(std::move(lhs)), rhs(std::move(rhs)), kind(kind)
		{
		}

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
		explicit AddressOfOp(Location loc, const Type* type, bool is_mutable, std::unique_ptr<Expr> expr)
		    : Expr(std::move(loc), type), expr(std::move(expr)), is_mutable(is_mutable)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		bool is_mutable;
	};

	struct SubscriptOp : Expr
	{
		explicit SubscriptOp(Location loc,
		    const Type* type,
		    std::unique_ptr<Expr> array,
		    std::vector<std::unique_ptr<Expr>> indices)
		    : Expr(std::move(loc), type), array(std::move(array)), indices(std::move(indices))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> array;
		std::vector<std::unique_ptr<Expr>> indices;
	};

	struct StructUpdateOp : Expr
	{
		struct Update
		{
			std::string field;
			std::unique_ptr<Expr> expr;
		};

		explicit StructUpdateOp(Location loc,
		    const Type* type,
		    std::unique_ptr<Expr> structure,
		    std::vector<Update> updates,
		    bool is_assignment,
		    bool is_optional)
		    : Expr(std::move(loc), type)
		    , structure(std::move(structure))
		    , updates(std::move(updates))
		    , is_assignment(is_assignment)
		    , is_optional(is_optional)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> structure;
		std::vector<Update> updates;
		bool is_assignment;
		bool is_optional;
	};

	struct LengthExpr : Expr
	{
		explicit LengthExpr(Location loc, DynLength len)
		    : Expr(std::move(loc), Type::makeLength()), length(std::move(len))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		DynLength length;
	};

	struct MoveExpr : Expr
	{
		explicit MoveExpr(Location loc, std::unique_ptr<Expr> expr)
		    : Expr(std::move(loc), expr->type()), expr(std::move(expr))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
	};

	struct ArraySpreadOp : Expr
	{
		explicit ArraySpreadOp(Location loc, const Type* type, std::unique_ptr<Expr> expr)
		    : Expr(std::move(loc), type), expr(std::move(expr))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
	};

	struct VariadicPackExpr : Expr
	{
		explicit VariadicPackExpr(Location loc, const Type* type, std::vector<ExprOrDefaultPtr> exprs)
		    : Expr(std::move(loc), type), exprs(std::move(exprs))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<ExprOrDefaultPtr> exprs;
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
		explicit ReturnStmt(Location loc, const Type* type, std::unique_ptr<Expr> expr)
		    : Stmt(std::move(loc)), expr(std::move(expr)), type(type)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		const Type* type;
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
		explicit WhileLoop(Location loc, std::unique_ptr<Expr> condition, std::unique_ptr<Block> body)
		    : Stmt(std::move(loc)), condition(std::move(condition)), body(std::move(body))
		{
		}

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
		explicit ImportStmt(Location loc, std::unique_ptr<Block> block) : Stmt(std::move(loc)), block(std::move(block))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Block> block;
	};

	struct HookBlock : Stmt
	{
		explicit HookBlock(Location loc, std::unique_ptr<Block> body) : Stmt(std::move(loc)), body(std::move(body)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		ProcessingPhase phase;
		std::unique_ptr<Block> body;
	};




	struct Declaration
	{
		Declaration(Location loc, DefnTree* tree, std::string name, const Type* type, bool is_mutable)
		    : location(std::move(loc)), name(std::move(name)), type(type), is_mutable(is_mutable), m_declared_tree(tree)
		{
		}

		bool operator==(const Declaration&) const = default;
		bool operator!=(const Declaration&) const = default;

		const DefnTree* declaredTree() const { return m_declared_tree; }
		const Definition* definition() const
		{
			assert(m_definition);
			return m_definition;
		}

		void define(const Definition* defn) { m_definition = defn; }

		Location location;
		std::string name;
		const Type* type;
		bool is_mutable;

		// literally just for functions...
		struct FunctionDecl
		{
			bool operator==(const FunctionDecl&) const = default;
			bool operator!=(const FunctionDecl&) const = default;

			struct Param
			{
				std::string name;
				const Type* type;
				std::unique_ptr<Expr> default_value;

				bool operator==(const Param&) const = default;
				bool operator!=(const Param&) const = default;
			};

			std::vector<Param> params;
		};

		std::optional<FunctionDecl> function_decl {};

	private:
		const Definition* m_definition = nullptr;
		const DefnTree* m_declared_tree = nullptr;
	};

	struct Definition : Stmt
	{
		Definition(Location loc, const Declaration* decl) : Stmt(std::move(loc)), declaration(decl) { }

		const Declaration* declaration;
	};

	struct VariableDefn : Definition
	{
		VariableDefn(Location loc, const Declaration* decl, bool is_global, ExprOrDefaultPtr init, bool is_mutable)
		    : Definition(std::move(loc), decl)
		    , initialiser(std::move(init))
		    , is_global(is_global)
		    , is_mutable(is_mutable)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		ExprOrDefaultPtr initialiser;

		bool is_global;
		bool is_mutable;
	};

	struct FunctionDefn : Definition
	{
		struct Param
		{
			std::unique_ptr<VariableDefn> defn;
			const Expr* default_value;
		};

		FunctionDefn(Location loc, const Declaration* decl, std::vector<Param> params, std::unique_ptr<Block> body)
		    : Definition(std::move(loc), decl), params(std::move(params)), body(std::move(body))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		ErrorOr<EvalResult> call(Evaluator* ev, std::vector<Value>& args) const;

		std::vector<Param> params;
		std::unique_ptr<Block> body;
	};


	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = ErrorOr<EvalResult> (*)(Evaluator*, std::vector<Value>&);

		BuiltinFunctionDefn(Location loc, const Declaration* decl, FuncTy fn)
		    : Definition(std::move(loc), decl), function(std::move(fn))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		FuncTy function;
	};


	struct EnumeratorDefn : Definition
	{
		EnumeratorDefn(Location loc, const Declaration* decl, EnumeratorDefn* prev_sibling)
		    : Definition(std::move(loc), decl), prev_sibling(prev_sibling)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		EnumeratorDefn* prev_sibling = nullptr;
		std::unique_ptr<Expr> value;
	};

	struct EnumDefn : Definition
	{
		EnumDefn(Location loc, const Declaration* decl, std::vector<std::unique_ptr<EnumeratorDefn>> enumerators)
		    : Definition(std::move(loc), decl), enumerators(std::move(enumerators))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		const EnumeratorDefn* getEnumeratorNamed(const std::string& name) const
		{
			for(auto& e : this->enumerators)
			{
				if(e->declaration->name == name)
					return e.get();
			}
			return nullptr;
		}

		std::vector<std::unique_ptr<EnumeratorDefn>> enumerators;
	};


	struct StructDefn : Definition
	{
		struct Field
		{
			std::string name;
			const Type* type;
			std::unique_ptr<Expr> initialiser;
		};

		StructDefn(Location loc, const Declaration* decl, std::vector<Field> fields)
		    : Definition(std::move(loc), decl), fields(std::move(fields))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<Field> fields;
	};

	struct UnionDefn : Definition
	{
		struct Case
		{
			std::string name;
			const StructType* type;
			std::vector<std::unique_ptr<Expr>> default_values;
		};

		UnionDefn(Location loc, const Declaration* decl, std::vector<Case> cases)
		    : Definition(std::move(loc), decl), cases(std::move(cases))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::vector<Case> cases;
	};


	struct EnumeratorExpr : Expr
	{
		explicit EnumeratorExpr(Location loc, const EnumType* enum_type, const EnumeratorDefn* enumerator)
		    : Expr(std::move(loc), enum_type), enumerator(enumerator)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		const EnumeratorDefn* enumerator;
	};

	struct UnionExpr : Expr
	{
		explicit UnionExpr(Location loc,
		    const UnionType* union_type,
		    const UnionDefn* union_defn,
		    size_t case_index,
		    std::vector<ExprOrDefaultPtr> values)
		    : Expr(std::move(loc), union_type)
		    , union_defn(union_defn)
		    , case_index(std::move(case_index))
		    , values(std::move(values))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		const UnionDefn* union_defn;
		size_t case_index;
		std::vector<ExprOrDefaultPtr> values;
	};


	struct IfLetUnionStmt : Stmt
	{
		struct Binding
		{
			enum class Kind
			{
				Named,
				Skip,
				Ellipsis,
			};

			Location loc;

			Kind kind;
			bool mut = false;
			bool reference = false;
			std::string field_name {};
			std::string binding_name {};
		};

		explicit IfLetUnionStmt(Location loc,
		    const UnionType* union_type,
		    size_t variant_index,
		    std::unique_ptr<VariableDefn> rhs_defn,
		    std::vector<std::unique_ptr<VariableDefn>> binding_defns,
		    std::unique_ptr<Expr> expr)
		    : Stmt(std::move(loc))
		    , union_type(std::move(union_type))
		    , variant_index(variant_index)
		    , rhs_defn(std::move(rhs_defn))
		    , binding_defns(std::move(binding_defns))
		    , expr(std::move(expr))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		const UnionType* union_type;
		size_t variant_index;

		std::unique_ptr<VariableDefn> rhs_defn;
		std::vector<std::unique_ptr<VariableDefn>> binding_defns;

		std::unique_ptr<Expr> expr;

		std::unique_ptr<Block> true_case {};
		std::unique_ptr<Block> else_case {};
	};

	struct IfLetOptionalStmt : Stmt
	{
		explicit IfLetOptionalStmt(Location loc, std::unique_ptr<Expr> expr, std::unique_ptr<VariableDefn> defn)
		    : Stmt(std::move(loc)), expr(std::move(expr)), defn(std::move(defn))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;

		std::unique_ptr<Expr> expr;
		std::unique_ptr<VariableDefn> defn;
		std::unique_ptr<Block> true_case {};
		std::unique_ptr<Block> else_case {};
	};

}
