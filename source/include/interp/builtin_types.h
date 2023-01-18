// builtin_types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"

#include "interp/ast.h"
#include "interp/parser_type.h"

namespace sap
{
	struct Style;
	enum class Alignment;

	namespace interp
	{
		struct Value;
	}
}

namespace sap::interp::builtin
{
	// BS: builtin struct
	// BE: builtin enum

	struct BS_Style
	{
		static constexpr auto name = "Style";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();

		static Value make(const Style* style);
		static const Style* unmake(Evaluator* ev, const Value& value);
	};

	struct BE_Alignment
	{
		static constexpr auto name = "Alignment";

		static const Type* type;

		static frontend::PType enumeratorType();
		static std::vector<EnumDefn::EnumeratorDefn> enumerators();

		static Value make(Alignment alignment);
		static Alignment unmake(const Value& value);
	};

	// o no builder pattern
	struct StructMaker
	{
		explicit StructMaker(const StructType* type);

		Value make();
		StructMaker& set(zst::str_view field, Value value);

	private:
		const StructType* m_type;
		std::vector<Value> m_fields;
	};
};
