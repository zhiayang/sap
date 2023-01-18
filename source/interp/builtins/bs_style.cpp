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
	static auto pt_alignment = PT::named(BE_Alignment::name);

	static auto get_null()
	{
		return std::make_unique<NullLit>(Location::builtin());
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
		    Field { .name = "paragraph_spacing", .type = PT::optional(pt_length), .initialiser = get_null() },
		    Field { .name = "alignment", .type = PT::optional(pt_alignment), .initialiser = get_null() });
	}

	ErrorOr<Value> builtin::BS_Style::make(Evaluator* ev, const Style* style)
	{
		return StructMaker(BS_Style::type->toStruct()) //
		    .set("font_size", Value::floating(style->font_size().value()))
		    .set("line_spacing", Value::floating(style->line_spacing()))
		    .set("paragraph_spacing", Value::length(DynLength(style->paragraph_spacing())))
		    .set("alignment",
		        Value::enumerator(BE_Alignment::type->toEnum(), Value::integer(static_cast<int64_t>(style->alignment()))))
		    .make();
	}



	ErrorOr<const Style*> builtin::BS_Style::unmake(Evaluator* ev, const Value& value)
	{
		auto cur_style = ev->currentStyle().unwrap();

		auto style = util::make<Style>();

		auto get_scalar = [&value](zst::str_view field) -> std::optional<sap::Length> {
			auto f = get_struct_field<double>(value, field);
			if(f.has_value())
				return sap::Length(*f);

			return std::nullopt;
		};

		style->set_font_size(get_scalar("font_size"));
		style->set_line_spacing(get_struct_field<double>(value, "line_spacing"));
		style->set_alignment(get_enumerator_field<Alignment>(value, "alignment"));

		if(auto x = get_struct_field<DynLength>(value, "paragraph_spacing", &Value::getLength); x.has_value())
			style->set_paragraph_spacing(x->resolve(cur_style->font(), cur_style->font_size()));

		return Ok(style);
	}
}
