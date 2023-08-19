// bs_colour.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "interp/interp.h"
#include "interp/builtin_fns.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;
	using Field = ast::StructDefn::Field;

	const Type* builtin::BS_Colour::type = nullptr;
	std::vector<Field> builtin::BS_Colour::fields()
	{
		auto make_zero = []() {
			auto ret = std::make_unique<ast::NumberLit>(Location::builtin());
			ret->is_floating = true;
			ret->float_value = 0.0;
			return ret;
		};

		using A = ast::StructLit::Arg;

		auto cmyk_init = std::make_unique<ast::StructLit>(Location::builtin());
		cmyk_init->struct_name = ptype_for_builtin<BS_ColourCMYK>().name();
		cmyk_init->field_inits = util::vectorOf( //
		    A { "c", make_zero() },              //
		    A { "m", make_zero() },              //
		    A { "y", make_zero() },              //
		    A { "k", make_zero() });

		auto rgb_init = std::make_unique<ast::StructLit>(Location::builtin());
		rgb_init->struct_name = ptype_for_builtin<BS_ColourRGB>().name();
		rgb_init->field_inits = util::vectorOf( //
		    A { "r", make_zero() },             //
		    A { "g", make_zero() },             //
		    A { "b", make_zero() });

		return util::vectorOf( //
		    Field { .name = "type", .type = ptype_for_builtin<BE_ColourType>() },
		    Field {
		        .name = "cmyk",
		        .type = ptype_for_builtin<BS_ColourCMYK>(),
		        .initialiser = std::move(cmyk_init),
		    },
		    Field {
		        .name = "rgb",
		        .type = ptype_for_builtin<BS_ColourRGB>(),
		        .initialiser = std::move(rgb_init),
		    });
	}



	Value builtin::BS_Colour::make(Evaluator* ev, Colour colour)
	{
		auto maker = StructMaker(BS_Colour::type->toStruct());
		maker.set("type", Value::enumerator(BE_ColourType::type->toEnum(), BE_ColourType::make(colour.type())));

		if(colour.isRGB())
		{
			maker.set("rgb", BS_ColourRGB::make(ev, colour.rgb()));
			maker.set("cmyk", BS_ColourCMYK::make(ev, {}));
		}
		else if(colour.isCMYK())
		{
			maker.set("cmyk", BS_ColourCMYK::make(ev, colour.cmyk()));
			maker.set("rgb", BS_ColourRGB::make(ev, {}));
		}
		else
		{
			sap::internal_error("invalid colour type");
		}

		return maker.make();
	}

	Colour builtin::BS_Colour::unmake(Evaluator* ev, const Value& value)
	{
		auto ct = static_cast<Colour::Type>(value.getStructField("type").getEnumerator().getInteger());
		if(ct == Colour::Type::RGB)
		{
			return Colour {
				.m_type = ct,
				.m_rgb = BS_ColourRGB::unmake(ev, value.getStructField("rgb")),
			};
		}
		else if(ct == Colour::Type::CMYK)
		{
			return Colour {
				.m_type = ct,
				.m_cmyk = BS_ColourCMYK::unmake(ev, value.getStructField("cmyk")),
			};
		}
		else
		{
			sap::internal_error("invalid colour type");
		}
	}










	const Type* builtin::BS_ColourRGB::type = nullptr;
	std::vector<Field> builtin::BS_ColourRGB::fields()
	{
		auto pt_float = PT::named(frontend::TYPE_FLOAT);
		return util::vectorOf(                       //
		    Field { .name = "r", .type = pt_float }, //
		    Field { .name = "g", .type = pt_float }, //
		    Field { .name = "b", .type = pt_float });
	}

	Value builtin::BS_ColourRGB::make(Evaluator* ev, Colour::RGB colour)
	{
		return StructMaker(BS_ColourRGB::type->toStruct())
		    .set("r", Value::floating(colour.r))
		    .set("g", Value::floating(colour.g))
		    .set("b", Value::floating(colour.b))
		    .make();
	}

	Colour::RGB builtin::BS_ColourRGB::unmake(Evaluator* ev, const Value& value)
	{
		return Colour::RGB {
			.r = get_struct_field<double>(value, "r"),
			.g = get_struct_field<double>(value, "g"),
			.b = get_struct_field<double>(value, "b"),
		};
	}


	const Type* builtin::BS_ColourCMYK::type = nullptr;
	std::vector<Field> builtin::BS_ColourCMYK::fields()
	{
		auto pt_float = PT::named(frontend::TYPE_FLOAT);
		return util::vectorOf(                       //
		    Field { .name = "c", .type = pt_float }, //
		    Field { .name = "m", .type = pt_float }, //
		    Field { .name = "y", .type = pt_float }, Field { .name = "k", .type = pt_float });
	}

	Value builtin::BS_ColourCMYK::make(Evaluator* ev, Colour::CMYK colour)
	{
		return StructMaker(BS_ColourCMYK::type->toStruct())
		    .set("c", Value::floating(colour.c))
		    .set("m", Value::floating(colour.m))
		    .set("y", Value::floating(colour.y))
		    .set("k", Value::floating(colour.k))
		    .make();
	}

	Colour::CMYK builtin::BS_ColourCMYK::unmake(Evaluator* ev, const Value& value)
	{
		return Colour::CMYK {
			.c = get_struct_field<double>(value, "c"),
			.m = get_struct_field<double>(value, "m"),
			.y = get_struct_field<double>(value, "y"),
			.k = get_struct_field<double>(value, "k"),
		};
	}






	const Type* builtin::BE_ColourType::type = nullptr;

	frontend::PType BE_ColourType::enumeratorType()
	{
		return PT::named(frontend::TYPE_INT);
	}

	std::vector<ast::EnumDefn::EnumeratorDefn> BE_ColourType::enumerators()
	{
		auto make_int = [](int value) {
			auto ret = std::make_unique<ast::NumberLit>(Location::builtin());
			ret->is_floating = false;
			ret->int_value = value;
			return ret;
		};

		using ED = ast::EnumDefn::EnumeratorDefn;
		return util::vectorOf(                                                    //
		    ED(Location::builtin(), "RGB", make_int((int) Colour::Type::RGB)),    //
		    ED(Location::builtin(), "CMYK", make_int((int) Colour::Type::CMYK))); //
	}

	Value builtin::BE_ColourType::make(Colour::Type ct)
	{
		return Value::enumerator(type->toEnum(), Value::integer(static_cast<int>(ct)));
	}

	Colour::Type builtin::BE_ColourType::unmake(const Value& value)
	{
		return static_cast<Colour::Type>(value.getEnumerator().getInteger());
	}
}
