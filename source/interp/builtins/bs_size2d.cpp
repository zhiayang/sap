// bs_size2d.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_Size2d::type = nullptr;
	std::vector<Field> builtin::BS_Size2d::fields()
	{
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		return util::vectorOf(                     //
		    Field { .name = "x", .type = pt_len }, //
		    Field { .name = "y", .type = pt_len }  //
		);
	}

	Value BS_Size2d::make(Evaluator* ev, Size2d pos)
	{
		return StructMaker(BS_Size2d::type->toStruct()) //
		    .set("x", Value::length(DynLength(pos.x())))
		    .set("y", Value::length(DynLength(pos.y())))
		    .make();
	}

	Value builtin::BS_Size2d::make(Evaluator* ev, DynLength2d pos)
	{
		return StructMaker(BS_Size2d::type->toStruct()) //
		    .set("x", Value::length(pos.x))
		    .set("y", Value::length(pos.y))
		    .make();
	}

	DynLength2d builtin::BS_Size2d::unmake(Evaluator* ev, const Value& value)
	{
		auto x = get_struct_field<DynLength>(value, "x", &Value::getLength);
		auto y = get_struct_field<DynLength>(value, "y", &Value::getLength);
		return DynLength2d { x, y };
	}
}
