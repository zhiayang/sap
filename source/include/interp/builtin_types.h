// builtin_types.h
// Copyright (c) 2022, zhiayang
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "util.h"

#include "interp/ast.h"
#include "interp/parser_type.h"

namespace sap::interp::builtin
{
	struct BStyle
	{
		static constexpr auto name = "Style";

		static const Type* type;
		static std::vector<StructDefn::Field> fields();
	};
};
