// tc_result.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/cst.h"
#include "interp/type.h"

namespace sap::interp
{
	namespace ast
	{
		struct Expr;
	}

	struct TCResult2
	{
		const Type* type() const { return m_type; }
		cst::Stmt* stmt() { return m_stmt.get(); }

		cst::Expr* expr()
		{
			assert(m_stmt->isExpr());
			assert(not m_type->isVoid());
			return static_cast<cst::Expr*>(m_stmt.get());
		}

		std::unique_ptr<cst::Stmt> take_stmt() && { return std::unique_ptr<cst::Stmt>(m_stmt.release()); }

		std::unique_ptr<cst::Expr> take_expr() &&
		{
			assert(m_stmt->isExpr());
			assert(not m_type->isVoid());
			return std::unique_ptr<cst::Expr>(static_cast<cst::Expr*>(m_stmt.release()));
		}

		template <typename T>
		std::unique_ptr<T> take() &&
		{
			return std::unique_ptr<T>(static_cast<T*>(m_stmt.release()));
		}


		bool isMutable() const { return m_is_mutable; }
		bool isLValue() const { return m_is_lvalue; }

		TCResult2 replacingType(const Type* type, std::unique_ptr<cst::Stmt> stmt) const
		{
			return TCResult2(type, std::move(stmt), m_is_mutable, m_is_lvalue);
		}

		static ErrorOr<TCResult2> ofVoid(std::unique_ptr<cst::Stmt> stmt)
		{
			return Ok(TCResult2(Type::makeVoid(), std::move(stmt), false, false));
		}

		static ErrorOr<TCResult2> ofRValue(std::unique_ptr<cst::Expr> expr)
		{
			auto t = expr->type();
			return Ok(TCResult2(t, std::move(expr), false, false));
		}

		static ErrorOr<TCResult2> ofLValue(std::unique_ptr<cst::Expr> expr, bool is_mutable)
		{
			auto t = expr->type();
			return Ok(TCResult2(t, std::move(expr), is_mutable, true));
		}

		template <typename T, typename... Args>
		static ErrorOr<TCResult2> ofVoid(Args&&... args)
		{
			return Ok(TCResult2(Type::makeVoid(), std::make_unique<T>(static_cast<Args&&>(args)...), false, false));
		}

		template <typename T, typename... Args>
		static ErrorOr<TCResult2> ofRValue(Args&&... args)
		{
			auto expr = std::make_unique<T>(static_cast<Args&&>(args)...);
			auto t = expr->type();
			return Ok(TCResult2(t, std::move(expr), false, false));
		}

		template <typename T, typename... Args>
		static ErrorOr<TCResult2> ofMutableLValue(Args&&... args)
		{
			auto expr = std::make_unique<T>(static_cast<Args&&>(args)...);
			auto t = expr->type();
			return Ok(TCResult2(t, std::move(expr), /* is_mutable: */ true, true));
		}


		template <typename T, typename... Args>
		static ErrorOr<TCResult2> ofImmutableLValue(Args&&... args)
		{
			auto expr = std::make_unique<T>(static_cast<Args&&>(args)...);
			auto t = expr->type();
			return Ok(TCResult2(t, std::move(expr), /* is_mutable: */ false, true));
		}


	private:
		explicit TCResult2(const Type* type, std::unique_ptr<cst::Stmt> stmt, bool is_mutable, bool is_lvalue)
		    : m_type(type), m_stmt(std::move(stmt)), m_is_mutable(is_mutable), m_is_lvalue(is_lvalue)
		{
		}

		const Type* m_type;
		std::unique_ptr<cst::Stmt> m_stmt;

		bool m_is_mutable;
		bool m_is_lvalue;
	};
}
