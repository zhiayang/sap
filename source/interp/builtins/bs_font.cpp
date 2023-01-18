// bs_font.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

#include "pdf/font.h"

#include "tree/base.h"

#include "interp/value.h"
#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = StructDefn::Field;

	static auto pt_int = PT::named(frontend::TYPE_INT);
	static auto pt_str = PT::named(frontend::TYPE_STRING);
	static auto pt_bool = PT::named(frontend::TYPE_BOOL);

	const Type* builtin::BS_Font::type = nullptr;
	std::vector<Field> builtin::BS_Font::fields()
	{
		return util::vectorOf(                        //
		    Field { .name = "name", .type = pt_str }, //
		    Field { .name = "font_id", .type = pt_int });
	}

	ErrorOr<Value> builtin::BS_Font::make(Evaluator* ev, int64_t font_id)
	{
		auto pdf_font = TRY(ev->interpreter()->getLoadedFontById(font_id));
		return StructMaker(BS_Font::type->toStruct()) //
		    .set("name", Value::string(unicode::u32StringFromUtf8(pdf_font->source().name())))
		    .set("font_id", Value::integer(font_id))
		    .make();
	}

	ErrorOr<int64_t> builtin::BS_Font::unmake(Evaluator* ev, const Value& value)
	{
		auto font_id = *get_struct_field<int64_t>(value, "font_id");
		return Ok(font_id);
	}
}
