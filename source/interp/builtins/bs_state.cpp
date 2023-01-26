// bs_state.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	const Type* builtin::BS_State::type = nullptr;
	std::vector<Field> builtin::BS_State::fields()
	{
		auto pt_int = PT::named(frontend::TYPE_INT);
		return util::vectorOf(                              //
		    Field { .name = "layout_pass", .type = pt_int } //
		);
	}

	Value builtin::BS_State::make(Evaluator* ev, GlobalState state)
	{
		return StructMaker(BS_State::type->toStruct()) //
		    .set("layout_pass", Value::integer(checked_cast<int64_t>(state.layout_pass)))
		    .make();
	}

	ErrorOr<GlobalState> builtin::BS_State::unmake(Evaluator* ev, const Value& value)
	{
		auto p = get_struct_field<int64_t>(value, "layout_pass");

		return Ok(GlobalState {
		    .layout_pass = checked_cast<size_t>(p),
		});
	}
}
