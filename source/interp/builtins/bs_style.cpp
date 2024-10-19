// bs_style.cpp
// Copyright (c) 2022, yuki / zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "interp/value.h"
#include "interp/evaluator.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	static auto pt_bool = PT::named(frontend::TYPE_BOOL);
	static auto pt_float = PT::named(frontend::TYPE_FLOAT);
	static auto pt_length = PT::named(frontend::TYPE_LENGTH);

	static auto get_null()
	{
		return std::make_unique<ast::NullLit>(Location::builtin());
	}

	const Type* builtin::BS_Style::type = nullptr;
	std::vector<Field> builtin::BS_Style::fields()
	{
		auto pt_font_family = ptype_for_builtin<BS_FontFamily>();
		auto pt_alignment = ptype_for_builtin<BE_Alignment>();
		auto pt_colour = ptype_for_builtin<BS_Colour>();

		return util::vectorOf(                                                                                     //
		    Field { .name = "font_family", .type = PT::optional(pt_font_family), .initialiser = get_null() },      //
		    Field { .name = "font_size", .type = PT::optional(pt_length), .initialiser = get_null() },             //
		    Field { .name = "line_spacing", .type = PT::optional(pt_float), .initialiser = get_null() },           //
		    Field { .name = "sentence_space_stretch", .type = PT::optional(pt_float), .initialiser = get_null() }, //
		    Field { .name = "paragraph_spacing", .type = PT::optional(pt_length), .initialiser = get_null() },     //
		    Field { .name = "horz_alignment", .type = PT::optional(pt_alignment), .initialiser = get_null() },     //
		    Field { .name = "colour", .type = PT::optional(pt_colour), .initialiser = get_null() },                //
		    Field { .name = "enable_smart_quotes", .type = PT::optional(pt_bool), .initialiser = get_null() }      //
		);
	}

	Value builtin::BS_Style::make(Evaluator* ev, const Style& style)
	{
		auto maker = StructMaker(BS_Style::type->toStruct());

		if(style.have_font_family())
			maker.set("font_family", BS_FontFamily::make(ev, style.font_family()));
		if(style.have_font_size())
			maker.set("font_size", Value::length(DynLength(style.font_size())));
		if(style.have_line_spacing())
			maker.set("line_spacing", Value::floating(style.line_spacing()));
		if(style.have_sentence_space_stretch())
			maker.set("sentence_space_stretch", Value::floating(style.sentence_space_stretch()));
		if(style.have_paragraph_spacing())
			maker.set("paragraph_spacing", Value::length(DynLength(style.paragraph_spacing())));
		if(style.have_horz_alignment())
			maker.set("horz_alignment", BE_Alignment::make(style.horz_alignment()));
		if(style.have_colour())
			maker.set("colour", BS_Colour::make(ev, style.colour()));
		if(style.have_smart_quotes_enablement())
			maker.set("enable_smart_quotes", Value::boolean(style.smart_quotes_enabled()));

		return maker.make();
	}



	ErrorOr<Style> builtin::BS_Style::unmake(Evaluator* ev, const Value& value)
	{
		auto cur_style = ev->currentStyle();
		Style style {};

		auto resolve_length_field = [&cur_style](const Value& str, zst::str_view field_name) -> std::optional<Length> {
			if(auto x = get_optional_struct_field<DynLength>(str, field_name, &Value::getLength); x.has_value())
				return x->resolve(cur_style.font(), cur_style.font_size(), cur_style.root_font_size());

			return std::nullopt;
		};

		style.set_font_size(resolve_length_field(value, "font_size"));
		style.set_line_spacing(get_optional_struct_field<double>(value, "line_spacing"));
		style.set_sentence_space_stretch(get_optional_struct_field<double>(value, "sentence_space_stretch"));
		style.set_horz_alignment(get_optional_enumerator_field<Alignment>(value, "horz_alignment"));

		style.set_paragraph_spacing(resolve_length_field(value, "paragraph_spacing"));
		style.enable_smart_quotes(get_optional_struct_field<bool>(value, "enable_smart_quotes"));

		if(auto& x = value.getStructField("font_family"); x.haveOptionalValue())
			style.set_font_family(TRY(BS_FontFamily::unmake(ev, **x.getOptional())));

		if(auto& x = value.getStructField("colour"); x.haveOptionalValue())
			style.set_colour(BS_Colour::unmake(ev, **x.getOptional()));

		return OkMove(style);
	}
}
