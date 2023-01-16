// bs_style.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "tree/base.h"
#include "interp/value.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	static auto pt_float = PT::named(frontend::TYPE_FLOAT);
	static auto pt_alignment = PT::named(BE_Alignment::name);

	static auto get_null()
	{
		return std::make_unique<NullLit>(Location::builtin());
	}

	template <typename T>
	static std::optional<T> get_field(const Value& str, zst::str_view field)
	{
		auto& fields = str.getStructFields();
		auto idx = str.type()->toStruct()->getFieldIndex(field);

		auto& f = fields[idx];
		if(not f.haveOptionalValue())
			return std::nullopt;

		return (*f.getOptional())->get<T>();
	}

	template <typename T>
	static std::optional<T> get_enumerator_field(const Value& val, zst::str_view field)
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
		return util::vectorOf(                                                                        //
		    Field { .name = "font_size", .type = PT::optional(pt_float), .initialiser = get_null() }, //
		    Field { .name = "line_spacing", .type = PT::optional(pt_float), .initialiser = get_null() },
		    Field { .name = "alignment", .type = PT::optional(pt_alignment), .initialiser = get_null() });
	}

	Value builtin::BS_Style::make(const Style* style)
	{
		return StructMaker(BS_Style::type->toStruct()) //
		    .set("font_size", Value::floating(style->font_size().value()))
		    .set("line_spacing", Value::floating(style->line_spacing()))
		    .set("alignment",
		        Value::enumerator(BE_Alignment::type->toEnum(), Value::integer(static_cast<int64_t>(style->alignment()))))
		    .make();
	}



	const Style* builtin::BS_Style::unmake(const Value& value)
	{
		auto style = util::make<Style>();

		auto get_scalar = [&value](zst::str_view field) -> std::optional<sap::Length> {
			auto f = get_field<double>(value, field);
			if(f.has_value())
				return sap::Length(*f);

			return std::nullopt;
		};
		style->set_font_size(get_scalar("font_size"));
		style->set_line_spacing(get_field<double>(value, "line_spacing"));
		style->set_alignment(get_enumerator_field<Alignment>(value, "alignment"));

		return style;
	}
}
