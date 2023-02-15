// ast.h
// Copyright (c) 2022, zhiayang
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

	struct Stmt
	{
		explicit Stmt(Location loc) : m_location(std::move(loc)) { }

		virtual ~Stmt();
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
			const Type* infer = nullptr, //
			bool keep_lvalue = false) const = 0;

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const = 0;

		[[nodiscard]] virtual ErrorOr<EvalResult> evaluate(Evaluator* ev) const;
		[[nodiscard]] virtual ErrorOr<TCResult> typecheck(Typechecker* ts, //
			const Type* infer = nullptr,
			bool keep_lvalue = false) const;

		const Location& loc() const { return m_location; }

		const Type* get_type() const
		{
			assert(m_tc_result.has_value());
			return m_tc_result->type();
		}

	protected:
		Location m_location;
		mutable std::optional<TCResult> m_tc_result {};
	};

	struct Expr : Stmt
	{
		explicit Expr(Location loc) : Stmt(std::move(loc)) { }
	};

	struct TreeInlineExpr : Expr
	{
		explicit TreeInlineExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		mutable std::vector<std::unique_ptr<tree::InlineObject>> objects;
	};

	struct TreeBlockExpr : Expr
	{
		explicit TreeBlockExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		mutable std::unique_ptr<tree::BlockObject> object;
	};


	struct Declaration;
	struct FunctionDecl;

	struct Ident : Expr
	{
		explicit Ident(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		QualifiedId name {};

		void resolve(const Declaration* decl) { m_resolved_decl = decl; }

	private:
		mutable const Declaration* m_resolved_decl = nullptr;
	};

	struct FunctionCall : Expr
	{
		explicit FunctionCall(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

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
		explicit NullLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;
	};

	struct BooleanLit : Expr
	{
		explicit BooleanLit(Location loc, bool value) : Expr(std::move(loc)), value(value) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		bool value;
	};

	struct NumberLit : Expr
	{
		explicit NumberLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		bool is_floating = false;

		int64_t int_value = 0;
		double float_value = 0;
	};

	struct StringLit : Expr
	{
		explicit StringLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::u32string string;
	};

	struct ArrayLit : Expr
	{
		explicit ArrayLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::optional<frontend::PType> elem_type;
		std::vector<std::unique_ptr<Expr>> elements;
	};

	struct FStringExpr : Expr
	{
		explicit FStringExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		using Part = std::variant<std::u32string, std::unique_ptr<Expr>>;
		mutable std::vector<Part> parts;
	};

	struct StructDefn;

	struct StructLit : Expr
	{
		explicit StructLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		using Arg = FunctionCall::Arg;

		QualifiedId struct_name;
		std::vector<Arg> field_inits;
		bool is_anonymous = false;

	private:
		mutable const StructDefn* m_struct_defn = nullptr;
	};

	struct UnaryOp : Expr
	{
		explicit UnaryOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

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

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

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

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

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

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> lhs;
		mutable std::unique_ptr<Expr> rhs;

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

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

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

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> lhs;
		std::string rhs;

		bool is_optional = false;

	private:
		mutable const StructType* m_struct_type = nullptr;
	};

	struct OptionalCheckOp : Expr
	{
		explicit OptionalCheckOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> expr;
	};

	struct NullCoalesceOp : Expr
	{
		explicit NullCoalesceOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

	private:
		mutable enum { Flatmap, ValueOr } m_kind;
	};

	struct DereferenceOp : Expr
	{
		explicit DereferenceOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> expr;
	};

	struct AddressOfOp : Expr
	{
		explicit AddressOfOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> expr;
		bool is_mutable = false;
	};

	struct SubscriptOp : Expr
	{
		explicit SubscriptOp(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> array;
		std::unique_ptr<Expr> index;
	};

	struct LengthExpr : Expr
	{
		explicit LengthExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		DynLength length;
	};

	struct MoveExpr : Expr
	{
		explicit MoveExpr(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> expr;
	};

	struct ArraySpreadOp : Expr
	{
		explicit ArraySpreadOp(Location loc, std::unique_ptr<Expr> expr) : Expr(std::move(loc)), expr(std::move(expr))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> expr;
	};



	struct Block : Stmt
	{
		explicit Block(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		bool checkAllPathsReturn(const Type* return_type);

		std::vector<std::unique_ptr<Stmt>> body;
		std::optional<QualifiedId> target_scope;
	};


	struct IfStmt : Stmt
	{
		explicit IfStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> if_cond;
		std::unique_ptr<Block> if_body;
		std::unique_ptr<Block> else_body;
	};

	struct ReturnStmt : Stmt
	{
		explicit ReturnStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> expr;
		mutable const Type* return_value_type;
	};

	struct BreakStmt : Stmt
	{
		explicit BreakStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;
	};

	struct ContinueStmt : Stmt
	{
		explicit ContinueStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;
	};

	struct WhileLoop : Stmt
	{
		explicit WhileLoop(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> condition;
		std::unique_ptr<Block> body;
	};



	struct ImportStmt : Stmt
	{
		explicit ImportStmt(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::string file_path;
		mutable std::unique_ptr<Block> imported_block;
	};

	struct HookBlock : Stmt
	{
		explicit HookBlock(Location loc) : Stmt(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		ProcessingPhase phase;
		std::unique_ptr<Block> body;
	};



	struct Definition;

	struct Declaration : Stmt
	{
		Declaration(Location loc, const std::string& name) : Stmt(std::move(loc)), name(name) { }

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
		Definition(Location loc, Declaration* decl) : Stmt(std::move(loc)), declaration(decl)
		{
			declaration->resolve(this);
		}

		// the definition owns its declaration
		std::unique_ptr<Declaration> declaration {};
	};

	struct VariableDecl : Declaration
	{
		VariableDecl(Location loc, const std::string& name, bool mut)
			: Declaration(std::move(loc), name)
			, is_mutable(mut)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		bool is_mutable;
	};

	struct VariableDefn : Definition
	{
		VariableDefn(Location loc,
			const std::string& name,
			bool is_mutable,
			bool is_global,
			std::unique_ptr<Expr> init,
			std::optional<frontend::PType> explicit_type)
			: Definition(loc, new VariableDecl(loc, name, is_mutable))
			, initialiser(std::move(init))
			, explicit_type(explicit_type)
			, m_is_global(is_global)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::unique_ptr<Expr> initialiser;
		std::optional<frontend::PType> explicit_type;

	private:
		mutable bool m_is_global = false;
	};

	struct FunctionDecl : Declaration
	{
		struct Param
		{
			std::string name;
			frontend::PType type;
			std::unique_ptr<Expr> default_value;
			Location loc;
		};

		FunctionDecl(Location loc, const std::string& name, std::vector<Param>&& params, frontend::PType return_type)
			: Declaration(loc, name)
			, m_params(std::move(params))
			, m_return_type(return_type)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

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
		FunctionDefn(Location loc,
			const std::string& name,
			std::vector<FunctionDecl::Param> params,
			frontend::PType return_type)
			: Definition(loc, new FunctionDecl(loc, name, std::move(params), return_type))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		ErrorOr<EvalResult> call(Evaluator* ev, std::vector<Value>& args) const;

		std::unique_ptr<Block> body;

		mutable std::vector<std::unique_ptr<VariableDefn>> param_defns;
	};


	struct BuiltinFunctionDefn : Definition
	{
		using FuncTy = std::function<ErrorOr<EvalResult>(Evaluator*, std::vector<Value>&)>;

		BuiltinFunctionDefn(Location loc,
			const std::string& name,
			std::vector<FunctionDecl::Param>&& params,
			frontend::PType return_type,
			const FuncTy& fn)
			: Definition(loc, new FunctionDecl(loc, name, std::move(params), return_type))
			, function(fn)
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		FuncTy function;
	};


	struct StructDecl : Declaration
	{
		StructDecl(Location loc, const std::string& name) : Declaration(loc, name) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;
	};

	struct StructDefn : Definition
	{
		struct Field
		{
			std::string name;
			frontend::PType type;
			std::unique_ptr<Expr> initialiser;
		};

		StructDefn(Location loc, const std::string& name, std::vector<Field> fields)
			: Definition(loc, new StructDecl(loc, name))
			, m_fields(std::move(fields))
		{
		}

		const std::vector<Field>& fields() const { return m_fields; }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

	private:
		mutable std::vector<Field> m_fields;
	};





	struct EnumDecl : Declaration
	{
		EnumDecl(Location loc, const std::string& name) : Declaration(loc, name) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;
	};

	struct EnumDefn : Definition
	{
		struct EnumeratorDecl : Declaration
		{
			EnumeratorDecl(Location loc, const std::string& name) : Declaration(loc, name) { }

			virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
			virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
				const Type* infer = nullptr,
				bool keep_lvalue = false) const override;
		};

		struct EnumeratorDefn : Definition
		{
			EnumeratorDefn(Location loc, const std::string& name, std::unique_ptr<Expr> value)
				: Definition(loc, new EnumeratorDecl(loc, name))
				, m_value(std::move(value))
			{
			}

			ErrorOr<EvalResult> evaluate_impl(Evaluator* ev, int64_t* prev_value) const;

			virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
			virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts,
				const Type* infer = nullptr,
				bool keep_lvalue = false) const override;

		private:
			friend struct EnumDefn;

			std::unique_ptr<Expr> m_value;
		};

		EnumDefn(Location loc,
			const std::string& name,
			frontend::PType type,
			std::vector<EnumeratorDefn> enumerators) //
			: Definition(loc, new EnumDecl(loc, name))
			, m_enumerator_type(std::move(type))
			, m_enumerators(std::move(enumerators))
		{
		}

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		const EnumeratorDefn* getEnumeratorNamed(const std::string& name) const;

	private:
		frontend::PType m_enumerator_type;
		std::vector<EnumeratorDefn> m_enumerators;
		mutable const EnumType* m_resolved_enumerator_type;
	};


	struct EnumLit : Expr
	{
		explicit EnumLit(Location loc) : Expr(std::move(loc)) { }

		virtual ErrorOr<EvalResult> evaluate_impl(Evaluator* ev) const override;
		virtual ErrorOr<TCResult> typecheck_impl(Typechecker* ts, const Type* infer = nullptr, bool keep_lvalue = false)
			const override;

		std::string name;

	private:
		mutable const EnumDefn::EnumeratorDefn* m_enumerator_defn = nullptr;
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
