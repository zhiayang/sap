// bs_glyph_adjustment.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/value.h"
#include "interp/interp.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_GlyphSpacingAdjustment::type = nullptr;
	std::vector<Field> builtin::BS_GlyphSpacingAdjustment::fields()
	{
		auto pt_char = PT::named(frontend::TYPE_CHAR);
		auto pt_float = PT::named(frontend::TYPE_FLOAT);

		return util::vectorOf(                                                //
		    Field { .name = "match", .type = PT::array(PT::array(pt_char)) }, //
		    Field { .name = "adjust", .type = PT::array(pt_float) }           //
		);
	}

	Value builtin::BS_GlyphSpacingAdjustment::make(Evaluator* ev, const GlyphSpacingAdjustment& adj)
	{
		auto matches = make_array(Type::makeArray(Type::makeChar()), adj.match, [](const auto& m) {
			return make_array(Type::makeChar(), m, [](const auto& ch) { return Value::character(ch); });
		});

		auto adjusts = make_array(Type::makeFloating(), adj.adjust, [](double x) { return Value::floating(x); });

		return StructMaker(BS_GlyphSpacingAdjustment::type->toStruct())
		    .set("match", std::move(matches))
		    .set("adjust", std::move(adjusts))
		    .make();
	}

	GlyphSpacingAdjustment builtin::BS_GlyphSpacingAdjustment::unmake(Evaluator* ev, const Value& value)
	{
		GlyphSpacingAdjustment ret {};
		ret.match = unmake_array<std::vector<char32_t>>(value.getStructField("match"), [](const auto& v) {
			return unmake_array<char32_t>(v, [](const auto& vv) { return vv.getChar(); });
		});

		ret.adjust = unmake_array<double>(value.getStructField("adjust"), [](const auto& v) {
			return v.getFloating();
		});

		return ret;
	}

}
