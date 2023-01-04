// tc_result.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "interp/type.h"

namespace sap::interp
{
	struct TCResult
	{
		const Type* type() const { return m_type; }
		bool isMutable() const { return m_is_mutable; }
		bool isLValue() const { return m_is_lvalue; }

		TCResult replacingType(const Type* type) const { return TCResult(type, m_is_mutable, m_is_lvalue); }

		static ErrorOr<TCResult> ofVoid() { return Ok(TCResult(Type::makeVoid(), false, false)); }
		static ErrorOr<TCResult> ofRValue(const Type* type) { return Ok(TCResult(type, false, false)); }
		static ErrorOr<TCResult> ofLValue(const Type* type, bool is_mutable) { return Ok(TCResult(type, is_mutable, true)); }

	private:
		explicit TCResult(const Type* type, bool is_mutable, bool is_lvalue)
		    : m_type(type)
		    , m_is_mutable(is_mutable)
		    , m_is_lvalue(is_lvalue)
		{
		}

		const Type* m_type;
		bool m_is_mutable;
		bool m_is_lvalue;
	};
}
