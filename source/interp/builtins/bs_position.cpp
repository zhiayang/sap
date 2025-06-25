// bs_position.cpp
// Copyright (c) 2022, yuki
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
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_PagePosition::type = nullptr;
	std::vector<Field> builtin::BS_PagePosition::fields()
	{
		auto pt_int = PT::named(frontend::TYPE_INT);
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		return util::vectorOf(                       //
		    Field { .name = "x", .type = pt_len },   //
		    Field { .name = "y", .type = pt_len },   //
		    Field { .name = "page", .type = pt_int } //
		);
	}

	Value builtin::BS_PagePosition::make(Evaluator* ev, layout::RelativePos pos)
	{
		return StructMaker(BS_PagePosition::type->toStruct()) //
		    .set("x", Value::length(DynLength(pos.pos.x())))
		    .set("y", Value::length(DynLength(pos.pos.y())))
		    .set("page", Value::integer(checked_cast<int64_t>(pos.page_num)))
		    .make();
	}

	layout::RelativePos builtin::BS_PagePosition::unmake(Evaluator* ev, const Value& value)
	{
		auto sty = ev->currentStyle();

		auto x = get_struct_field<DynLength>(value, "x", &Value::getLength).resolve(sty);
		auto y = get_struct_field<DynLength>(value, "y", &Value::getLength).resolve(sty);

		auto p = get_struct_field<int64_t>(value, "page");

		return layout::RelativePos {
			.pos = { x, y },
			.page_num = checked_cast<size_t>(p),
		};
	}



	const Type* builtin::BS_AbsPosition::type = nullptr;
	std::vector<Field> builtin::BS_AbsPosition::fields()
	{
		auto pt_int = PT::named(frontend::TYPE_INT);
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		return util::vectorOf(                       //
		    Field { .name = "x", .type = pt_len },   //
		    Field { .name = "y", .type = pt_len },   //
		    Field { .name = "page", .type = pt_int } //
		);
	}

	Value builtin::BS_AbsPosition::make(Evaluator* ev, layout::AbsolutePagePos pos)
	{
		return StructMaker(BS_AbsPosition::type->toStruct()) //
		    .set("x", Value::length(DynLength(pos.pos.x())))
		    .set("y", Value::length(DynLength(pos.pos.y())))
		    .set("page", Value::integer(checked_cast<int64_t>(pos.page_num)))
		    .make();
	}

	layout::AbsolutePagePos builtin::BS_AbsPosition::unmake(Evaluator* ev, const Value& value)
	{
		auto sty = ev->currentStyle();

		auto x = get_struct_field<DynLength>(value, "x", &Value::getLength).resolve(sty);
		auto y = get_struct_field<DynLength>(value, "y", &Value::getLength).resolve(sty);

		auto p = get_struct_field<int64_t>(value, "page");

		return layout::AbsolutePagePos {
			.pos = { x, y },
			.page_num = checked_cast<size_t>(p),
		};
	}
}
