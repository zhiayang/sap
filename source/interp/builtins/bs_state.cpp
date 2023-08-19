// bs_state.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/frontend.h"

#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_State::type = nullptr;
	std::vector<Field> builtin::BS_State::fields()
	{
		auto pt_int = PT::named(frontend::TYPE_INT);
		auto pt_size2d = ptype_for_builtin<BS_Size2d>();
		auto pt_font_family = ptype_for_builtin<BS_FontFamily>();

		return util::vectorOf(                                             //
		    Field { .name = "layout_pass", .type = pt_int },               //
		    Field { .name = "serif_font_family", .type = pt_font_family }, //
		    Field { .name = "sans_font_family", .type = pt_font_family },  //
		    Field { .name = "mono_font_family", .type = pt_font_family }   //
		);
	}

	Value builtin::BS_State::make(Evaluator* ev, const GlobalState& state)
	{
		return StructMaker(BS_State::type->toStruct()) //
		    .set("layout_pass", Value::integer(checked_cast<int64_t>(state.layout_pass)))
		    .set("serif_font_family", BS_FontFamily::make(ev, *state.document_settings->serif_font_family))
		    .set("sans_font_family", BS_FontFamily::make(ev, *state.document_settings->sans_font_family))
		    .set("mono_font_family", BS_FontFamily::make(ev, *state.document_settings->mono_font_family))
		    .make();
	}
}
