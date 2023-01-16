// be_alignment.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"

#include "interp/value.h"
#include "interp/builtin_types.h"

namespace sap::interp::builtin
{
	using PT = frontend::PType;

	const Type* builtin::BE_Alignment::type = nullptr;

	frontend::PType BE_Alignment::enumeratorType()
	{
		return PT::named(frontend::TYPE_INT);
	}

	std::vector<EnumDefn::EnumeratorDefn> BE_Alignment::enumerators()
	{
		auto make_int = [](int value) {
			auto ret = std::make_unique<NumberLit>();
			ret->is_floating = false;
			ret->int_value = value;
			return ret;
		};

		using ED = EnumDefn::EnumeratorDefn;
		return util::vectorOf(                                 //
		    ED("Left", make_int((int) Alignment::Left)),       //
		    ED("Right", make_int((int) Alignment::Right)),     //
		    ED("Centred", make_int((int) Alignment::Centre)),  //
		    ED("Centered", make_int((int) Alignment::Centre)), //
		    ED("Justified", make_int((int) Alignment::Justified)));
	}

	Value builtin::BE_Alignment::make(Alignment alignment)
	{
		return Value::enumerator(type->toEnum(), Value::integer(static_cast<int>(alignment)));
	}

	Alignment builtin::BE_Alignment::unmake(const Value& value)
	{
		return static_cast<Alignment>(value.getEnumerator().getInteger());
	}
}
