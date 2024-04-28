// bs_path_style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "interp/value.h"
#include "interp/evaluator.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BE_LineCapStyle::type = nullptr;
	const Type* builtin::BE_LineJoinStyle::type = nullptr;

	frontend::PType BE_LineCapStyle::enumeratorType()
	{
		return PT::named(frontend::TYPE_INT);
	}

	std::vector<ast::EnumDefn::Enumerator> BE_LineCapStyle::enumerators()
	{
		return util::vectorOf(                                                              //
		    make_builtin_enumerator("Butt", static_cast<int>(PathStyle::CapStyle::Butt)),   //
		    make_builtin_enumerator("Round", static_cast<int>(PathStyle::CapStyle::Round)), //
		    make_builtin_enumerator("Projecting", static_cast<int>(PathStyle::CapStyle::Projecting)));
	}

	Value builtin::BE_LineCapStyle::make(PathStyle::CapStyle cap)
	{
		return Value::enumerator(type->toEnum(), Value::integer(static_cast<int>(cap)));
	}

	PathStyle::CapStyle builtin::BE_LineCapStyle::unmake(const Value& value)
	{
		return static_cast<PathStyle::CapStyle>(value.getEnumerator().getInteger());
	}


	frontend::PType BE_LineJoinStyle::enumeratorType()
	{
		return PT::named(frontend::TYPE_INT);
	}

	std::vector<ast::EnumDefn::Enumerator> BE_LineJoinStyle::enumerators()
	{
		return util::vectorOf(                                                               //
		    make_builtin_enumerator("Miter", static_cast<int>(PathStyle::JoinStyle::Miter)), //
		    make_builtin_enumerator("Round", static_cast<int>(PathStyle::JoinStyle::Round)), //
		    make_builtin_enumerator("Bevel", static_cast<int>(PathStyle::JoinStyle::Bevel)));
	}

	Value builtin::BE_LineJoinStyle::make(PathStyle::JoinStyle cap)
	{
		return Value::enumerator(type->toEnum(), Value::integer(static_cast<int>(cap)));
	}

	PathStyle::JoinStyle builtin::BE_LineJoinStyle::unmake(const Value& value)
	{
		return static_cast<PathStyle::JoinStyle>(value.getEnumerator().getInteger());
	}



	static auto get_null()
	{
		return std::make_unique<ast::NullLit>(Location::builtin());
	}

	static auto pt_float = PT::named(frontend::TYPE_FLOAT);
	static auto pt_length = PT::named(frontend::TYPE_LENGTH);

	const Type* builtin::BS_PathStyle::type = nullptr;
	std::vector<Field> builtin::BS_PathStyle::fields()
	{
		auto pt_join_style = ptype_for_builtin<BE_LineJoinStyle>();
		auto pt_cap_style = ptype_for_builtin<BE_LineCapStyle>();
		auto pt_colour = ptype_for_builtin<BS_Colour>();

		return util::vectorOf(                                                                              //
		    Field { .name = "line_width", .type = PT::optional(pt_length), .initialiser = get_null() },     //
		    Field { .name = "cap_style", .type = PT::optional(pt_cap_style), .initialiser = get_null() },   //
		    Field { .name = "join_style", .type = PT::optional(pt_join_style), .initialiser = get_null() }, //
		    Field { .name = "miter_limit", .type = PT::optional(pt_float), .initialiser = get_null() },     //
		    Field { .name = "stroke_colour", .type = PT::optional(pt_colour), .initialiser = get_null() },  //
		    Field { .name = "fill_colour", .type = PT::optional(pt_colour), .initialiser = get_null() }     //
		);
	}

	Value builtin::BS_PathStyle::make(Evaluator* ev, const PathStyle& style)
	{
		auto t_join_style = BE_LineJoinStyle::type->toEnum();
		auto t_cap_style = BE_LineCapStyle::type->toEnum();
		auto t_colour = BS_Colour::type;
		auto t_length = Type::makeLength();
		auto t_float = Type::makeFloating();

		constexpr auto& opt = Value::optional;

		return StructMaker(BS_PathStyle::type->toStruct())
		    .set("line_width", opt(t_length, Value::length(DynLength(style.line_width))))
		    .set("cap_style", opt(t_cap_style, BE_LineCapStyle::make(style.cap_style)))
		    .set("join_style", opt(t_join_style, BE_LineJoinStyle::make(style.join_style)))
		    .set("miter_limit", opt(t_float, Value::floating(style.miter_limit)))
		    .set("stroke_colour", opt(t_colour, BS_Colour::make(ev, style.stroke_colour)))
		    .set("fill_colour", opt(t_colour, BS_Colour::make(ev, style.fill_colour)))
		    .make();
	}



	PathStyle builtin::BS_PathStyle::unmake(Evaluator* ev, const Value& value)
	{
		auto cur_style = ev->currentStyle();

		// this sets the default values already.
		auto path_style = PathStyle();

		auto resolve_length_field = [&cur_style](const Value& str, zst::str_view field_name) -> std::optional<Length> {
			if(auto x = get_optional_struct_field<DynLength>(str, field_name, &Value::getLength); x.has_value())
				return x->resolve(cur_style.font(), cur_style.font_size(), cur_style.root_font_size());

			return std::nullopt;
		};


		if(auto lw = resolve_length_field(value, "line_width"); lw)
			path_style.line_width = *lw;

		if(auto cs = get_optional_enumerator_field<PathStyle::CapStyle>(value, "cap_style"); cs)
			path_style.cap_style = *cs;

		if(auto js = get_optional_enumerator_field<PathStyle::JoinStyle>(value, "join_style"); js)
			path_style.join_style = *js;

		if(auto ml = get_optional_struct_field<double>(value, "miter_limit"); ml)
			path_style.miter_limit = *ml;

		if(auto& x = value.getStructField("stroke_colour"); x.haveOptionalValue())
			path_style.stroke_colour = BS_Colour::unmake(ev, **x.getOptional());

		if(auto& x = value.getStructField("fill_colour"); x.haveOptionalValue())
			path_style.fill_colour = BS_Colour::unmake(ev, **x.getOptional());

		return path_style;
	}
}
