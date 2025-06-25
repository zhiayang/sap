// bs_border_style.cpp
// Copyright (c) 2022, yuki
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

	static auto get_zero_length()
	{
		auto ret = std::make_unique<ast::LengthExpr>(Location::builtin());
		ret->length = DynLength(0);
		return ret;
	}

	const Type* builtin::BS_BorderStyle::type = nullptr;
	std::vector<Field> builtin::BS_BorderStyle::fields()
	{
		auto pt_length = PT::named(frontend::TYPE_LENGTH);
		auto pt_path_style = ptype_for_builtin<BS_PathStyle>();

		return util::vectorOf(                                                                          //
		    Field { .name = "top", .type = PT::optional(pt_path_style), .initialiser = get_null() },    //
		    Field { .name = "left", .type = PT::optional(pt_path_style), .initialiser = get_null() },   //
		    Field { .name = "right", .type = PT::optional(pt_path_style), .initialiser = get_null() },  //
		    Field { .name = "bottom", .type = PT::optional(pt_path_style), .initialiser = get_null() }, //
		    Field { .name = "top_padding", .type = pt_length, .initialiser = get_zero_length() },       //
		    Field { .name = "left_padding", .type = pt_length, .initialiser = get_zero_length() },      //
		    Field { .name = "right_padding", .type = pt_length, .initialiser = get_zero_length() },     //
		    Field { .name = "bottom_padding", .type = pt_length, .initialiser = get_zero_length() }     //
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

		maker.set("top_padding", Value::length(style.top_padding));
		maker.set("left_padding", Value::length(style.left_padding));
		maker.set("right_padding", Value::length(style.right_padding));
		maker.set("bottom_padding", Value::length(style.bottom_padding));

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

		style.top_padding = value.getStructField("top_padding").getLength();
		style.left_padding = value.getStructField("left_padding").getLength();
		style.right_padding = value.getStructField("right_padding").getLength();
		style.bottom_padding = value.getStructField("bottom_padding").getLength();
		return style;
	}
}
