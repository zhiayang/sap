// bs_doc_settings.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/document_settings.h"

#include "interp/value.h"
#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	static std::unique_ptr<NullLit> make_null()
	{
		return std::make_unique<NullLit>(Location::builtin());
	}

	const Type* builtin::BS_DocumentSettings::type = nullptr;
	std::vector<Field> builtin::BS_DocumentSettings::fields()
	{
		using QID = QualifiedId;
		auto pt_len = PT::named(frontend::TYPE_LENGTH);

		auto pt_font_family = PT::named(QID { .top_level = true, .parents = { "builtin" }, .name = BS_FontFamily::name });
		auto pt_size2d = PT::named(QID { .top_level = true, .parents = { "builtin" }, .name = BS_Size2d::name });
		auto pt_margins = PT::named(QID { .top_level = true, .parents = { "builtin" }, .name = BS_DocumentMargins::name });

		return util::vectorOf(                                                                                 //
		    Field { .name = "font_family", .type = PT::optional(pt_font_family), .initialiser = make_null() }, //
		    Field { .name = "font_size", .type = PT::optional(pt_len), .initialiser = make_null() },           //
		    Field { .name = "paper_size", .type = PT::optional(pt_size2d), .initialiser = make_null() },       //
		    Field { .name = "margins", .type = PT::optional(pt_margins), .initialiser = make_null() }          //
		);
	}

	Value builtin::BS_DocumentSettings::make(Evaluator* ev, DocumentSettings settings)
	{
		return StructMaker(BS_DocumentSettings::type->toStruct())
		    .setOptional("margins", settings.margins, ev, &BS_DocumentMargins::make)
		    .setOptional("font_family", settings.font_family, ev, &BS_FontFamily::make)
		    .setOptional("paper_size", settings.paper_size, ev, &BS_Size2d::make)
		    .setOptional("font_size", settings.font_size, &Value::length)
		    .make();
	}

	ErrorOr<DocumentSettings> builtin::BS_DocumentSettings::unmake(Evaluator* ev, const Value& value)
	{
		DocumentSettings settings {};

		if(auto& x = value.getStructField("margins"); x.haveOptionalValue())
			settings.margins = TRY(BS_DocumentMargins::unmake(ev, **x.getOptional()));

		if(auto& x = value.getStructField("font_family"); x.haveOptionalValue())
			settings.font_family = TRY(BS_FontFamily::unmake(ev, **x.getOptional()));

		if(auto& x = value.getStructField("paper_size"); x.haveOptionalValue())
			settings.paper_size = TRY(BS_Size2d::unmake(ev, **x.getOptional()));

		settings.font_size = get_optional_struct_field<DynLength>(value, "font_size", &Value::getLength);
		return Ok(std::move(settings));
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

	ErrorOr<DocumentSettings::Margins> builtin::BS_DocumentMargins::unmake(Evaluator* ev, const Value& value)
	{
		DocumentSettings::Margins margins {};

		margins.top = get_optional_struct_field<DynLength>(value, "top", &Value::getLength);
		margins.bottom = get_optional_struct_field<DynLength>(value, "bottom", &Value::getLength);
		margins.left = get_optional_struct_field<DynLength>(value, "left", &Value::getLength);
		margins.right = get_optional_struct_field<DynLength>(value, "right", &Value::getLength);

		return Ok(std::move(margins));
	}
}
