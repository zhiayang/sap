// be_alignment.cpp
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#include "sap/style.h"
#include "tree/base.h"
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

	std::vector<ast::EnumDefn::Enumerator> BE_Alignment::enumerators()
	{
		return util::vectorOf(                                                          //
			make_builtin_enumerator("Left", static_cast<int>(Alignment::Left)),         //
			make_builtin_enumerator("Right", static_cast<int>(Alignment::Right)),       //
			make_builtin_enumerator("Centred", static_cast<int>(Alignment::Centre)),    //
			make_builtin_enumerator("Centered", static_cast<int>(Alignment::Centre)),   //
			make_builtin_enumerator("Justified", static_cast<int>(Alignment::Justified)));
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
