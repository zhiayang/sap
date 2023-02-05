// bs_style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "tree/base.h"
#include "interp/value.h"
#include "interp/evaluator.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	static auto pt_float = PT::named(frontend::TYPE_FLOAT);
	static auto pt_length = PT::named(frontend::TYPE_LENGTH);

	static auto get_null()
	{
		return std::make_unique<NullLit>(Location::builtin());
	}

	template <typename T>
	static std::optional<T> get_optional_enumerator_field(const Value& val, zst::str_view field)
	{
		auto& fields = val.getStructFields();
		auto idx = val.type()->toStruct()->getFieldIndex(field);

		auto& f = fields[idx];
		if(not f.haveOptionalValue())
			return std::nullopt;

		return static_cast<T>((*f.getOptional())->getEnumerator().getInteger());
	};



	const Type* builtin::BS_Style::type = nullptr;
	std::vector<Field> builtin::BS_Style::fields()
	{
		auto pt_font_family = PT::named(QualifiedId {
		    .top_level = true,
		    .parents = { "builtin" },
		    .name = BS_FontFamily::name,
		});

		auto pt_alignment = PT::named(QualifiedId {
		    .top_level = true,
		    .parents = { "builtin" },
		    .name = BE_Alignment::name,
		});

		return util::vectorOf(                                                                                 //
		    Field { .name = "font_family", .type = PT::optional(pt_font_family), .initialiser = get_null() },  //
		    Field { .name = "font_size", .type = PT::optional(pt_length), .initialiser = get_null() },         //
		    Field { .name = "line_spacing", .type = PT::optional(pt_float), .initialiser = get_null() },       //
		    Field { .name = "paragraph_spacing", .type = PT::optional(pt_length), .initialiser = get_null() }, //
		    Field { .name = "alignment", .type = PT::optional(pt_alignment), .initialiser = get_null() }       //
		);
	}

	Value builtin::BS_Style::make(Evaluator* ev, const Style* style)
	{
		return StructMaker(BS_Style::type->toStruct()) //
		    .set("font_family", BS_FontFamily::make(ev, style->font_family()))
		    .set("font_size", Value::length(DynLength(style->font_size())))
		    .set("line_spacing", Value::floating(style->line_spacing()))
		    .set("paragraph_spacing", Value::length(DynLength(style->paragraph_spacing())))
		    .set("alignment",
		        Value::enumerator(BE_Alignment::type->toEnum(), Value::integer(static_cast<int64_t>(style->alignment()))))
		    .make();
	}



	ErrorOr<const Style*> builtin::BS_Style::unmake(Evaluator* ev, const Value& value)
	{
		auto cur_style = ev->currentStyle();
		auto style = util::make<Style>();

		auto resolve_length_field = [&cur_style](const Value& str, zst::str_view field_name) -> std::optional<Length> {
			if(auto x = get_optional_struct_field<DynLength>(str, field_name, &Value::getLength); x.has_value())
				return x->resolve(cur_style->font(), cur_style->font_size(), cur_style->root_font_size());

			return std::nullopt;
		};

		style->set_font_size(resolve_length_field(value, "font_size"));
		style->set_line_spacing(get_optional_struct_field<double>(value, "line_spacing"));
		style->set_alignment(get_optional_enumerator_field<Alignment>(value, "alignment"));

		style->set_paragraph_spacing(resolve_length_field(value, "paragraph_spacing"));

		if(auto& x = value.getStructField("font_family"); x.haveOptionalValue())
			style->set_font_family(TRY(BS_FontFamily::unmake(ev, **x.getOptional())));

		return Ok(style);
	}
}
