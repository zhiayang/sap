// bs_outline_item.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	const Type* builtin::BS_OutlineItem::type = nullptr;
	std::vector<Field> builtin::BS_OutlineItem::fields()
	{
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		return util::vectorOf(                     //
			Field { .name = "x", .type = pt_len }, //
			Field { .name = "y", .type = pt_len }  //
		);
	}

	Value builtin::BS_OutlineItem::make(Evaluator* ev, OutlineItem pos)
	{
		return StructMaker(BS_OutlineItem::type->toStruct()).make();
	}

	ErrorOr<OutlineItem> builtin::BS_OutlineItem::unmake(Evaluator* ev, const Value& value)
	{
		auto x = get_struct_field<DynLength>(value, "x", &Value::getLength);
		auto y = get_struct_field<DynLength>(value, "y", &Value::getLength);
		return ErrMsg(ev, "stop");
		// return Ok(DynLength2d { x, y });
	}
}
