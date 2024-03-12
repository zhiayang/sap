// bs_border_style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/path.h"

#include "interp/value.h"
#include "interp/evaluator.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	static auto get_null()
	{
		return std::make_unique<ast::NullLit>(Location::builtin());
	}

	const Type* builtin::BS_BorderStyle::type = nullptr;
	std::vector<Field> builtin::BS_BorderStyle::fields()
	{
		auto pt_path_style = ptype_for_builtin<BS_PathStyle>();

		return util::vectorOf(                                                                         //
		    Field { .name = "top", .type = PT::optional(pt_path_style), .initialiser = get_null() },   //
		    Field { .name = "left", .type = PT::optional(pt_path_style), .initialiser = get_null() },  //
		    Field { .name = "right", .type = PT::optional(pt_path_style), .initialiser = get_null() }, //
		    Field { .name = "bottom", .type = PT::optional(pt_path_style), .initialiser = get_null() } //
		);
	}

	Value builtin::BS_BorderStyle::make(Evaluator* ev, const BorderStyle& style)
	{
		auto maker = StructMaker(BS_BorderStyle::type->toStruct());
		if(style.top)
			maker.set("top", BS_PathStyle::make(ev, *style.top));
		if(style.left)
			maker.set("left", BS_PathStyle::make(ev, *style.left));
		if(style.right)
			maker.set("right", BS_PathStyle::make(ev, *style.right));
		if(style.bottom)
			maker.set("bottom", BS_PathStyle::make(ev, *style.bottom));

		return maker.make();
	}



	BorderStyle builtin::BS_BorderStyle::unmake(Evaluator* ev, const Value& value)
	{
		BorderStyle style {};

		auto& s_top = value.getStructField("top");
		auto& s_left = value.getStructField("left");
		auto& s_right = value.getStructField("right");
		auto& s_bottom = value.getStructField("bottom");

		if(s_top.haveOptionalValue())
			style.top = BS_PathStyle::unmake(ev, **s_top.getOptional());

		if(s_left.haveOptionalValue())
			style.left = BS_PathStyle::unmake(ev, **s_left.getOptional());

		if(s_right.haveOptionalValue())
			style.right = BS_PathStyle::unmake(ev, **s_right.getOptional());

		if(s_bottom.haveOptionalValue())
			style.bottom = BS_PathStyle::unmake(ev, **s_bottom.getOptional());

		return style;
	}
}
