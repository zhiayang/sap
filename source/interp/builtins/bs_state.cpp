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
		auto pt_size2d = PT::named(QualifiedId {
			.top_level = true,
			.parents = { "builtin" },
			.name = BS_Size2d::name,
		});

		return util::vectorOf(                               //
			Field { .name = "layout_pass", .type = pt_int }, //
			Field { .name = "page_count", .type = pt_int },  //
			Field { .name = "page_size", .type = pt_size2d } //
		);
	}

	Value builtin::BS_State::make(Evaluator* ev, GlobalState state)
	{
		return StructMaker(BS_State::type->toStruct()) //
		    .set("layout_pass", Value::integer(checked_cast<int64_t>(state.layout_pass)))
		    .set("page_count", Value::integer(checked_cast<int64_t>(state.page_count)))
		    .set("page_size", BS_Size2d::make(ev, state.page_size))
		    .make();
	}

	GlobalState builtin::BS_State::unmake(Evaluator* ev, const Value& value)
	{
		auto layout_pass = get_struct_field<int64_t>(value, "layout_pass");
		auto page_count = get_struct_field<int64_t>(value, "page_count");
		auto& page_size = value.getStructField("page_size");

		return GlobalState {
			.layout_pass = checked_cast<size_t>(layout_pass),
			.page_count = checked_cast<size_t>(page_count),
			.page_size = BS_Size2d::unmake(ev, page_size),
		};
	}
}
