// bs_link_annotation.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	const Type* builtin::BS_LinkAnnotation::type = nullptr;
	std::vector<Field> builtin::BS_LinkAnnotation::fields()
	{
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		auto pt_string = PT::named(frontend::TYPE_STRING);

		return util::vectorOf(                                                           //
			Field { .name = "position", .type = ptype_for_builtin<BS_AbsPosition>() },   //
			Field { .name = "size", .type = ptype_for_builtin<BS_Size2d>() },            //
			Field { .name = "destination", .type = ptype_for_builtin<BS_AbsPosition>() } //
		);
	}

	Value builtin::BS_LinkAnnotation::make(Evaluator* ev, LinkAnnotation annot)
	{
		return StructMaker(BS_LinkAnnotation::type->toStruct())
		    .set("position", BS_AbsPosition::make(ev, annot.position))
		    .set("size", BS_Size2d::make(ev, annot.size))
		    .set("destination", BS_AbsPosition::make(ev, annot.destination))
		    .make();
	}

	LinkAnnotation builtin::BS_LinkAnnotation::unmake(Evaluator* ev, const Value& value)
	{
		auto position = BS_AbsPosition::unmake(ev, value.getStructField("position"));
		auto dest = BS_AbsPosition::unmake(ev, value.getStructField("destination"));
		auto size = BS_Size2d::unmake(ev, value.getStructField("size"));

		return LinkAnnotation {
			.position = std::move(position),
			.size = size.resolve(ev->currentStyle()),
			.destination = std::move(dest),
		};
	}
}
