// bs_font.cpp
// Copyright (c) 2022, yuki / zhiayang
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
	using Field = ast::StructDefn::Field;

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

	Value builtin::BS_Font::make(Evaluator* ev, pdf::PdfFont* font)
	{
		return StructMaker(BS_Font::type->toStruct()) //
		    .set("name", Value::string(unicode::u32StringFromUtf8(font->source().name())))
		    .set("font_id", Value::integer(font->fontId()))
		    .make();
	}

	ErrorOr<pdf::PdfFont*> builtin::BS_Font::unmake(Evaluator* ev, const Value& value)
	{
		auto font_id = get_struct_field<int64_t>(value, "font_id");
		return ev->interpreter()->getLoadedFontById(font_id);
	}





	const Type* builtin::BS_FontFamily::type = nullptr;
	std::vector<Field> builtin::BS_FontFamily::fields()
	{
		auto pt_font = PT::named(QualifiedId {
		    .top_level = true,
		    .parents = { "builtin" },
		    .name = BS_Font::name,
		});

		return util::vectorOf(                            //
		    Field { .name = "regular", .type = pt_font }, //
		    Field { .name = "italic", .type = pt_font },  //
		    Field { .name = "bold", .type = pt_font },    //
		    Field { .name = "bold_italic", .type = pt_font });
	}

	Value builtin::BS_FontFamily::make(Evaluator* ev, FontFamily font)
	{
		return StructMaker(BS_FontFamily::type->toStruct()) //
		    .set("regular", BS_Font::make(ev, font.regular()))
		    .set("italic", BS_Font::make(ev, font.italic()))
		    .set("bold", BS_Font::make(ev, font.bold()))
		    .set("bold_italic", BS_Font::make(ev, font.boldItalic()))
		    .make();
	}

	ErrorOr<FontFamily> builtin::BS_FontFamily::unmake(Evaluator* ev, const Value& value)
	{
		auto& rg = value.getStructField("regular");
		auto& it = value.getStructField("italic");
		auto& bd = value.getStructField("bold");
		auto& bi = value.getStructField("bold_italic");

		return Ok(FontFamily(             //
		    TRY(BS_Font::unmake(ev, rg)), //
		    TRY(BS_Font::unmake(ev, it)), //
		    TRY(BS_Font::unmake(ev, bd)), //
		    TRY(BS_Font::unmake(ev, bi))));
	}
}
