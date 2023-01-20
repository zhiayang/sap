// bs_position.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

#include "pdf/font.h"

#include "tree/base.h"

#include "interp/value.h"
#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	const Type* builtin::BS_Position::type = nullptr;
	std::vector<Field> builtin::BS_Position::fields()
	{
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		return util::vectorOf(                     //
		    Field { .name = "x", .type = pt_len }, //
		    Field { .name = "y", .type = pt_len });
	}

	ErrorOr<Value> builtin::BS_Position::make(Evaluator* ev, DynLength2d pos)
	{
		return StructMaker(BS_Position::type->toStruct()) //
		    .set("x", Value::length(pos.x))
		    .set("y", Value::length(pos.y))
		    .make();
	}

	ErrorOr<DynLength2d> builtin::BS_Position::unmake(Evaluator* ev, const Value& value)
	{
		auto x = get_struct_field<DynLength>(value, "x", &Value::getLength);
		auto y = get_struct_field<DynLength>(value, "y", &Value::getLength);

		return Ok(DynLength2d { .x = x, .y = y });
	}
}
