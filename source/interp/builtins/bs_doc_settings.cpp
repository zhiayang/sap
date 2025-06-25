// bs_doc_settings.cpp
// Copyright (c) 2022, yuki
// SPDX-License-Identifier: Apache-2.0

#include "sap/document_settings.h"

#include "interp/value.h"
#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	static std::unique_ptr<ast::NullLit> make_null()
	{
		return std::make_unique<ast::NullLit>(Location::builtin());
	}

	const Type* builtin::BS_DocumentSettings::type = nullptr;
	std::vector<Field> builtin::BS_DocumentSettings::fields()
	{
		auto pt_len = PT::named(frontend::TYPE_LENGTH);
		auto pt_float = PT::named(frontend::TYPE_FLOAT);

		auto pt_font_family = ptype_for_builtin<BS_FontFamily>();
		auto pt_size2d = ptype_for_builtin<BS_Size2d>();
		auto pt_margins = ptype_for_builtin<BS_DocumentMargins>();
		auto pt_style = ptype_for_builtin<BS_Style>();

		return util::vectorOf(                                                                                       //
		    Field { .name = "serif_font_family", .type = PT::optional(pt_font_family), .initialiser = make_null() }, //
		    Field { .name = "sans_font_family", .type = PT::optional(pt_font_family), .initialiser = make_null() },  //
		    Field { .name = "mono_font_family", .type = PT::optional(pt_font_family), .initialiser = make_null() },  //
		    Field { .name = "paper_size", .type = PT::optional(pt_size2d), .initialiser = make_null() },             //
		    Field { .name = "margins", .type = PT::optional(pt_margins), .initialiser = make_null() },               //
		    Field { .name = "default_style", .type = PT::optional(pt_style), .initialiser = make_null() }            //
		);
	}

	Value builtin::BS_DocumentSettings::make(Evaluator* ev, DocumentSettings settings)
	{
		return StructMaker(BS_DocumentSettings::type->toStruct())
		    .setOptional("margins", settings.margins, ev, &BS_DocumentMargins::make)
		    .setOptional("serif_font_family", settings.serif_font_family, ev, &BS_FontFamily::make)
		    .setOptional("sans_font_family", settings.sans_font_family, ev, &BS_FontFamily::make)
		    .setOptional("mono_font_family", settings.mono_font_family, ev, &BS_FontFamily::make)
		    .setOptional("paper_size", settings.paper_size, ev, &BS_Size2d::make)
		    .setOptional("default_style", settings.default_style, ev, &BS_Style::make)
		    .make();
	}

	ErrorOr<DocumentSettings> builtin::BS_DocumentSettings::unmake(Evaluator* ev, const Value& value)
	{
		DocumentSettings settings {};

		if(auto& x = value.getStructField("margins"); x.haveOptionalValue())
			settings.margins = BS_DocumentMargins::unmake(ev, **x.getOptional());

		if(auto& x = value.getStructField("serif_font_family"); x.haveOptionalValue())
			settings.serif_font_family = TRY(BS_FontFamily::unmake(ev, **x.getOptional()));

		if(auto& x = value.getStructField("sans_font_family"); x.haveOptionalValue())
			settings.sans_font_family = TRY(BS_FontFamily::unmake(ev, **x.getOptional()));

		if(auto& x = value.getStructField("mono_font_family"); x.haveOptionalValue())
			settings.mono_font_family = TRY(BS_FontFamily::unmake(ev, **x.getOptional()));

		if(auto& x = value.getStructField("paper_size"); x.haveOptionalValue())
			settings.paper_size = BS_Size2d::unmake(ev, **x.getOptional());

		if(auto& x = value.getStructField("default_style"); x.haveOptionalValue())
			settings.default_style = TRY(BS_Style::unmake(ev, **x.getOptional()));

		return OkMove(settings);
	}




	const Type* builtin::BS_DocumentMargins::type = nullptr;
	std::vector<Field> builtin::BS_DocumentMargins::fields()
	{
		auto t = PT::optional(PT::named(frontend::TYPE_LENGTH));
		return util::vectorOf(                                                 //
		    Field { .name = "top", .type = t, .initialiser = make_null() },    //
		    Field { .name = "bottom", .type = t, .initialiser = make_null() }, //
		    Field { .name = "left", .type = t, .initialiser = make_null() },   //
		    Field { .name = "right", .type = t, .initialiser = make_null() }   //
		);
	}

	Value builtin::BS_DocumentMargins::make(Evaluator* ev, DocumentSettings::Margins margins)
	{
		return StructMaker(BS_DocumentMargins::type->toStruct()) //
		    .setOptional("top", margins.top, &Value::length)
		    .setOptional("bottom", margins.bottom, &Value::length)
		    .setOptional("left", margins.left, &Value::length)
		    .setOptional("right", margins.right, &Value::length)
		    .make();
	}

	DocumentSettings::Margins builtin::BS_DocumentMargins::unmake(Evaluator* ev, const Value& value)
	{
		DocumentSettings::Margins margins {};

		margins.top = get_optional_struct_field<DynLength>(value, "top", &Value::getLength);
		margins.bottom = get_optional_struct_field<DynLength>(value, "bottom", &Value::getLength);
		margins.left = get_optional_struct_field<DynLength>(value, "left", &Value::getLength);
		margins.right = get_optional_struct_field<DynLength>(value, "right", &Value::getLength);

		return margins;
	}
}
