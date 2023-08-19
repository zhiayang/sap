// bs_outline_item.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_OutlineItem::type = nullptr;
	std::vector<Field> builtin::BS_OutlineItem::fields()
	{
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		auto pt_string = PT::named(frontend::TYPE_STRING);

		return util::vectorOf(                                                                   //
		    Field { .name = "title", .type = pt_string },                                        //
		    Field { .name = "position", .type = ptype_for_builtin<BS_AbsPosition>() },           //
		    Field { .name = "children", .type = PT::array(ptype_for_builtin<BS_OutlineItem>()) } //
		);
	}

	Value builtin::BS_OutlineItem::make(Evaluator* ev, OutlineItem pos)
	{
		return StructMaker(BS_OutlineItem::type->toStruct())
		    .set("title", Value::string(unicode::u32StringFromUtf8(pos.title)))
		    .set("position", BS_AbsPosition::make(ev, pos.position))
		    .set("children",
		        Value::array(BS_OutlineItem::type,
		            util::map(pos.children, [ev](const auto& x) { return BS_OutlineItem::make(ev, x); })))
		    .make();
	}

	OutlineItem builtin::BS_OutlineItem::unmake(Evaluator* ev, const Value& value)
	{
		auto title = value.getStructField("title").getUtf8String();
		auto position = BS_AbsPosition::unmake(ev, value.getStructField("position"));
		auto children = util::map(value.getStructField("children").getArray(), [ev](const auto& child) {
			return BS_OutlineItem::unmake(ev, child);
		});

		return OutlineItem {
			.title = std::move(title),
			.position = std::move(position),
			.children = std::move(children),
		};
	}
}
