// dump_ast.cpp
// Copyright (c) 2024, yuki
// SPDX-License-Identifier: Apache-2.0

#include "picojson.h"
#include "interp/ast.h"

namespace pj = picojson;
namespace ast = sap::interp::ast;

namespace sap::interp::ast
{
	extern const char* op_to_string(UnaryOp::Op op);
	extern const char* op_to_string(BinaryOp::Op op);
	extern const char* op_to_string(AssignOp::Op op);
	extern const char* op_to_string(LogicalBinOp::Op op);
	extern const char* op_to_string(ComparisonOp::Op op);

	static std::string phase_to_string(ProcessingPhase phase)
	{
		switch(phase)
		{
			using enum ProcessingPhase;
			case Start: return "Start";
			case Preamble: return "Preamble";
			case Layout: return "Layout";
			case Position: return "Position";
			case PostLayout: return "PostLayout";
			case Finalise: return "Finalise";
			case Render: return "Render";
		}
	}
}

namespace test
{
	using V = pj::value;
	using O = pj::object;
	using A = pj::array;

#define DC(T, x) dynamic_cast<const ast::T*>(x)

#define _MATCH(T, x, xn)                         \
	if(auto xn = dynamic_cast<const ast::T*>(x)) \
		return dump##T(xn);

#define M(T, x) _MATCH(T, x, a##__COUNTER__)

	pj::value dumpStmt(const ast::Stmt* x);
	pj::value dumpExpr(const ast::Expr* x);

	static pj::value dumpBlock(const ast::Block* x)
	{
		O obj {};
		obj["ast"] = V("Block");

		pj::array stmts {};
		for(auto& s : x->body)
			stmts.push_back(dumpStmt(s.get()));

		obj["body"] = V(std::move(stmts));
		return V(obj);
	}

	static pj::value dumpAssignOp(const ast::AssignOp* x)
	{
		return V(O {
		    { "ast", V("AssignOp") },
		    { "op", V(op_to_string(x->op)) },
		    { "lhs", dumpExpr(x->lhs.get()) },
		    { "rhs", dumpExpr(x->rhs.get()) },
		});
	}

	static pj::value dumpBreakStmt(const ast::BreakStmt* x)
	{
		return V(O { { "ast", V("BreakStmt") } });
	}

	static pj::value dumpContinueStmt(const ast::ContinueStmt* x)
	{
		return V(O { { "ast", V("ContinueStmt") } });
	}

	static pj::value dumpReturnStmt(const ast::ReturnStmt* x)
	{
		return V(O {
		    { "ast", V("ReturnStmt") },
		    { "expr", dumpExpr(x->expr.get()) },
		});
	}

	static pj::value dumpWhileLoop(const ast::WhileLoop* x)
	{
		return V(O {
		    { "ast", V("WhileLoop") },
		    { "cond", dumpExpr(x->condition.get()) },
		    { "body", dumpStmt(x->body.get()) },
		});
	}

	static pj::value dumpForLoop(const ast::ForLoop* x)
	{
		return V(O {
		    { "ast", V("ForLoop") },
		    { "init", dumpStmt(x->init.get()) },
		    { "cond", dumpExpr(x->cond.get()) },
		    { "update", dumpStmt(x->update.get()) },
		    { "body", dumpStmt(x->body.get()) },
		});
	}

	static pj::value dumpImportStmt(const ast::ImportStmt* x)
	{
		return V(O {
		    { "ast", V("ImportStmt") },
		    { "path", V(x->file_path) },
		});
	}

	static pj::value dumpUsingStmt(const ast::UsingStmt* x)
	{
		return V(O {
		    { "ast", V("UsingStmt") },
		    { "alias", V(x->alias) },
		    { "module", V(x->module.str()) },
		});
	}

	static pj::value dumpHookBlock(const ast::HookBlock* x)
	{
		return V(O {
		    { "ast", V("HookBlock") },
		    { "phase", V(ast::phase_to_string(x->phase)) },
		    { "body", dumpStmt(x->body.get()) },
		});
	}

	static pj::value dumpVariableDefn(const ast::VariableDefn* x)
	{
		return V(O {
		    { "ast", V("VariableDefn") },
		    { "name", V(x->name) },
		    { "explicit_type", x->explicit_type.has_value() ? V(x->explicit_type->str()) : V() },
		    { "initialiser", dumpExpr(x->initialiser.get()) },
		    { "is_global", V(x->is_global) },
		    { "is_mutable", V(x->is_mutable) },
		});
	}

	static pj::value dump_param(const ast::FunctionDefn::Param& p)
	{
		return V(O {
		    { "name", V(p.name) },
		    { "type", V(p.type.str()) },
		    { "is_mutable", V(p.is_mutable) },
		    { "default_value", dumpExpr(p.default_value.get()) },
		});
	}

	static pj::value dumpFunctionDefn(const ast::FunctionDefn* x)
	{
		A params {};
		for(auto& p : x->params)
			params.push_back(dump_param(p));

		return V(O {
		    { "ast", V("FunctionDefn") },
		    { "name", V(x->name) },
		    { "params", V(std::move(params)) },
		    { "return_type", V(x->return_type.str()) },
		    { "body", dumpStmt(x->body.get()) },
		});
	}

	static pj::value dumpBuiltinFunctionDefn(const ast::BuiltinFunctionDefn* x)
	{
		A params {};
		for(auto& p : x->params)
			params.push_back(dump_param(p));

		return V(O {
		    { "ast", V("BuiltinFunctionDefn") },
		    { "name", V(x->name) },
		    { "params", V(std::move(params)) },
		    { "return_type", V(x->return_type.str()) },
		});
	}

	static pj::value dumpStructDefn(const ast::StructDefn* x)
	{
		A fields {};
		for(auto& f : x->fields())
		{
			fields.push_back(V(O {
			    { "name", V(f.name) },
			    { "type", V(f.type.str()) },
			    { "initialiser", dumpExpr(f.initialiser.get()) },
			}));
		}

		return V(O {
		    { "ast", V("StructDefn") },
		    { "name", V(x->name) },
		    { "fields", V(std::move(fields)) },
		});
	}

	static pj::value dumpEnumDefn(const ast::EnumDefn* x)
	{
		A enums {};
		for(auto& e : x->enumerators())
		{
			enums.push_back(V(O {
			    { "name", V(e.name) },
			    { "value", dumpExpr(e.value.get()) },
			}));
		}

		return V(O {
		    { "ast", V("EnumDefn") },
		    { "name", V(x->name) },
		    { "enumerators", V(std::move(enums)) },
		    { "enumerator_type", V(x->enumeratorType().str()) },
		});
	}

	static pj::value dumpUnionDefn(const ast::UnionDefn* x)
	{
		A cases {};
		for(auto& c : x->cases)
		{
			A params {};
			for(auto& p : c.params)
				params.push_back(dump_param(p));

			cases.push_back(V(O {
			    { "name", V(c.name) },
			    { "params", V(std::move(params)) },
			}));
		}

		return V(O {
		    { "ast", V("UnionDefn") },
		    { "name", V(x->name) },
		    { "cases", V(std::move(cases)) },
		});
	}

	static pj::value dumpDefinition(const ast::Definition* x)
	{
		M(EnumDefn, x);
		M(UnionDefn, x);
		M(StructDefn, x);
		M(VariableDefn, x);
		M(FunctionDefn, x);
		M(BuiltinFunctionDefn, x);

		util::unreachable();
	}

	static pj::value dumpIfStmt(const ast::IfStmt* x)
	{
		return V(O {
		    { "ast", V("IfStmt") },
		    { "cond", dumpExpr(x->if_cond.get()) },
		    { "true", dumpStmt(x->true_case.get()) },
		    { "else", dumpStmt(x->else_case.get()) },
		});
	}

	static pj::value dumpIfLetOptionalStmt(const ast::IfLetOptionalStmt* x)
	{
		return V(O {
		    { "ast", V("IfLetOptionalStmt") },
		    { "name", V(x->name) },
		    { "is_mutable", V(x->is_mutable) },
		    { "expr", dumpExpr(x->expr.get()) },
		    { "true", dumpStmt(x->true_case.get()) },
		    { "else", dumpStmt(x->else_case.get()) },
		});
	}

	static pj::value dumpIfLetUnionStmt(const ast::IfLetUnionStmt* x)
	{
		A bindings {};
		for(auto& b : x->bindings)
		{
			using enum ast::IfLetUnionStmt::Binding::Kind;
			bindings.push_back(V(O {
			    { "field", V(b.field_name) },
			    { "binding", V(b.binding_name) },
			    { "kind",
			        V(b.kind == Skip      ? "skip"
			            : b.kind == Named ? "named"
			                              : "ellipsis") },
			    { "mut", V(b.mut) },
			    { "reference", V(b.reference) },
			}));
		}

		return V(O {
		    { "ast", V("IfLetUnionStmt") },
		    { "variant_name", V(x->variant_name.str()) },
		    { "expr", dumpExpr(x->expr.get()) },
		    { "true", dumpStmt(x->true_case.get()) },
		    { "else", dumpStmt(x->else_case.get()) },
		    { "bindings", V(std::move(bindings)) },
		});
	}

	static pj::value dumpIfStmtLike(const ast::IfStmtLike* x)
	{
		M(IfStmt, x);
		M(IfLetUnionStmt, x);
		M(IfLetOptionalStmt, x);

		util::unreachable();
	}

	pj::value dumpStmt(const ast::Stmt* x)
	{
		if(x == nullptr)
			return V();

		M(Expr, x);

		M(Block, x);
		M(AssignOp, x);
		M(BreakStmt, x);
		M(ContinueStmt, x);
		M(ReturnStmt, x);
		M(IfStmtLike, x);
		M(ForLoop, x);
		M(WhileLoop, x);
		M(ImportStmt, x);
		M(UsingStmt, x);
		M(HookBlock, x);

		M(Definition, x);

		util::unreachable();
	}

	static pj::value dumpLengthExpr(const ast::LengthExpr* x)
	{
		return V(O {
		    { "ast", V("LengthExpr") },
		    { "length", V(x->length.str()) },
		});
	}

	static pj::value dumpMoveExpr(const ast::MoveExpr* x)
	{
		return V(O {
		    { "ast", V("MoveExpr") },
		    { "expr", dumpExpr(x->expr.get()) },
		});
	}

	static pj::value dumpCharLit(const ast::CharLit* x)
	{
		return V(O {
		    { "ast", V("CharLit") },
		    { "char", V(static_cast<int64_t>(x->character)) },
		});
	}

	static pj::value dumpNullLit(const ast::NullLit* x)
	{
		return V(O { { "ast", V("NullLit") } });
	}

	static pj::value dumpBooleanLit(const ast::BooleanLit* x)
	{
		return V(O {
		    { "ast", V("BooleanLit") },
		    { "value", V(x->value) },
		});
	}

	static pj::value dumpNumberLit(const ast::NumberLit* x)
	{
		return V(O {
		    { "ast", V("NumberLit") },
		    { "is_floating", V(x->is_floating) },
		    { "value", x->is_floating ? V(x->float_value) : V(x->int_value) },
		});
	}

	static pj::value dumpStringLit(const ast::StringLit* x)
	{
		return V(O {
		    { "ast", V("StringLit") },
		    { "string", V(unicode::stringFromU32String(x->string)) },
		});
	}

	static pj::value dumpArrayLit(const ast::ArrayLit* x)
	{
		A values {};
		for(auto& v : x->elements)
			values.push_back(dumpExpr(v.get()));

		return V(O {
		    { "ast", V("ArrayLit") },
		    { "type", x->elem_type.has_value() ? V(x->elem_type->str()) : V() },
		});
	}

	static pj::value dumpIdent(const ast::Ident* x)
	{
		return V(O {
		    { "ast", V("Ident") },
		    { "name", V(x->name.str()) },
		});
	}

	static pj::value dumpTypeExpr(const ast::TypeExpr* x)
	{
		return V(O {
		    { "ast", V("TypeExpr") },
		    { "type", V(x->ptype.str()) },
		});
	}

	static pj::value dumpCastExpr(const ast::CastExpr* x)
	{
		return V(O {
		    { "ast", V("CastExpr") },
		    { "expr", dumpExpr(x->expr.get()) },
		    { "target_type", dumpTypeExpr(x->target_type.get()) },
		});
	}

	static pj::array dump_args(const std::vector<ast::FunctionCall::Arg>& args)
	{
		pj::array arr {};
		for(auto& arg : args)
		{
			arr.push_back(V(O {
			    { "name", arg.name.has_value() ? V(*arg.name) : V() },
			    { "value", dumpExpr(arg.value.get()) },
			}));
		}
		return arr;
	}

	static pj::value dumpContextIdent(const ast::ContextIdent* x)
	{
		return V(O {
		    { "ast", V("ContextIdent") },
		    { "name", V(x->name) },
		    { "args", V(dump_args(x->arguments)) },
		});
	}

	static pj::value dumpDereferenceOp(const ast::DereferenceOp* x)
	{
		return V(O {
		    { "ast", V("DereferenceOp") },
		    { "expr", dumpExpr(x->expr.get()) },
		});
	}

	static pj::value dumpAddressOfOp(const ast::AddressOfOp* x)
	{
		return V(O {
		    { "ast", V("AddressOfOp") },
		    { "expr", dumpExpr(x->expr.get()) },
		});
	}

	static pj::value dumpArraySpreadOp(const ast::ArraySpreadOp* x)
	{
		return V(O {
		    { "ast", V("ArraySpreadOp") },
		    { "expr", dumpExpr(x->expr.get()) },
		});
	}

	static pj::value dumpSubscriptOp(const ast::SubscriptOp* x)
	{
		pj::array indices {};
		for(auto& i : x->indices)
			indices.push_back(dumpExpr(i.get()));

		return V(O {
		    { "ast", V("SubscriptOp") },
		    { "array", dumpExpr(x->array.get()) },
		    { "indices", V(std::move(indices)) },
		});
	}

	static pj::value dumpNullCoalesceOp(const ast::NullCoalesceOp* x)
	{
		return V(O {
		    { "ast", V("NullCoalesceOp") },
		    { "lhs", dumpExpr(x->lhs.get()) },
		    { "rhs", dumpExpr(x->rhs.get()) },
		});
	}

	static pj::value dumpUnaryOp(const ast::UnaryOp* x)
	{
		return V(O {
		    { "ast", V("UnaryOp") },
		    { "op", V(ast::op_to_string(x->op)) },
		    { "expr", dumpExpr(x->expr.get()) },
		});
	}

	static pj::value dumpBinaryOp(const ast::BinaryOp* x)
	{
		return V(O {
		    { "ast", V("BinaryOp") },
		    { "op", V(ast::op_to_string(x->op)) },
		    { "lhs", dumpExpr(x->lhs.get()) },
		    { "rhs", dumpExpr(x->rhs.get()) },
		});
	}

	static pj::value dumpLogicalBinOp(const ast::LogicalBinOp* x)
	{
		return V(O {
		    { "ast", V("LogicalBinOp") },
		    { "op", V(ast::op_to_string(x->op)) },
		    { "lhs", dumpExpr(x->lhs.get()) },
		    { "rhs", dumpExpr(x->rhs.get()) },
		});
	}

	static pj::value dumpComparisonOp(const ast::ComparisonOp* x)
	{
		pj::array rest {};
		for(auto& rs : x->rest)
		{
			rest.push_back(V(O {
			    { "op", V(ast::op_to_string(rs.first)) },
			    { "expr", dumpExpr(rs.second.get()) },
			}));
		}

		return V(O {
		    { "ast", V("ComparisonOp") },
		    { "first", dumpExpr(x->first.get()) },
		    { "rest", V(std::move(rest)) },
		});
	}

	static pj::value dumpDotOp(const ast::DotOp* x)
	{
		return V(O {
		    { "ast", V("DotOp") },
		    { "lhs", dumpExpr(x->lhs.get()) },
		    { "rhs", V(x->rhs) },
		});
	}

	static pj::value dumpFunctionCall(const ast::FunctionCall* x)
	{
		return V(O {
		    { "ast", V("FunctionCall") },
		    { "callee", dumpExpr(x->callee.get()) },
		    { "is_optional_ufcs", V(x->is_optional_ufcs) },
		    { "rewritten_ufcs", V(x->rewritten_ufcs) },
		    { "args", V(dump_args(x->arguments)) },
		});
	}

	static pj::value dumpImplicitUnionVariantCastExpr(const ast::ImplicitUnionVariantCastExpr* x)
	{
		return V(O {
		    { "ast", V("ImplicitUnionVariantCastExpr") },
		    { "expr", dumpExpr(x->expr.get()) },
		    { "variant_name", V(x->variant_name) },
		});
	}

	static pj::value dumpFStringExpr(const ast::FStringExpr* x)
	{
		pj::array parts {};
		for(auto& part : x->parts)
		{
			if(auto s = std::get_if<std::u32string>(&part); s)
				parts.push_back(V(unicode::stringFromU32String(*s)));
			else if(auto e = std::get_if<std::unique_ptr<ast::Expr>>(&part); e)
				parts.push_back(dumpExpr(e->get()));
			else
				util::unreachable();
		}

		return V(O {
		    { "ast", V("FStringExpr") },
		    { "parts", V(std::move(parts)) },
		});
	}

	static pj::value dumpStructLit(const ast::StructLit* x)
	{
		return V(O {
		    { "ast", V("StructLit") },
		    { "is_anonymous", V(x->is_anonymous) },
		    { "struct_name", V(x->struct_name.str()) },
		    { "fields", V(dump_args(x->field_inits)) },
		});
	}

	static pj::value dumpStructUpdateOp(const ast::StructUpdateOp* x)
	{
		pj::array updates {};
		for(auto& u : x->updates)
		{
			updates.push_back(V(O {
			    { "field", V(u.first) },
			    { "expr", dumpExpr(u.second.get()) },
			}));
		}

		return V(O {
		    { "ast", V("StructUpdateOp") },
		    { "structure", dumpExpr(x->structure.get()) },
		    { "is_assignment", V(x->is_assignment) },
		    { "is_optional", V(x->is_optional) },
		    { "updates", V(std::move(updates)) },
		});
	}

	static pj::value dumpTreeInlineExpr(const ast::TreeInlineExpr* x)
	{
		return V(O {
		    { "ast", V("TreeInlineExpr") },
		});
	}

	static pj::value dumpTreeBlockExpr(const ast::TreeBlockExpr* x)
	{
		return V(O {
		    { "ast", V("TreeBlockExpr") },
		});
	}

	pj::value dumpExpr(const ast::Expr* x)
	{
		if(x == nullptr)
			return V();

		M(MoveExpr, x);
		M(LengthExpr, x);
		M(CharLit, x);
		M(NullLit, x);
		M(BooleanLit, x);
		M(NumberLit, x);
		M(StringLit, x);
		M(ArrayLit, x);
		M(Ident, x);
		M(TypeExpr, x);
		M(CastExpr, x);
		M(ContextIdent, x);
		M(DereferenceOp, x);
		M(AddressOfOp, x);
		M(ArraySpreadOp, x);
		M(SubscriptOp, x);
		M(NullCoalesceOp, x);
		M(UnaryOp, x);
		M(BinaryOp, x);
		M(LogicalBinOp, x);
		M(ComparisonOp, x);
		M(DotOp, x);
		M(FunctionCall, x);
		M(ImplicitUnionVariantCastExpr, x);
		M(FStringExpr, x);
		M(StructLit, x);
		M(StructUpdateOp, x);
		M(TreeInlineExpr, x);
		M(TreeBlockExpr, x);

		util::unreachable();
	}
}
